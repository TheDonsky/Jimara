#include "GraphicsMesh.h"
#include "../../Math/Helpers.h"
#include <set>


namespace Jimara {
	namespace Graphics {
		namespace {
			struct GraphicsMeshCachedId {
				Reference<GraphicsDevice> device;
				Reference<const TriMesh> mesh;
				Experimental::GraphicsPipeline::IndexType geometryType = Experimental::GraphicsPipeline::IndexType::TRIANGLE;

				inline bool operator<(const GraphicsMeshCachedId& id)const {
					if (device < id.device) return true;
					else if (device > id.device) return false;
					else if (mesh < id.mesh) return true;
					else if (mesh > id.mesh) return false;
					else return geometryType < id.geometryType;
				}

				inline bool operator==(const GraphicsMeshCachedId& id)const {
					return (device == id.device)
						&& (mesh == id.mesh)
						&& (geometryType == id.geometryType);
				}
			};
		}
	}
}

namespace std {
	template<>
	struct hash<Jimara::Graphics::GraphicsMeshCachedId> {
		size_t operator()(const Jimara::Graphics::GraphicsMeshCachedId& id)const {
			size_t deviceHash = std::hash<Jimara::Graphics::GraphicsDevice*>()(id.device);
			size_t meshHash = std::hash<const Jimara::TriMesh*>()(id.mesh);
			size_t geometryTypeHash = std::hash<uint8_t>()(static_cast<uint8_t>(id.geometryType));
			return Jimara::MergeHashes(deviceHash, Jimara::MergeHashes(meshHash, geometryTypeHash));
		}
	};
}

namespace Jimara {
	namespace Graphics {
		namespace {
#pragma warning(disable: 4250)
			class CachedGraphicsMesh : public virtual GraphicsMesh, public virtual ObjectCache<GraphicsMeshCachedId>::StoredObject {
			public:
				inline CachedGraphicsMesh(GraphicsDevice* device, const TriMesh* mesh, Experimental::GraphicsPipeline::IndexType geometryType)
					: GraphicsMesh(device, mesh, geometryType) {}

				class Cache : public virtual ObjectCache<GraphicsMeshCachedId> {
				public:
					inline static Reference<GraphicsMesh> GetFor(const GraphicsMeshCachedId& id) {
						static Cache cache;
						return cache.GetCachedOrCreate(id, false, [&]() { return Object::Instantiate<CachedGraphicsMesh>(id.device, id.mesh, id.geometryType); });
					}
				};
			};
#pragma warning(default: 4250)
		}

		GraphicsMesh::GraphicsMesh(GraphicsDevice* device, const TriMesh* mesh, Experimental::GraphicsPipeline::IndexType geometryType)
			: m_device(device), m_mesh(mesh), m_indexType(geometryType) {
			m_mesh->OnDirty() += Callback(&GraphicsMesh::MeshChanged, this);
		}

		GraphicsMesh::~GraphicsMesh() {
			m_mesh->OnDirty() -= Callback(&GraphicsMesh::MeshChanged, this);
		}

		Reference<GraphicsMesh> GraphicsMesh::Cached(GraphicsDevice* device, const TriMesh* mesh, Experimental::GraphicsPipeline::IndexType geometryType) {
			if (device == nullptr) return nullptr;
			else if (mesh == nullptr) {
				device->Log()->Error("GraphicsMesh::Cached - null mesh provided!");
				return nullptr;
			}
			else return CachedGraphicsMesh::Cache::GetFor(GraphicsMeshCachedId{ device, mesh, geometryType });
		}

		void GraphicsMesh::GetBuffers(ArrayBufferReference<MeshVertex>& vertexBuffer, ArrayBufferReference<uint32_t>& indexBuffer) {
			{
				std::unique_lock<SpinLock> lock(m_bufferLock);
				vertexBuffer = m_vertexBuffer;
				indexBuffer = m_indexBuffer;
				if (vertexBuffer != nullptr && indexBuffer != nullptr) return;
			}
			TriMesh::Reader reader(m_mesh);
			if (vertexBuffer == nullptr) {
				vertexBuffer = m_device->CreateArrayBuffer<MeshVertex>(reader.VertCount());
				MeshVertex* verts = vertexBuffer.Map();
				for (uint32_t i = 0; i < reader.VertCount(); i++)
					verts[i] = reader.Vert(i);
				vertexBuffer->Unmap(true);
			}
			if (indexBuffer == nullptr) {
				if (m_indexType == Experimental::GraphicsPipeline::IndexType::TRIANGLE) {
					// Generate triangle indices:
					indexBuffer = m_device->CreateArrayBuffer<uint32_t>(static_cast<size_t>(reader.FaceCount()) * 3u);
					uint32_t* indices = indexBuffer.Map();
					for (uint32_t i = 0; i < reader.FaceCount(); i++) {
						TriangleFace face = reader.Face(i);
						uint32_t index = 3u * i;
						indices[index] = face.a;
						indices[index + 1] = face.b;
						indices[index + 2] = face.c;
					}
					indexBuffer->Unmap(true);
				}
				else if (m_indexType == Experimental::GraphicsPipeline::IndexType::EDGE) {
					// Generate edge indices:
					typedef Stacktor<uint32_t, 16u> EdgeList;
					static thread_local std::vector<EdgeList> edges;
					edges.clear();
					edges.resize(reader.VertCount());
					size_t edgeCount = 0;
					for (uint32_t i = 0; i < reader.FaceCount(); i++) {
						auto addEdge = [&](uint32_t a, uint32_t b) {
							if (a > b) std::swap(a, b);
							else if (a == b) return;
							if (a >= edges.size()) return;
							EdgeList& list = edges[a];
							{
								const uint32_t* ptr = list.Data();
								const uint32_t* const end = ptr + list.Size();
								while (ptr < end) {
									if ((*ptr) == b) return;
									ptr++;
								}
							}
							list.Push(b);
							edgeCount++;
						};
						TriangleFace face = reader.Face(i);
						addEdge(face.a, face.b);
						addEdge(face.b, face.c);
						addEdge(face.c, face.a);
					}
					indexBuffer = m_device->CreateArrayBuffer<uint32_t>(edgeCount * 2u);
					uint32_t* ptr = indexBuffer.Map();
					for (uint32_t a = 0; a < edges.size(); a++) {
						const EdgeList& list = edges[a];
						const uint32_t* dataPtr = list.Data();
						const uint32_t* const dataEnd = dataPtr + list.Size();
						while (dataPtr < dataEnd) {
							(*ptr) = a;
							ptr++;
							(*ptr) = (*dataPtr);
							ptr++;
							dataPtr++;
						}
					}
					indexBuffer->Unmap(true);
					edges.clear();
				}
				else {
					// Generate vertex indices:
					indexBuffer = m_device->CreateArrayBuffer<uint32_t>(static_cast<size_t>(reader.VertCount()));
					uint32_t* indices = indexBuffer.Map();
					for (uint32_t i = 0; i < reader.VertCount(); i++)
						indices[i] = i;
					indexBuffer->Unmap(true);
				}
			}
			{
				std::unique_lock<SpinLock> lock(m_bufferLock);
				m_vertexBuffer = vertexBuffer;
				m_indexBuffer = indexBuffer;
			}
		}

		Event<GraphicsMesh*>& GraphicsMesh::OnInvalidate() { return m_onInvalidate; }

		void GraphicsMesh::MeshChanged(const Mesh<MeshVertex, TriangleFace>* mesh) {
			{
				std::unique_lock<SpinLock> lock(m_bufferLock);
				m_vertexBuffer = nullptr;
				m_indexBuffer = nullptr;
			}
			m_onInvalidate(this);
		}
	}
}
