#include "GraphicsMesh.h"


namespace Jimara {
	namespace Graphics {
		GraphicsMesh::GraphicsMesh(GraphicsDevice* device, const TriMesh* mesh)
			: m_device(device), m_mesh(mesh) {
			m_mesh->OnDirty() += Callback(&GraphicsMesh::MeshChanged, this);
		}

		GraphicsMesh::~GraphicsMesh() {
			m_mesh->OnDirty() -= Callback(&GraphicsMesh::MeshChanged, this);
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


		GraphicsMeshCache::GraphicsMeshCache(GraphicsDevice* device)
			: m_device(device) { }

		Reference<GraphicsMesh> GraphicsMeshCache::GetMesh(const TriMesh* mesh, bool storePermanently) {
			if (mesh == nullptr) return nullptr;
			return GetCachedOrCreate(mesh, storePermanently,
				[&]()->Reference<GraphicsMesh> { return Object::Instantiate<GraphicsMesh>(m_device, mesh); });
		}


		namespace {
#pragma warning(disable: 4250)
			class CacheOfCaches : public virtual ObjectCache<Reference<GraphicsDevice>> {
			private:
				class CachedCache : public virtual GraphicsMeshCache, public virtual ObjectCache<Reference<GraphicsDevice>>::StoredObject {
				public:
					inline CachedCache(GraphicsDevice* device) : GraphicsMeshCache(device) {}
				};

			public:
				inline static Reference<GraphicsMeshCache> ForDevice(GraphicsDevice* device) {
					static const Reference<CacheOfCaches> cache = Object::Instantiate<CacheOfCaches>();
					return cache->GetCachedOrCreate(device, false, [&]()->Reference<GraphicsMeshCache> { return Object::Instantiate<CachedCache>(device); });
				}
			};
#pragma warning(default: 4250)
		}

		Reference<GraphicsMeshCache> GraphicsMeshCache::ForDevice(GraphicsDevice* device) {
			return CacheOfCaches::ForDevice(device);
		}
	}
}
