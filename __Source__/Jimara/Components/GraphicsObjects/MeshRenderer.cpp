#include "MeshRenderer.h"
#include "../../Math/Helpers.h"
#include "../../Data/Geometry/GraphicsMesh.h"
#include "../../Data/Materials/StandardLitShaderInputs.h"
#include "../../Environment/Rendering/SceneObjects/Objects/GraphicsObjectDescriptor.h"


namespace Jimara {
	namespace {
#pragma warning(disable: 4250)
		class MeshRenderPipelineDescriptor 
			: public virtual ObjectCache<TriMeshRenderer::Configuration>::StoredObject
			, public virtual GraphicsObjectDescriptor
			, public virtual GraphicsObjectDescriptor::ViewportData
			, public virtual JobSystem::Job {
		private:
			const TriMeshRenderer::Configuration m_desc;
			const Reference<GraphicsObjectDescriptor::Set> m_graphicsObjectSet;
			// __TODO__: This is not fully safe... stores self-reference; some refactor down the line would be adviced
			Reference<GraphicsObjectDescriptor::Set::ItemOwner> m_owner = nullptr; 
			Material::CachedInstance m_cachedMaterialInstance;
			std::mutex m_lock;

			// Mesh data:
			class MeshBuffers {
			private:
				const Reference<Graphics::GraphicsMesh> m_graphicsMesh;
				const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_vertices = Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_indices = Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				std::atomic<bool> m_dirty;

				inline void OnMeshDirty(Graphics::GraphicsMesh*) { m_dirty = true; }

				inline void UpdateBuffers() {
					Graphics::ArrayBufferReference<MeshVertex> vertices;
					Graphics::ArrayBufferReference<uint32_t> indices;
					m_graphicsMesh->GetBuffers(vertices, indices);
					m_vertices->BoundObject() = vertices;
					m_indices->BoundObject() = indices;
				}

			public:
				inline void Update() {
					if (!m_dirty) return;
					m_dirty = false;
					UpdateBuffers();
				}

				inline MeshBuffers(const TriMeshRenderer::Configuration& desc)
					: m_graphicsMesh(Graphics::GraphicsMesh::Cached(desc.context->Graphics()->Device(), desc.mesh, desc.geometryType))
					, m_dirty(true) {
					UpdateBuffers();
					m_graphicsMesh->OnInvalidate() += Callback<Graphics::GraphicsMesh*>(&MeshBuffers::OnMeshDirty, this);
					Update();
				}

				inline virtual ~MeshBuffers() {
					m_graphicsMesh->OnInvalidate() -= Callback<Graphics::GraphicsMesh*>(&MeshBuffers::OnMeshDirty, this);
				}

				inline const Graphics::ResourceBinding<Graphics::ArrayBuffer>* Buffer() { return m_vertices; }

				inline const Graphics::ResourceBinding<Graphics::ArrayBuffer>* IndexBuffer()const { return m_indices; }
			} mutable m_meshBuffers;

			struct InstanceInfo {
				alignas(16) Matrix4 objectTransform;
				alignas(4) uint32_t objectIndex;
			};

			// Instancing data:
			class InstanceBuffer {
			private:
				Graphics::GraphicsDevice* const m_device;
				const bool m_isStatic;
				std::unordered_map<Component*, size_t> m_componentIndices;
				std::vector<Reference<Component>> m_components;
				std::vector<Matrix4> m_transformBufferData;
				Stacktor<Graphics::ArrayBufferReference<InstanceInfo>, 4u> m_bufferCache;
				size_t m_bufferCacheIndex = 0u;
				const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_bufferBinding = 
					Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				std::atomic<bool> m_dirty;
				std::atomic<size_t> m_instanceCount;


			public:
				inline void Update(SceneContext* context) {
					if (m_isStatic && (context != nullptr) && (!context->Updating()))
						m_dirty = true;
					if ((!m_dirty) && m_isStatic) return;
					
					m_instanceCount = m_components.size();

					static thread_local std::vector<Transform*> transforms;
					transforms.clear();
					for (size_t i = 0; i < m_instanceCount; i++)
						transforms.push_back(m_components[i]->GetTransfrom());

					bool bufferDirty = (m_bufferBinding->BoundObject() == nullptr || m_bufferBinding->BoundObject()->ObjectCount() < m_instanceCount);
					size_t i = 0;
					if (bufferDirty) {
						size_t count = m_instanceCount;
						if (count <= 0) count = 1;
						if (m_isStatic)
							m_bufferBinding->BoundObject() = m_device->CreateArrayBuffer<InstanceInfo>(count, Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
						else m_bufferBinding->BoundObject() = nullptr;
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
						if (!m_isStatic) {
							Graphics::ArrayBufferReference<InstanceInfo>& buffer = m_bufferCache[m_bufferCacheIndex];
							m_bufferCacheIndex = (m_bufferCacheIndex + 1u) % m_bufferCache.Size();
							if (buffer == nullptr || buffer->ObjectCount() < m_instanceCount)
								buffer = m_device->CreateArrayBuffer<InstanceInfo>(m_instanceCount, Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
							m_bufferBinding->BoundObject() = buffer;
						}
						{
							InstanceInfo* instanceData = reinterpret_cast<InstanceInfo*>(m_bufferBinding->BoundObject()->Map());
							const size_t instanceCount = m_components.size();
							const Matrix4* const instanceTransforms = m_transformBufferData.data();
							for (size_t instanceId = 0u; instanceId < instanceCount; instanceId++) {
								instanceData->objectTransform = instanceTransforms[instanceId];
								instanceData->objectIndex = static_cast<uint32_t>(instanceId);
								instanceData++;
							}
							m_bufferBinding->BoundObject()->Unmap(true);
						}
					}

					transforms.clear();
					m_dirty = false;
				}

				inline InstanceBuffer(Graphics::GraphicsDevice* device, bool isStatic, size_t maxInFlightCommandBuffers) 
					: m_device(device), m_isStatic(isStatic), m_dirty(true), m_instanceCount(0) { 
					if (!isStatic)
						m_bufferCache.Resize(maxInFlightCommandBuffers);
					Update(nullptr);
				}

				inline const Graphics::ResourceBinding<Graphics::ArrayBuffer>* Buffer() { return m_bufferBinding; }

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
			inline MeshRenderPipelineDescriptor(const TriMeshRenderer::Configuration& desc)
				: GraphicsObjectDescriptor(desc.layer)
				, GraphicsObjectDescriptor::ViewportData(desc.context, desc.material->Shader(), desc.geometryType)
				, m_desc(desc)
				, m_graphicsObjectSet(GraphicsObjectDescriptor::Set::GetInstance(desc.context))
				, m_cachedMaterialInstance(desc.material)
				, m_meshBuffers(desc)
				, m_instanceBuffer(desc.context->Graphics()->Device(), desc.isStatic, desc.context->Graphics()->Configuration().MaxInFlightCommandBufferCount()) {}

			inline virtual ~MeshRenderPipelineDescriptor() {}

			/** ShaderResourceBindingSet: */
			inline virtual Graphics::BindingSet::BindingSearchFunctions BindingSearchFunctions()const override { 
				return m_cachedMaterialInstance.BindingSearchFunctions(); 
			}

			/** GraphicsObjectDescriptor */
			inline virtual Reference<const GraphicsObjectDescriptor::ViewportData> GetViewportData(const RendererFrustrumDescriptor*) override { return this; }

			inline virtual GraphicsObjectDescriptor::VertexInputInfo VertexInput()const {
				GraphicsObjectDescriptor::VertexInputInfo info = {};
				info.vertexBuffers.Resize(2u);
				info.vertexBuffers.Resize(2u);
				{
					GraphicsObjectDescriptor::VertexBufferInfo& vertexInfo = info.vertexBuffers[0u];
					vertexInfo.layout.inputRate = Graphics::GraphicsPipeline::VertexInputInfo::InputRate::VERTEX;
					vertexInfo.layout.bufferElementSize = sizeof(MeshVertex);
					vertexInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
						StandardLitShaderInputs::JM_VertexPosition_Location, offsetof(MeshVertex, position)));
					vertexInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
						StandardLitShaderInputs::JM_VertexNormal_Location, offsetof(MeshVertex, normal)));
					vertexInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
						StandardLitShaderInputs::JM_VertexUV_Location, offsetof(MeshVertex, uv)));
					vertexInfo.binding = m_meshBuffers.Buffer();
				}
				{
					GraphicsObjectDescriptor::VertexBufferInfo& instanceInfo = info.vertexBuffers[1u];
					instanceInfo.layout.inputRate = Graphics::GraphicsPipeline::VertexInputInfo::InputRate::INSTANCE;
					instanceInfo.layout.bufferElementSize = sizeof(InstanceInfo);
					instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
						StandardLitShaderInputs::JM_ObjectTransform_Location, offsetof(InstanceInfo, objectTransform)));
					instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
						StandardLitShaderInputs::JM_ObjectIndex_Location, offsetof(InstanceInfo, objectIndex)));
					instanceInfo.binding = m_instanceBuffer.Buffer();
				}
				info.indexBuffer = m_meshBuffers.IndexBuffer();
				return info;
			}

			inline virtual size_t IndexCount()const override { return m_meshBuffers.IndexBuffer()->BoundObject()->ObjectCount(); }

			inline virtual size_t InstanceCount()const override { return m_instanceBuffer.InstanceCount(); }

			inline virtual Reference<Component> GetComponent(size_t objectIndex)const override {
				return m_instanceBuffer.FindComponent(objectIndex);
			}

		protected:
			/** JobSystem::Job: */
			virtual void CollectDependencies(Callback<Job*>)override {}

			virtual void Execute()final override {
				std::unique_lock<std::mutex> lock(m_lock);
				m_cachedMaterialInstance.Update();
				m_meshBuffers.Update();
				m_instanceBuffer.Update(m_desc.context);
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
			class Instancer : ObjectCache<TriMeshRenderer::Configuration> {
			public:
				inline static Reference<MeshRenderPipelineDescriptor> GetDescriptor(const TriMeshRenderer::Configuration& desc) {
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
		}
		if (ActiveInHeirarchy() && Mesh() != nullptr && MaterialInstance() != nullptr && GetTransfrom() != nullptr) {
			const TriMeshRenderer::Configuration desc(this);
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

	template<> void TypeIdDetails::GetTypeAttributesOf<MeshRenderer>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<MeshRenderer>(
			"Mesh Renderer", "Jimara/Graphics/MeshRenderer", "Component, that let's the render engine know, a mesh has to be drawn somewhere");
		report(factory);
	}
}
