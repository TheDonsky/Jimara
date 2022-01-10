#include "MeshRenderer.h"
#include "../../Math/Helpers.h"
#include "../../Graphics/Data/GraphicsPipelineSet.h"

namespace Jimara {
	namespace {
		struct InstancedBatchDesc {
			Reference<GraphicsContext> context;
			Reference<const TriMesh> mesh;
			Reference<const Material::Instance> material;
			bool isStatic;

			inline InstancedBatchDesc(GraphicsContext* ctx = nullptr, const TriMesh* geometry = nullptr, const Material::Instance* mat = nullptr, bool stat = false)
				: context(ctx), mesh(geometry), material(mat), isStatic(stat) {}

			inline bool operator<(const InstancedBatchDesc& desc)const {
				if (context < desc.context) return true;
				else if (context > desc.context) return false;
				else if (mesh < desc.mesh) return true;
				else if (mesh > desc.mesh) return false;
				else if (material < desc.material) return true;
				else if (material > desc.material) return false;
				else return isStatic < desc.isStatic;
			}

			inline bool operator==(const InstancedBatchDesc& desc)const {
				return (context == desc.context) && (mesh == desc.mesh) && (material == desc.material) && (isStatic == desc.isStatic);
			}
		};
	}
}

namespace std {
	template<>
	struct hash<Jimara::InstancedBatchDesc> {
		size_t operator()(const Jimara::InstancedBatchDesc& desc)const {
			size_t ctxHash = std::hash<Jimara::GraphicsContext*>()(desc.context);
			size_t meshHash = std::hash<const Jimara::TriMesh*>()(desc.mesh);
			size_t matHash = std::hash<const Jimara::Material::Instance*>()(desc.material);
			size_t staticHash = std::hash<bool>()(desc.isStatic);
			return Jimara::MergeHashes(Jimara::MergeHashes(ctxHash, Jimara::MergeHashes(meshHash, matHash)), staticHash);
		}
	};
}

namespace Jimara {
	namespace {
#pragma warning(disable: 4250)
		class MeshRenderPipelineDescriptor 
			: public virtual ObjectCache<InstancedBatchDesc>::StoredObject
			, public virtual GraphicsObjectDescriptor
			, public virtual GraphicsContext::GraphicsObjectSynchronizer {
		private:
			const InstancedBatchDesc m_desc;
			Material::CachedInstance m_cachedMaterialInstance;
			std::mutex m_lock;

			// Mesh data:
			class MeshBuffers : public virtual Graphics::VertexBuffer {
			private:
				const Reference<Graphics::GraphicsMesh> m_graphicsMesh;
				Graphics::ArrayBufferReference<MeshVertex> m_vertices;
				Graphics::ArrayBufferReference<uint32_t> m_indices;
				std::atomic<bool> m_dirty;

				inline void OnMeshDirty(Graphics::GraphicsMesh*) { m_dirty = true; }

			public:
				inline void Update() {
					if (!m_dirty) return;
					m_graphicsMesh->GetBuffers(m_vertices, m_indices);
					m_dirty = false;
				}

				inline MeshBuffers(const InstancedBatchDesc& desc)
					: m_graphicsMesh(Graphics::GraphicsMeshCache::ForDevice(desc.context->Device())->GetMesh(desc.mesh, false))
					, m_dirty(true) {
					m_graphicsMesh->GetBuffers(m_vertices, m_indices);
					m_graphicsMesh->OnInvalidate() += Callback<Graphics::GraphicsMesh*>(&MeshBuffers::OnMeshDirty, this);
					Update();
				}

				inline virtual ~MeshBuffers() {
					m_graphicsMesh->OnInvalidate() -= Callback<Graphics::GraphicsMesh*>(&MeshBuffers::OnMeshDirty, this);
				}

				inline virtual size_t AttributeCount()const override { return 3; }

				inline virtual AttributeInfo Attribute(size_t index)const override {
					static const AttributeInfo INFOS[] = {
						{ AttributeInfo::Type::FLOAT3, 0, offsetof(MeshVertex, position) },
						{ AttributeInfo::Type::FLOAT3, 1, offsetof(MeshVertex, normal) },
						{ AttributeInfo::Type::FLOAT2, 2, offsetof(MeshVertex, uv) },
					};
					return INFOS[index];
				}

				inline virtual size_t BufferElemSize()const override { return sizeof(MeshVertex); }

				inline virtual Reference<Graphics::ArrayBuffer> Buffer() override { return m_vertices; }

				inline Graphics::ArrayBufferReference<uint32_t> IndexBuffer()const { return m_indices; }
			} mutable m_meshBuffers;

			// Instancing data:
			class InstanceBuffer : public virtual Graphics::InstanceBuffer {
			private:
				Graphics::GraphicsDevice* const m_device;
				const bool m_isStatic;
				std::unordered_map<const Transform*, size_t> m_transformIndices;
				std::vector<Reference<const Transform>> m_transforms;
				std::vector<Matrix4> m_transformBufferData;
				Graphics::ArrayBufferReference<Matrix4> m_buffer;
				std::atomic<bool> m_dirty;
				std::atomic<size_t> m_instanceCount;


			public:
				inline void Update() {
					if ((!m_dirty) && m_isStatic) return;
					m_instanceCount = m_transforms.size();

					bool bufferDirty = (m_buffer == nullptr || m_buffer->ObjectCount() < m_instanceCount);
					size_t i = 0;
					if (bufferDirty) {
						size_t count = m_instanceCount;
						if (count <= 0) count = 1;
						if (m_isStatic)
							m_buffer = m_device->CreateArrayBuffer<Matrix4>(count, Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
						else m_buffer = nullptr;
					}
					else while (i < m_instanceCount) {
						if (m_transforms[i]->WorldMatrix() != m_transformBufferData[i]) {
							bufferDirty = true;
							break;
						}
						else i++;
					}
					if (bufferDirty) {
						while (i < m_transforms.size()) {
							m_transformBufferData[i] = m_transforms[i]->WorldMatrix();
							i++;
						}
						if (!m_isStatic)
							m_buffer = m_device->CreateArrayBuffer<Matrix4>(m_instanceCount, Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
						memcpy(m_buffer.Map(), m_transformBufferData.data(), m_transforms.size() * sizeof(Matrix4));
						m_buffer->Unmap(true);
					}

					m_dirty = false;
				}

				inline InstanceBuffer(Graphics::GraphicsDevice* device, bool isStatic) : m_device(device), m_isStatic(isStatic), m_dirty(true), m_instanceCount(0) { Update(); }

				inline virtual size_t AttributeCount()const override { return 1; }

				inline virtual Graphics::InstanceBuffer::AttributeInfo Attribute(size_t)const {
					return { Graphics::InstanceBuffer::AttributeInfo::Type::MAT_4X4, 3, 0 };
				}

				inline virtual size_t BufferElemSize()const override { return sizeof(Matrix4); }

				inline virtual Reference<Graphics::ArrayBuffer> Buffer() override { return m_buffer; }

				inline size_t InstanceCount()const { return m_instanceCount; }

				inline size_t AddTransform(const Transform* transform) {
					if (m_transformIndices.find(transform) != m_transformIndices.end()) return m_transforms.size();
					m_transformIndices[transform] = m_transforms.size();
					m_transforms.push_back(transform);
					while (m_transformBufferData.size() < m_transforms.size())
						m_transformBufferData.push_back(Matrix4(0.0f));
					m_dirty = true;
					return m_transforms.size();
				}

				inline size_t RemoveTransform(const Transform* transform) {
					std::unordered_map<const Transform*, size_t>::iterator it = m_transformIndices.find(transform);
					if (it == m_transformIndices.end()) return m_transforms.size();
					const size_t lastIndex = m_transforms.size() - 1;
					const size_t index = it->second;
					m_transformIndices.erase(it);
					if (index < lastIndex) {
						const Transform* last = m_transforms[lastIndex];
						m_transforms[index] = last;
						m_transformIndices[last] = index;
					}
					m_transforms.pop_back();
					m_dirty = true;
					return m_transforms.size();
				}
			} mutable m_instanceBuffer;

		public:
			inline MeshRenderPipelineDescriptor(const InstancedBatchDesc& desc)
				: GraphicsObjectDescriptor(desc.material->Shader())
				, m_desc(desc)
				, m_cachedMaterialInstance(desc.material)
				, m_meshBuffers(desc)
				, m_instanceBuffer(desc.context->Device(), desc.isStatic) {}

			inline virtual ~MeshRenderPipelineDescriptor() {}

			/** ShaderResourceBindingSet: */

			inline virtual const Graphics::ShaderResourceBindings::ConstantBufferBinding* FindConstantBufferBinding(const std::string& name)const override {
				// Note: Maybe index would make this bit faster, but binding count is expected to be low and this function is invoked only once per resource per pipeline creation...
				for (size_t i = 0; i < m_cachedMaterialInstance.ConstantBufferCount(); i++)
					if (m_cachedMaterialInstance.ConstantBufferName(i) == name) return m_cachedMaterialInstance.ConstantBuffer(i);
				return nullptr;
			}

			inline virtual const Graphics::ShaderResourceBindings::StructuredBufferBinding* FindStructuredBufferBinding(const std::string& name)const override {
				// Note: Maybe index would make this bit faster, but binding count is expected to be low and this function is invoked only once per resource per pipeline creation...
				for (size_t i = 0; i < m_cachedMaterialInstance.StructuredBufferCount(); i++)
					if (m_cachedMaterialInstance.StructuredBufferName(i) == name) return m_cachedMaterialInstance.StructuredBuffer(i);
				return nullptr;
			}

			inline virtual const Graphics::ShaderResourceBindings::TextureSamplerBinding* FindTextureSamplerBinding(const std::string& name)const override {
				// Note: Maybe index would make this bit faster, but binding count is expected to be low and this function is invoked only once per resource per pipeline creation...
				for (size_t i = 0; i < m_cachedMaterialInstance.TextureSamplerCount(); i++)
					if (m_cachedMaterialInstance.TextureSamplerName(i) == name) return m_cachedMaterialInstance.TextureSampler(i);
				return nullptr;
			}


			/** GraphicsObjectDescriptor */

			inline virtual AABB Bounds()const override {
				// __TODO__: Implement this crap!
				return AABB();
			}

			inline virtual size_t VertexBufferCount()const override { return 1; }

			inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index)const override { return &m_meshBuffers; }

			inline virtual size_t InstanceBufferCount()const override { return 1; }

			inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index)const override { return &m_instanceBuffer; }

			inline virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer()const override { return m_meshBuffers.IndexBuffer(); }

			inline virtual size_t IndexCount()const override { return m_meshBuffers.IndexBuffer()->ObjectCount(); }

			inline virtual size_t InstanceCount()const override { return m_instanceBuffer.InstanceCount(); }


			/** GraphicsContext::GraphicsObjectSynchronizer: */

			virtual inline void OnGraphicsSynch() override {
				std::unique_lock<std::mutex> lock(m_lock);
				m_cachedMaterialInstance.Update();
				m_meshBuffers.Update();
				m_instanceBuffer.Update();
			}

			/** Writer */
			class Writer : public virtual std::unique_lock<std::mutex> {
			private:
				MeshRenderPipelineDescriptor* const m_desc;

			public:
				inline Writer(MeshRenderPipelineDescriptor* desc) : std::unique_lock<std::mutex>(desc->m_lock), m_desc(desc) {}

				void AddTransform(const Transform* transform) {
					if (transform == nullptr) return;
					if (m_desc->m_instanceBuffer.AddTransform(transform) == 1)
						m_desc->m_desc.context->AddSceneObject(m_desc);
				}

				void RemoveTransform(const Transform* transform) {
					if (transform == nullptr) return;
					if (m_desc->m_instanceBuffer.RemoveTransform(transform) <= 0)
						m_desc->m_desc.context->RemoveSceneObject(m_desc);
				}
			};


			/** Instancer: */
			class Instancer : ObjectCache<InstancedBatchDesc> {
			public:
				inline static Reference<MeshRenderPipelineDescriptor> GetDescriptor(const InstancedBatchDesc& desc) {
					static Instancer instance;
					return instance.GetCachedOrCreate(desc, false,
						[&]() -> Reference<MeshRenderPipelineDescriptor> { return Object::Instantiate<MeshRenderPipelineDescriptor>(desc); });
				}
			};
		};
#pragma warning(default: 4250)
	}

	MeshRenderer::MeshRenderer(Component* parent, const std::string_view& name, const TriMesh* mesh, const Jimara::Material* material, bool instanced, bool isStatic) : Component(parent, name) {
		MarkStatic(isStatic);
		RenderInstanced(instanced);
		SetMesh(mesh);
		SetMaterial(material);
	}


	void MeshRenderer::OnTriMeshRendererDirty() {
		if (m_pipelineDescriptor != nullptr) {
			MeshRenderPipelineDescriptor* descriptor = dynamic_cast<MeshRenderPipelineDescriptor*>(m_pipelineDescriptor.operator->());
			{
				MeshRenderPipelineDescriptor::Writer writer(descriptor);
				writer.RemoveTransform(m_descriptorTransform);
			}
			m_pipelineDescriptor = nullptr;
			m_descriptorTransform = nullptr;
		}
		if (Mesh() != nullptr && MaterialInstance() != nullptr) {
			m_descriptorTransform = GetTransfrom();
			if (m_descriptorTransform == nullptr) return;
			const InstancedBatchDesc desc(Context()->Graphics(), Mesh(), MaterialInstance(), IsStatic());
			Reference<MeshRenderPipelineDescriptor> descriptor;
			if (IsInstanced()) descriptor = MeshRenderPipelineDescriptor::Instancer::GetDescriptor(desc);
			else descriptor = Object::Instantiate<MeshRenderPipelineDescriptor>(desc);
			{
				MeshRenderPipelineDescriptor::Writer writer(descriptor);
				writer.AddTransform(m_descriptorTransform);
			}
			m_pipelineDescriptor = descriptor;
		}
	}
}
