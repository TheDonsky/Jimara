#include "MeshRenderer.h"
#include "../../Math/Helpers.h"
#include "../../Graphics/Data/GraphicsPipelineSet.h"
#include "../../Graphics/Data/GraphicsMesh.h"
#include "../../Environment/Rendering/SceneObjects/GraphicsObjectDescriptor.h"


namespace Jimara {
	namespace {
		struct InstancedBatchDesc {
			Reference<SceneContext> context;
			Reference<const TriMesh> mesh;
			Reference<const Material::Instance> material;
			GraphicsLayer layer = 0;
			bool isStatic = false;
			Graphics::GraphicsPipeline::IndexType geometryType = Graphics::GraphicsPipeline::IndexType::TRIANGLE;

			inline InstancedBatchDesc() {}

			inline InstancedBatchDesc(
				SceneContext* ctx,
				const TriMesh* geometry,
				const Material::Instance* mat,
				GraphicsLayer lay,
				bool stat,
				Graphics::GraphicsPipeline::IndexType type)
				: context(ctx), mesh(geometry), material(mat), layer(lay), isStatic(stat), geometryType(type) {}

			inline bool operator<(const InstancedBatchDesc& desc)const {
				if (context < desc.context) return true;
				else if (context > desc.context) return false;
				else if (mesh < desc.mesh) return true;
				else if (mesh > desc.mesh) return false;
				else if (material < desc.material) return true;
				else if (material > desc.material) return false;
				else if (layer < desc.layer) return true;
				else if (layer > desc.layer) return false;
				else if (isStatic < desc.isStatic) return true;
				else if (isStatic > desc.isStatic) return false;
				else return geometryType < desc.geometryType;
			}

			inline bool operator==(const InstancedBatchDesc& desc)const {
				return (context == desc.context)
					&& (mesh == desc.mesh)
					&& (material == desc.material)
					&& (layer == desc.layer)
					&& (isStatic == desc.isStatic)
					&& (geometryType == desc.geometryType);
			}
		};
	}
}

namespace std {
	template<>
	struct hash<Jimara::InstancedBatchDesc> {
		size_t operator()(const Jimara::InstancedBatchDesc& desc)const {
			size_t ctxHash = std::hash<Jimara::SceneContext*>()(desc.context);
			size_t meshHash = std::hash<const Jimara::TriMesh*>()(desc.mesh);
			size_t matHash = std::hash<const Jimara::Material::Instance*>()(desc.material);
			size_t layerHash = std::hash<Jimara::GraphicsLayer>()(desc.layer);
			size_t staticHash = std::hash<bool>()(desc.isStatic);
			size_t geometryTypeHash = std::hash<uint8_t>()(static_cast<uint8_t>(desc.geometryType));
			return Jimara::MergeHashes(
				Jimara::MergeHashes(ctxHash, Jimara::MergeHashes(meshHash, matHash)),
				Jimara::MergeHashes(layerHash, Jimara::MergeHashes(staticHash, geometryTypeHash)));
		}
	};
}

namespace Jimara {
	namespace {
#pragma warning(disable: 4250)
		class MeshRenderPipelineDescriptor 
			: public virtual ObjectCache<InstancedBatchDesc>::StoredObject
			, public virtual GraphicsObjectDescriptor
			, JobSystem::Job {
		private:
			const InstancedBatchDesc m_desc;
			const Reference<GraphicsObjectDescriptor::Set> m_graphicsObjectSet;
			// __TODO__: This is not fully safe... stores self-reference; some refactor down the line would be adviced
			Reference<GraphicsObjectDescriptor::Set::ItemOwner> m_owner = nullptr; 
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
					: m_graphicsMesh(Graphics::GraphicsMesh::Cached(desc.context->Graphics()->Device(), desc.mesh, desc.geometryType))
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
				std::unordered_map<Component*, size_t> m_componentIndices;
				std::vector<Reference<Component>> m_components;
				std::vector<Matrix4> m_transformBufferData;
				Graphics::ArrayBufferReference<Matrix4> m_buffer;
				std::atomic<bool> m_dirty;
				std::atomic<size_t> m_instanceCount;


			public:
				inline void Update() {
					if ((!m_dirty) && m_isStatic) return;
					
					m_instanceCount = m_components.size();

					static thread_local std::vector<Transform*> transforms;
					transforms.clear();
					for (size_t i = 0; i < m_instanceCount; i++)
						transforms.push_back(m_components[i]->GetTransfrom());

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
						if (transforms[i]->WorldMatrix() != m_transformBufferData[i]) {
							bufferDirty = true;
							break;
						}
						else i++;
					}
					if (bufferDirty) {
						while (i < m_components.size()) {
							m_transformBufferData[i] = transforms[i]->WorldMatrix();
							i++;
						}
						if (!m_isStatic)
							m_buffer = m_device->CreateArrayBuffer<Matrix4>(m_instanceCount, Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
						memcpy(m_buffer.Map(), m_transformBufferData.data(), m_components.size() * sizeof(Matrix4));
						m_buffer->Unmap(true);
					}

					transforms.clear();
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

				inline size_t AddComponent(Component* component) {
					if (m_componentIndices.find(component) != m_componentIndices.end()) return m_components.size();
					m_componentIndices[component] = m_components.size();
					m_components.push_back(component);
					while (m_transformBufferData.size() < m_components.size())
						m_transformBufferData.push_back(Matrix4(0.0f));
					m_dirty = true;
					return m_components.size();
				}

				inline size_t RemoveComponent(Component* component) {
					std::unordered_map<Component*, size_t>::iterator it = m_componentIndices.find(component);
					if (it == m_componentIndices.end()) return m_components.size();
					const size_t lastIndex = m_components.size() - 1;
					const size_t index = it->second;
					m_componentIndices.erase(it);
					if (index < lastIndex) {
						Component* last = m_components[lastIndex];
						m_components[index] = last;
						m_componentIndices[last] = index;
					}
					m_components.pop_back();
					m_dirty = true;
					return m_components.size();
				}

				inline Reference<Component> FindComponent(size_t index) {
					if (index >= m_components.size()) return nullptr;
					else return m_components[index];
				}
			} mutable m_instanceBuffer;

		public:
			inline MeshRenderPipelineDescriptor(const InstancedBatchDesc& desc)
				: GraphicsObjectDescriptor(desc.material->Shader(), desc.layer, desc.geometryType)
				, m_desc(desc)
				, m_graphicsObjectSet(GraphicsObjectDescriptor::Set::GetInstance(desc.context))
				, m_cachedMaterialInstance(desc.material)
				, m_meshBuffers(desc)
				, m_instanceBuffer(desc.context->Graphics()->Device(), desc.isStatic) {}

			inline virtual ~MeshRenderPipelineDescriptor() {}

			/** ShaderResourceBindingSet: */

			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override {
				// Note: Maybe index would make this bit faster, but binding count is expected to be low and this function is invoked only once per resource per pipeline creation...
				for (size_t i = 0; i < m_cachedMaterialInstance.ConstantBufferCount(); i++)
					if (m_cachedMaterialInstance.ConstantBufferName(i) == name) return m_cachedMaterialInstance.ConstantBuffer(i);
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override {
				// Note: Maybe index would make this bit faster, but binding count is expected to be low and this function is invoked only once per resource per pipeline creation...
				for (size_t i = 0; i < m_cachedMaterialInstance.StructuredBufferCount(); i++)
					if (m_cachedMaterialInstance.StructuredBufferName(i) == name) return m_cachedMaterialInstance.StructuredBuffer(i);
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string& name)const override {
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

			inline virtual Reference<Component> GetComponent(size_t instanceId, size_t)const override {
				return m_instanceBuffer.FindComponent(instanceId);
			}

		protected:
			/** JobSystem::Job: */
			virtual void CollectDependencies(Callback<Job*>)override {}

			virtual void Execute()final override {
				std::unique_lock<std::mutex> lock(m_lock);
				m_cachedMaterialInstance.Update();
				m_meshBuffers.Update();
				m_instanceBuffer.Update();
			}

		public:
			/** Writer */
			class Writer : public virtual std::unique_lock<std::mutex> {
			private:
				MeshRenderPipelineDescriptor* const m_desc;

			public:
				inline Writer(MeshRenderPipelineDescriptor* desc) : std::unique_lock<std::mutex>(desc->m_lock), m_desc(desc) {}

				void AddComponent(Component* component) {
					if (component == nullptr) return;
					if (m_desc->m_instanceBuffer.AddComponent(component) == 1) {
						if (m_desc->m_owner != nullptr)
							m_desc->m_desc.context->Log()->Fatal(
								"MeshRenderPipelineDescriptor::Writer::AddComponent - m_owner expected to be nullptr! [File: '", __FILE__, "'; Line: ", __LINE__);
						Reference<GraphicsObjectDescriptor::Set::ItemOwner> owner = Object::Instantiate<GraphicsObjectDescriptor::Set::ItemOwner>(m_desc);
						m_desc->m_owner = owner;
						m_desc->m_graphicsObjectSet->Add(owner);
						m_desc->m_desc.context->Graphics()->SynchPointJobs().Add(m_desc);
					}
				}

				void RemoveComponent(Component* component) {
					if (component == nullptr) return;
					if (m_desc->m_instanceBuffer.RemoveComponent(component) <= 0) {
						if (m_desc->m_owner == nullptr)
							m_desc->m_desc.context->Log()->Fatal(
								"MeshRenderPipelineDescriptor::Writer::RemoveComponent - m_owner expected to be non-nullptr! [File: '", __FILE__, "'; Line: ", __LINE__);
						m_desc->m_graphicsObjectSet->Remove(m_desc->m_owner);
						m_desc->m_owner = nullptr;
						m_desc->m_desc.context->Graphics()->SynchPointJobs().Remove(m_desc);
					}
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

	MeshRenderer::MeshRenderer(Component* parent, const std::string_view& name, TriMesh* mesh, Jimara::Material* material, bool instanced, bool isStatic) 
		: Component(parent, name) {
		bool wasEnabled = Enabled();
		SetEnabled(false);
		MarkStatic(isStatic);
		RenderInstanced(instanced);
		SetMesh(mesh);
		SetMaterial(material);
		SetEnabled(wasEnabled);
	}


	void MeshRenderer::OnTriMeshRendererDirty() {
		if (m_pipelineDescriptor != nullptr) {
			MeshRenderPipelineDescriptor* descriptor = dynamic_cast<MeshRenderPipelineDescriptor*>(m_pipelineDescriptor.operator->());
			{
				MeshRenderPipelineDescriptor::Writer writer(descriptor);
				writer.RemoveComponent(this);
			}
			m_pipelineDescriptor = nullptr;
			m_descriptorTransform = nullptr;
		}
		if (ActiveInHeirarchy() && Mesh() != nullptr && MaterialInstance() != nullptr) {
			m_descriptorTransform = GetTransfrom();
			if (m_descriptorTransform == nullptr) return;
			const InstancedBatchDesc desc(Context(), Mesh(), MaterialInstance(), Layer(), IsStatic(), GeometryType());
			Reference<MeshRenderPipelineDescriptor> descriptor;
			if (IsInstanced()) descriptor = MeshRenderPipelineDescriptor::Instancer::GetDescriptor(desc);
			else descriptor = Object::Instantiate<MeshRenderPipelineDescriptor>(desc);
			{
				MeshRenderPipelineDescriptor::Writer writer(descriptor);
				writer.AddComponent(this);
			}
			m_pipelineDescriptor = descriptor;
		}
	}

	namespace {
		class MeshRendererSerializer : public virtual ComponentSerializer::Of<MeshRenderer> {
		public:
			inline MeshRendererSerializer() : ItemSerializer("Jimara/Graphics/MeshRenderer", "Mesh Renderer") {}

			inline static const MeshRendererSerializer* Instance() {
				static const MeshRendererSerializer instance;
				return &instance;
			}

			virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, MeshRenderer* target)const final override {
				TypeId::Of<TriMeshRenderer>().FindAttributeOfType<Serialization::SerializerList::From<TriMeshRenderer>>()->GetFields(recordElement, target);
			}
		};
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<MeshRenderer>(const Callback<const Object*>& report) {
		report(MeshRendererSerializer::Instance());
	}
}
