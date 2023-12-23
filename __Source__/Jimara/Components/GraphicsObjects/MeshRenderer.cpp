#include "MeshRenderer.h"
#include "../../Math/Helpers.h"
#include "../../Data/Geometry/GraphicsMesh.h"
#include "../../Data/Geometry/MeshBoundingBox.h"
#include "../../Graphics/Pipeline/OneTimeCommandPool.h"
#include "../../Data/Materials/StandardLitShaderInputs.h"
#include "../../Environment/Rendering/Culling/FrustrumAABB/FrustrumAABBCulling.h"
#include "../../Environment/Rendering/SceneObjects/Objects/GraphicsObjectDescriptor.h"


namespace Jimara {
	namespace {
#pragma warning(disable: 4250)
		class MeshRenderPipelineDescriptor 
			: public virtual ObjectCache<TriMeshRenderer::Configuration>::StoredObject
			, public virtual ObjectCache<Reference<const Object>>
			, public virtual GraphicsObjectDescriptor
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

			using InstanceInfo = Culling::FrustrumAABBCulling::CulledInstanceInfo;

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
						m_bufferBinding->BoundObject() = m_device->CreateArrayBuffer<InstanceInfo>(count, Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
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
						Graphics::ArrayBuffer* dataBuffer;
						if (!m_isStatic) {
							Graphics::ArrayBufferReference<InstanceInfo>& buffer = m_bufferCache[m_bufferCacheIndex];
							m_bufferCacheIndex = (m_bufferCacheIndex + 1u) % m_bufferCache.Size();
							if (buffer == nullptr || buffer->ObjectCount() < m_instanceCount)
								buffer = m_device->CreateArrayBuffer<InstanceInfo>(m_instanceCount, Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
							dataBuffer = buffer;
						}
						else dataBuffer = m_bufferBinding->BoundObject();
						{
							InstanceInfo* instanceData = reinterpret_cast<InstanceInfo*>(dataBuffer->Map());
							const size_t instanceCount = m_components.size();
							const Matrix4* const instanceTransforms = m_transformBufferData.data();
							for (size_t instanceId = 0u; instanceId < instanceCount; instanceId++) {
								instanceData->transform = instanceTransforms[instanceId];
								instanceData->index = static_cast<uint32_t>(instanceId);
								instanceData++;
							}
							dataBuffer->Unmap(true);
						}
						if (dataBuffer != m_bufferBinding->BoundObject()) {
							if (context == nullptr) {
								Reference<Graphics::OneTimeCommandPool> pool = Graphics::OneTimeCommandPool::GetFor(m_device);
								Graphics::OneTimeCommandPool::Buffer commandBuffer(pool);
								m_bufferBinding->BoundObject()->Copy(commandBuffer, dataBuffer);
							}
							else {
								Graphics::CommandBuffer* commandBuffer = context->Graphics()->GetWorkerThreadCommandBuffer();
								m_bufferBinding->BoundObject()->Copy(commandBuffer, dataBuffer);
							}
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

			struct ViewportDataUpdater : public virtual JobSystem::Job {
				mutable SpinLock ownerLock;
				MeshRenderPipelineDescriptor* owner = nullptr;
				EventInstance<> m_onUpdate;

				inline ViewportDataUpdater() {}
				inline virtual ~ViewportDataUpdater() {}

				inline Reference<MeshRenderPipelineDescriptor> Owner()const {
					Reference<MeshRenderPipelineDescriptor> data;
					{
						std::unique_lock<SpinLock> lock(ownerLock);
						data = owner;
					}
					return data;
				}

				virtual void Execute() override { if (Owner() != nullptr) m_onUpdate(); }
				virtual void CollectDependencies(Callback<Job*> addDependency) override {
					Reference<MeshRenderPipelineDescriptor> owner = Owner();
					if (owner != nullptr)
						addDependency(owner);
				}
			};
			const Reference<ViewportDataUpdater> m_viewportDataUpdater = Object::Instantiate<ViewportDataUpdater>();


			class ViewportData 
				: public virtual GraphicsObjectDescriptor::ViewportData
				, public virtual ObjectCache<Reference<const Object>>::StoredObject {
			private:
				MeshRenderPipelineDescriptor* const m_pipelineDescriptor;
				const Reference<ViewportDataUpdater> m_updater;
				const Reference<const RendererFrustrumDescriptor> m_frustrumDescriptor;
				const Reference<TriMeshBoundingBox> m_meshBounds;
				const Reference<Culling::FrustrumAABBCulling> m_cullTask;
				const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_instanceBufferBinding =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				const Graphics::IndirectDrawBufferReference m_indirectDrawBuffer;
				Graphics::DrawIndirectCommand m_lastDrawCommand = {};
				GraphicsSimulation::TaskBinding m_cullTaskBinding;

				inline void UpdateIndirectDrawBuffer(bool force) {
					{
						const uint32_t indexCount = static_cast<uint32_t>(IndexCount());
						force |= (indexCount != m_lastDrawCommand.indexCount);
						m_lastDrawCommand.indexCount = indexCount;
					}
					{
						m_lastDrawCommand.instanceCount = static_cast<uint32_t>(m_pipelineDescriptor->m_instanceBuffer.InstanceCount());
					}
					{
						m_lastDrawCommand.firstIndex = 0u;
						m_lastDrawCommand.vertexOffset = 0u;
						m_lastDrawCommand.firstInstance = 0u;
					}
					if (force) {
						reinterpret_cast<Graphics::DrawIndirectCommand*>(m_indirectDrawBuffer->Map())[0u] = m_lastDrawCommand;
						m_indirectDrawBuffer->Unmap(true);
					}
				}

				inline void Update() {
					UpdateIndirectDrawBuffer(false);

					Graphics::ArrayBuffer* srcBuffer = m_pipelineDescriptor->m_instanceBuffer.Buffer()->BoundObject();
					if (m_instanceBufferBinding->BoundObject() == nullptr ||
						m_instanceBufferBinding->BoundObject()->ObjectCount() < Math::Max(m_lastDrawCommand.instanceCount, 1u) ||
						m_instanceBufferBinding->BoundObject() == srcBuffer)
						m_instanceBufferBinding->BoundObject() = m_pipelineDescriptor->m_desc.context
							->Graphics()->Device()->CreateArrayBuffer<InstanceInfo>(Math::Max(m_lastDrawCommand.instanceCount, 1u));

					if (m_instanceBufferBinding->BoundObject() == nullptr || m_instanceBufferBinding->BoundObject() == srcBuffer) {
						m_pipelineDescriptor->m_desc.context->Log()->Error("MeshRenderPipelineDescriptor::ViewportData::Update - ",
							"Failed to allocate culled instance buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						m_instanceBufferBinding->BoundObject() = srcBuffer;
						m_cullTask->Configure({}, {}, 0u, nullptr, nullptr, nullptr, 0u);
						UpdateIndirectDrawBuffer(true);
						return;
					}

					const Matrix4 frustrum = (m_frustrumDescriptor != nullptr)
						? m_frustrumDescriptor->FrustrumTransform() : Matrix4(0.0f);
					const AABB bounds = m_meshBounds->GetMeshBounds();

					m_cullTask->Configure(frustrum, bounds, m_lastDrawCommand.instanceCount,
						srcBuffer, m_instanceBufferBinding->BoundObject(),
						m_indirectDrawBuffer, offsetof(Graphics::DrawIndirectCommand, instanceCount));
				}

			public:
				inline ViewportData(
					MeshRenderPipelineDescriptor* pipelineDescriptor,
					const RendererFrustrumDescriptor* frustrumDescriptor)
					: GraphicsObjectDescriptor::ViewportData(
						pipelineDescriptor->m_desc.context, 
						pipelineDescriptor->m_desc.material->Shader(), 
						pipelineDescriptor->m_desc.geometryType)
					, m_pipelineDescriptor(pipelineDescriptor)
					, m_updater(pipelineDescriptor->m_viewportDataUpdater)
					, m_frustrumDescriptor(frustrumDescriptor)
					, m_meshBounds(TriMeshBoundingBox::GetFor(pipelineDescriptor->m_desc.mesh))
					, m_cullTask(Object::Instantiate<Culling::FrustrumAABBCulling>(pipelineDescriptor->m_desc.context))
					, m_indirectDrawBuffer(pipelineDescriptor->m_desc.context->Graphics()->Device()
						->CreateIndirectDrawBuffer(1u, Graphics::Buffer::CPUAccess::CPU_READ_WRITE)) {
					m_cullTaskBinding = m_cullTask;
					assert(m_indirectDrawBuffer != nullptr);
					{
						m_updater->m_onUpdate.operator Jimara::Event<>& () += Callback(&ViewportData::Update, this);
						std::unique_lock<std::mutex> lock(m_pipelineDescriptor->m_lock);
						UpdateIndirectDrawBuffer(true);
						m_instanceBufferBinding->BoundObject() = m_pipelineDescriptor->m_instanceBuffer.Buffer()->BoundObject();
					}
				}

				inline virtual ~ViewportData() {
					m_updater->m_onUpdate.operator Jimara::Event<>& () -= Callback(&ViewportData::Update, this);
					m_cullTaskBinding = nullptr;
				}

				inline virtual Graphics::BindingSet::BindingSearchFunctions BindingSearchFunctions()const override {
					return m_pipelineDescriptor->m_cachedMaterialInstance.BindingSearchFunctions();
				}

				inline virtual GraphicsObjectDescriptor::VertexInputInfo VertexInput()const override {
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
						vertexInfo.binding = m_pipelineDescriptor->m_meshBuffers.Buffer();
					}
					{
						GraphicsObjectDescriptor::VertexBufferInfo& instanceInfo = info.vertexBuffers[1u];
						instanceInfo.layout.inputRate = Graphics::GraphicsPipeline::VertexInputInfo::InputRate::INSTANCE;
						instanceInfo.layout.bufferElementSize = sizeof(InstanceInfo);
						instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
							StandardLitShaderInputs::JM_ObjectTransform_Location, offsetof(InstanceInfo, transform)));
						instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
							StandardLitShaderInputs::JM_ObjectIndex_Location, offsetof(InstanceInfo, index)));
						instanceInfo.binding = m_instanceBufferBinding;
					}
					info.indexBuffer = m_pipelineDescriptor->m_meshBuffers.IndexBuffer();
					return info;
				}

				virtual Graphics::IndirectDrawBufferReference IndirectBuffer()const override { return m_indirectDrawBuffer; }

				inline virtual size_t IndexCount()const override { return m_pipelineDescriptor->m_meshBuffers.IndexBuffer()->BoundObject()->ObjectCount(); }

				inline virtual size_t InstanceCount()const override { return 1u; }

				inline virtual Reference<Component> GetComponent(size_t objectIndex)const override {
					return m_pipelineDescriptor->m_instanceBuffer.FindComponent(objectIndex);
				}
			};


		public:
			inline MeshRenderPipelineDescriptor(const TriMeshRenderer::Configuration& desc)
				: GraphicsObjectDescriptor(desc.layer)
				, m_desc(desc)
				, m_graphicsObjectSet(GraphicsObjectDescriptor::Set::GetInstance(desc.context))
				, m_cachedMaterialInstance(desc.material)
				, m_meshBuffers(desc)
				, m_instanceBuffer(desc.context->Graphics()->Device(), desc.isStatic, desc.context->Graphics()->Configuration().MaxInFlightCommandBufferCount()) {
				m_viewportDataUpdater->owner = this;
				m_desc.context->Graphics()->SynchPointJobs().Add(m_viewportDataUpdater);
			}

			inline virtual ~MeshRenderPipelineDescriptor() 
			{
				m_desc.context->Graphics()->SynchPointJobs().Remove(m_viewportDataUpdater);
				{
					std::unique_lock<SpinLock> lock(m_viewportDataUpdater->ownerLock);
					m_viewportDataUpdater->owner = nullptr;
				}
			}

			/** GraphicsObjectDescriptor */
			inline virtual Reference<const GraphicsObjectDescriptor::ViewportData> GetViewportData(const RendererFrustrumDescriptor* frustrum) override { 
				return GetCachedOrCreate(frustrum, false, [&]() { return Object::Instantiate<ViewportData>(this, frustrum); });
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
