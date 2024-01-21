#include "MeshRenderer.h"
#include "../../Math/Helpers.h"
#include "../../Data/Geometry/GraphicsMesh.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Graphics/Pipeline/OneTimeCommandPool.h"
#include "../../Data/Materials/StandardLitShaderInputs.h"
#include "../../Environment/Rendering/Culling/FrustrumAABB/FrustrumAABBCulling.h"
#include "../../Environment/Rendering/SceneObjects/Objects/GraphicsObjectDescriptor.h"


namespace Jimara {
	struct MeshRenderer::Helpers {
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

			// Instance buffer data
			struct CulledInstanceInfo {
				alignas(16) Matrix4 transform = {};
				alignas(16) Vector3 pad_0 = {}; // Overlaps with InstanceInfo::instanceData.bboxMin
				alignas(4) uint32_t index = 0u;
				alignas(16) Vector4 pad_1 = {};	// Overlaps with InstanceInfo::instanceData.bboxMax & packedViewportSizeRange
			};
			static_assert(sizeof(CulledInstanceInfo) == (16u * 6u));

			// Pre-cull instance data
			union JIMARA_API InstanceInfo {
				struct {
					alignas(16) Matrix4 instanceTransform = {};
					alignas(16) Vector3 bboxMin = {};
					alignas(4) uint32_t pad_0;	// Overlaps with CulledInstanceInfo::index
					alignas(16) Vector3 bboxMax = {};
					alignas(4) uint32_t packedViewportSizeRange = 0u;
				} instanceData;
				struct {
					alignas(16) CulledInstanceInfo data;
				} culledInstance;

				inline bool operator!=(const InstanceInfo& other)const {
					return
						(instanceData.bboxMin != other.instanceData.bboxMin) ||
						(instanceData.bboxMax != other.instanceData.bboxMax) ||
						(instanceData.instanceTransform != other.instanceData.instanceTransform) ||
						(instanceData.packedViewportSizeRange != other.instanceData.packedViewportSizeRange) ||
						(culledInstance.data.index != other.culledInstance.data.index);
				}
			};
			static_assert(sizeof(InstanceInfo) == (16u * 6));
			static_assert(sizeof(InstanceInfo) == sizeof(CulledInstanceInfo));
			static_assert(offsetof(InstanceInfo, instanceData.instanceTransform) == offsetof(InstanceInfo, culledInstance.data));


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
				inline bool Update() {
					if (!m_dirty) 
						return false;
					m_dirty = false;
					UpdateBuffers();
					return true;
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


			// Instancing data:
			class InstanceBuffer {
			private:
				Graphics::GraphicsDevice* const m_device;
				const Reference<TriMeshBoundingBox> m_meshBBox;
				const bool m_isStatic;
				std::unordered_map<MeshRenderer*, size_t> m_componentIndices;
				std::vector<Reference<MeshRenderer>> m_components;
				std::vector<InstanceInfo> m_transformBufferData;
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
					if ((!m_dirty) && m_isStatic) 
						return;
					
					m_instanceCount = m_components.size();

					bool bufferDirty = (
						m_bufferBinding->BoundObject() == nullptr || 
						m_bufferBinding->BoundObject()->ObjectCount() < m_instanceCount);
					
					const AABB meshBounds = m_meshBBox->GetBoundaries();

					size_t componentId = 0u;
					auto getInstanceInfo = [&]() {
						const MeshRenderer* renderer = m_components[componentId];
						const Transform* transform = renderer->GetTransfrom();
						const RendererCullingOptions& culling = renderer->CullingOptions();
						const Vector3 boundsStart = meshBounds.start - culling.boundaryThickness + culling.boundaryOffset;
						const Vector3 boundsEnd = meshBounds.end + culling.boundaryThickness + culling.boundaryOffset;
						InstanceInfo info = {};
						info.instanceData.bboxMin =
							Vector3(Math::Min(boundsStart.x, boundsEnd.x), Math::Min(boundsStart.y, boundsEnd.y), Math::Min(boundsStart.z, boundsEnd.z));
						info.instanceData.bboxMax =
							Vector3(Math::Max(boundsStart.x, boundsEnd.x), Math::Max(boundsStart.y, boundsEnd.y), Math::Max(boundsStart.z, boundsEnd.z));
						info.instanceData.instanceTransform = (transform == nullptr) ? Math::Identity() : transform->FrameCachedWorldMatrix();
						info.instanceData.packedViewportSizeRange = glm::packHalf2x16(
							(culling.onScreenSizeRangeEnd >= 0.0f) ? Vector2(
								Math::Min(culling.onScreenSizeRangeStart, culling.onScreenSizeRangeEnd),
								Math::Max(culling.onScreenSizeRangeStart, culling.onScreenSizeRangeEnd))
							: Vector2(culling.onScreenSizeRangeStart, -1.0f));
						assert(info.instanceData.instanceTransform == info.culledInstance.data.transform);
						info.culledInstance.data.index = static_cast<uint32_t>(componentId);
						return info;
					};

					if (bufferDirty) {
						size_t count = m_instanceCount;
						if (count <= 0) count = 1;
						m_bufferBinding->BoundObject() = m_device->CreateArrayBuffer<InstanceInfo>(count, Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
					}
					else while (componentId < m_instanceCount) {
						if (getInstanceInfo() != m_transformBufferData[componentId]) {
							bufferDirty = true;
							break;
						}
						else componentId++;
					}
					if (bufferDirty) {
						while (componentId < m_components.size()) {
							m_transformBufferData[componentId] = getInstanceInfo();
							componentId++;
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
							const InstanceInfo* const instanceTransforms = m_transformBufferData.data();
							for (size_t instanceId = 0u; instanceId < instanceCount; instanceId++) {
								(*instanceData) = instanceTransforms[instanceId];
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

					m_dirty = false;
				}

				inline InstanceBuffer(Graphics::GraphicsDevice* device, const TriMesh* mesh, bool isStatic, size_t maxInFlightCommandBuffers) 
					: m_device(device), m_meshBBox(TriMeshBoundingBox::GetFor(mesh)), m_isStatic(isStatic), m_dirty(true), m_instanceCount(0) {
					assert(m_meshBBox != nullptr);
					if (!isStatic)
						m_bufferCache.Resize(maxInFlightCommandBuffers);
					Update(nullptr);
				}

				inline const Graphics::ResourceBinding<Graphics::ArrayBuffer>* Buffer() { return m_bufferBinding; }

				inline size_t InstanceCount()const { return m_instanceCount; }

				inline size_t AddComponent(MeshRenderer* component) {
					if (m_componentIndices.find(component) != m_componentIndices.end()) return m_components.size();
					m_componentIndices[component] = m_components.size();
					m_components.push_back(component);
					while (m_transformBufferData.size() < m_components.size())
						m_transformBufferData.push_back({});
					m_dirty = true;
					return m_components.size();
				}

				inline size_t RemoveComponent(MeshRenderer* component) {
					std::unordered_map<MeshRenderer*, size_t>::iterator it = m_componentIndices.find(component);
					if (it == m_componentIndices.end()) return m_components.size();
					const size_t lastIndex = m_components.size() - 1;
					const size_t index = it->second;
					m_componentIndices.erase(it);
					if (index < lastIndex) {
						MeshRenderer* last = m_components[lastIndex];
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

				inline void MakeDirty() { m_dirty = true; }
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
				const Reference<Culling::FrustrumAABBCulling> m_cullTask;
				const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_instanceBufferBinding =
					Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				const Graphics::IndirectDrawBufferReference m_indirectDrawBuffer;
				Graphics::DrawIndirectCommand m_lastDrawCommand = {};
				GraphicsSimulation::TaskBinding m_cullTaskBinding;

				inline bool IsPrimaryViewport()const {
					if (m_frustrumDescriptor == nullptr)
						return true;
					else return (m_frustrumDescriptor->Flags() & RendererFrustrumFlags::PRIMARY) != RendererFrustrumFlags::NONE;
				}

				inline void UpdateIndirectDrawBuffer(bool force, bool zeroDrawCount) {
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
						Graphics::DrawIndirectCommand& command = *reinterpret_cast<Graphics::DrawIndirectCommand*>(m_indirectDrawBuffer->Map());
						command = m_lastDrawCommand;
						if (zeroDrawCount)
							command.instanceCount = 0u;
						m_indirectDrawBuffer->Unmap(true);
					}
				}

				inline void Update() {
					Graphics::ArrayBuffer* srcBuffer = m_pipelineDescriptor->m_instanceBuffer.Buffer()->BoundObject();

					// On first update, culling is disabled for safety reasons:
					if (m_cullTaskBinding == nullptr) {
						UpdateIndirectDrawBuffer(true, !IsPrimaryViewport());
						m_instanceBufferBinding->BoundObject() = m_pipelineDescriptor->m_instanceBuffer.Buffer()->BoundObject();
						m_cullTask->Configure<InstanceInfo, CulledInstanceInfo>({}, {}, 0u, nullptr, nullptr, nullptr, 0u);
						m_cullTaskBinding = m_cullTask;
						return;
					}

					// Reallocate m_instanceBufferBinding if needed:
					UpdateIndirectDrawBuffer(false, !IsPrimaryViewport());
					if (m_instanceBufferBinding->BoundObject() == nullptr ||
						m_instanceBufferBinding->BoundObject()->ObjectCount() < Math::Max(m_lastDrawCommand.instanceCount, 1u) ||
						m_instanceBufferBinding->BoundObject() == srcBuffer)
						m_instanceBufferBinding->BoundObject() = m_pipelineDescriptor->m_desc.context
							->Graphics()->Device()->CreateArrayBuffer<CulledInstanceInfo>(Math::Max(m_lastDrawCommand.instanceCount, 1u));

					// If m_instanceBufferBinding allocation failed, we do not cull:
					if (m_instanceBufferBinding->BoundObject() == nullptr || m_instanceBufferBinding->BoundObject() == srcBuffer) {
						m_pipelineDescriptor->m_desc.context->Log()->Error("MeshRenderPipelineDescriptor::ViewportData::Update - ",
							"Failed to allocate culled instance buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						m_instanceBufferBinding->BoundObject() = srcBuffer;
						m_cullTask->Configure<InstanceInfo, CulledInstanceInfo>({}, {}, 0u, nullptr, nullptr, nullptr, 0u);
						UpdateIndirectDrawBuffer(true, false);
						return;
					}

					// Get frustrums:
					const Matrix4 cullingFrustrum = (m_frustrumDescriptor != nullptr)
						? m_frustrumDescriptor->FrustrumTransform() : Matrix4(0.0f);
					const Matrix4 viewportFrustrum = [&]() {
						const RendererFrustrumDescriptor* viewportDescriptor = (m_frustrumDescriptor != nullptr)
							? m_frustrumDescriptor->ViewportFrustrumDescriptor() : nullptr;
						return (viewportDescriptor == nullptr) ? cullingFrustrum : viewportDescriptor->FrustrumTransform();
					}();
					
					// Configure culling task:
					m_cullTask->Configure<InstanceInfo, CulledInstanceInfo>(cullingFrustrum, viewportFrustrum, 
						m_lastDrawCommand.instanceCount, srcBuffer, m_instanceBufferBinding->BoundObject(),
						m_indirectDrawBuffer, offsetof(Graphics::DrawIndirectCommand, instanceCount));
				}

			public:
				inline ViewportData(
					MeshRenderPipelineDescriptor* pipelineDescriptor,
					const RendererFrustrumDescriptor* frustrumDescriptor)
					: GraphicsObjectDescriptor::ViewportData(pipelineDescriptor->m_desc.geometryType)
					, m_pipelineDescriptor(pipelineDescriptor)
					, m_updater(pipelineDescriptor->m_viewportDataUpdater)
					, m_frustrumDescriptor(frustrumDescriptor)
					, m_cullTask(Object::Instantiate<Culling::FrustrumAABBCulling>(pipelineDescriptor->m_desc.context))
					, m_indirectDrawBuffer(pipelineDescriptor->m_desc.context->Graphics()->Device()
						->CreateIndirectDrawBuffer(1u, Graphics::Buffer::CPUAccess::CPU_READ_WRITE)) {
					assert(m_indirectDrawBuffer != nullptr);
					{
						UpdateIndirectDrawBuffer(true, !IsPrimaryViewport());
						m_instanceBufferBinding->BoundObject() = m_pipelineDescriptor->m_instanceBuffer.Buffer()->BoundObject();
					}
					m_updater->m_onUpdate.operator Jimara::Event<>& () += Callback(&ViewportData::Update, this);
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
						instanceInfo.layout.bufferElementSize = sizeof(CulledInstanceInfo);
						instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
							StandardLitShaderInputs::JM_ObjectTransform_Location, offsetof(CulledInstanceInfo, transform)));
						instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
							StandardLitShaderInputs::JM_ObjectIndex_Location, offsetof(CulledInstanceInfo, index)));
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
				: GraphicsObjectDescriptor(desc.material->Shader(), desc.layer)
				, m_desc(desc)
				, m_graphicsObjectSet(GraphicsObjectDescriptor::Set::GetInstance(desc.context))
				, m_cachedMaterialInstance(desc.material)
				, m_meshBuffers(desc)
				, m_instanceBuffer(desc.context->Graphics()->Device(), desc.mesh, desc.isStatic, 
					desc.context->Graphics()->Configuration().MaxInFlightCommandBufferCount()) {
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

			inline void MakeInstanceInfoDirty() {
				m_instanceBuffer.MakeDirty();
			}

			/** GraphicsObjectDescriptor */
			inline virtual Reference<const GraphicsObjectDescriptor::ViewportData> GetViewportData(const RendererFrustrumDescriptor* frustrum) override { 
				std::unique_lock<std::mutex> lock(m_lock);
				return GetCachedOrCreate(frustrum, [&]() { return Object::Instantiate<ViewportData>(this, frustrum); });
			}

		protected:
			/** JobSystem::Job: */
			virtual void CollectDependencies(Callback<Job*>)override {}

			virtual void Execute()final override {
				std::unique_lock<std::mutex> lock(m_lock);
				m_cachedMaterialInstance.Update();
				if (m_meshBuffers.Update())
					m_instanceBuffer.MakeDirty();
				m_instanceBuffer.Update(m_desc.context);
			}

		public:
			/** Writer */
			class Writer : public virtual std::unique_lock<std::mutex> {
			private:
				MeshRenderPipelineDescriptor* const m_desc;

			public:
				inline Writer(MeshRenderPipelineDescriptor* desc) : std::unique_lock<std::mutex>(desc->m_lock), m_desc(desc) {}

				void AddComponent(MeshRenderer* component) {
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

				void RemoveComponent(MeshRenderer* component) {
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
					return instance.GetCachedOrCreate(desc,
						[&]() -> Reference<MeshRenderPipelineDescriptor> { return Object::Instantiate<MeshRenderPipelineDescriptor>(desc); });
				}
			};
		};
#pragma warning(default: 4250)
	};

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

	AABB MeshRenderer::GetLocalBoundaries()const {
		Reference<TriMeshBoundingBox> bbox;
		{
			std::unique_lock<SpinLock> lock(m_meshBoundsLock);
			if (m_meshBounds == nullptr || m_meshBounds->TargetMesh() != Mesh())
				m_meshBounds = TriMeshBoundingBox::GetFor(Mesh());
			bbox = m_meshBounds;
		}
		AABB bounds = (bbox == nullptr) ? AABB(Vector3(0.0f), Vector3(0.0f)) : bbox->GetBoundaries();
		const Vector3 start = bounds.start - m_cullingOptions.boundaryThickness + m_cullingOptions.boundaryOffset;
		const Vector3 end = bounds.end + m_cullingOptions.boundaryThickness + m_cullingOptions.boundaryOffset;
		return AABB(
			Vector3(Math::Min(start.x, end.x), Math::Min(start.y, end.y), Math::Min(start.z, end.z)),
			Vector3(Math::Max(start.x, end.x), Math::Max(start.y, end.y), Math::Max(start.z, end.z)));
	}

	AABB MeshRenderer::GetBoundaries()const {
		const AABB localBoundaries = GetLocalBoundaries();
		const Transform* transform = GetTransfrom();
		return (transform == nullptr) ? localBoundaries : (transform->WorldMatrix() * localBoundaries);
	}

	bool MeshRenderer::RendererCullingOptions::operator==(const RendererCullingOptions& other)const {
		return
			boundaryThickness == other.boundaryThickness &&
			boundaryOffset == other.boundaryOffset &&
			onScreenSizeRangeStart == other.onScreenSizeRangeStart &&
			onScreenSizeRangeEnd == other.onScreenSizeRangeEnd;
	}

	void MeshRenderer::SetCullingOptions(const RendererCullingOptions& options) {
		if (options == m_cullingOptions)
			return;
		m_cullingOptions = options;
		Helpers::MeshRenderPipelineDescriptor* descriptor =
			dynamic_cast<Helpers::MeshRenderPipelineDescriptor*>(m_pipelineDescriptor.operator->());
		if (descriptor != nullptr)
			descriptor->MakeInstanceInfoDirty();
	}

	void MeshRenderer::OnTriMeshRendererDirty() {
		GetLocalBoundaries();
		if (m_pipelineDescriptor != nullptr) {
			Helpers::MeshRenderPipelineDescriptor* descriptor = 
				dynamic_cast<Helpers::MeshRenderPipelineDescriptor*>(m_pipelineDescriptor.operator->());
			{
				Helpers::MeshRenderPipelineDescriptor::Writer writer(descriptor);
				writer.RemoveComponent(this);
			}
			m_pipelineDescriptor = nullptr;
		}
		if (ActiveInHeirarchy() && Mesh() != nullptr && MaterialInstance() != nullptr && MaterialInstance()->Shader() != nullptr) {
			const TriMeshRenderer::Configuration desc(this);
			Reference<Helpers::MeshRenderPipelineDescriptor> descriptor;
			if (IsInstanced()) descriptor = Helpers::MeshRenderPipelineDescriptor::Instancer::GetDescriptor(desc);
			else descriptor = Object::Instantiate<Helpers::MeshRenderPipelineDescriptor>(desc);
			{
				Helpers::MeshRenderPipelineDescriptor::Writer writer(descriptor);
				writer.AddComponent(this);
			}
			m_pipelineDescriptor = descriptor;
		}
	}

	
	void MeshRenderer::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		TriMeshRenderer::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			{
				RendererCullingOptions cullingOptions = CullingOptions();
				JIMARA_SERIALIZE_FIELD(cullingOptions, "Culling Options", "Renderer cull/visibility options");
				SetCullingOptions(cullingOptions);
			}
		};
	}

	void MeshRenderer::RendererCullingOptions::Serializer::GetFields(
		const Callback<Serialization::SerializedObject>& recordElement, RendererCullingOptions* target)const {
		JIMARA_SERIALIZE_FIELDS(target, recordElement) {
			JIMARA_SERIALIZE_FIELD(target->boundaryThickness, "Boundary Thickness",
				"'Natural' culling boundary of the geometry will be expanded by this amount in each direction in local space\n"
				"(Useful for the cases when the shader does some vertex displacement and the visible geometry goes out of initial boundaries)");
			JIMARA_SERIALIZE_FIELD(target->boundaryOffset, "Boundary Offset",
				"Local-space culling boundary will be offset by this amount");
			
			static const constexpr std::string_view onScreenSizeRangeHint =
				"Object will be visible if and only if the object occupies \n"
				"a fraction of the viewport between Min and Max on-screen sizes; \n"
				"If Max On-Screen Size is negative, it will be interpreted as unbounded \n"
				"(Hint: You can buld LOD systems with these)";
			JIMARA_SERIALIZE_FIELD(target->onScreenSizeRangeStart, "Min On-Screen Size", onScreenSizeRangeHint);
			{
				const bool onScreenSizeWasPresent = (target->onScreenSizeRangeEnd >= 0.0f);
				bool hasMaxOnScreenSize = onScreenSizeWasPresent;
				JIMARA_SERIALIZE_FIELD(hasMaxOnScreenSize, "Has Max On-Screen Size", onScreenSizeRangeHint);
				if (hasMaxOnScreenSize != onScreenSizeWasPresent) {
					if (hasMaxOnScreenSize)
						target->onScreenSizeRangeEnd = Math::Max(1.0f, target->onScreenSizeRangeEnd);
					else target->onScreenSizeRangeEnd = -1.0f;
				}
			}
			if (target->onScreenSizeRangeEnd >= 0.0f)
				JIMARA_SERIALIZE_FIELD(target->onScreenSizeRangeEnd, "Max On-Screen Size", onScreenSizeRangeHint);
		};
	}


	template<> void TypeIdDetails::GetTypeAttributesOf<MeshRenderer>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<MeshRenderer>(
			"Mesh Renderer", "Jimara/Graphics/MeshRenderer", "Component, that let's the render engine know, a mesh has to be drawn somewhere");
		report(factory);
	}
}
