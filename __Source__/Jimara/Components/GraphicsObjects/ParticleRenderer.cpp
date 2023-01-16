#include "ParticleRenderer.h"
#include "../../Math/Random.h"
#include "../../Math/BinarySearch.h"
#include "../../Graphics/Data/GraphicsMesh.h"
#include "../../Data/Generators/MeshGenerator.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Environment/Rendering/SceneObjects/Objects/GraphicsObjectDescriptor.h"
#include "../../Environment/Rendering/Particles/ParticleState.h"
#include "../../Environment/Rendering/Particles/CoreSteps/SimulationStep/ParticleSimulationStepKernel.h"
#include "../../Environment/Rendering/Particles/CoreSteps/InstanceBufferGenerator/InstanceBufferGenerator.h"


namespace Jimara {
	struct ParticleRenderer::Helpers {
		inline static TriMesh* ViewFacingQuad() {
			static const Reference<TriMesh> mesh = GenerateMesh::Tri::Plane(Vector3(0.0f), Math::Right(), Math::Up(), Size2(1u), "Particle_ViewFacingQuad");
			return mesh;
		}

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
				m_dirty = false;
				m_graphicsMesh->GetBuffers(m_vertices, m_indices);
			}

			inline MeshBuffers(const TriMeshRenderer::Configuration& desc)
				: m_graphicsMesh(Graphics::GraphicsMesh::Cached(
					desc.context->Graphics()->Device(), (desc.mesh == nullptr) ? ViewFacingQuad() : desc.mesh, desc.geometryType))
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
		};

		struct RendererData {
			ParticleRenderer* renderer = nullptr;
			size_t instanceEndIndex = 0u;

			inline void Unbind(const ViewportDescriptor* viewprt)const {
				dynamic_cast<ParticleInstanceBufferGenerator*>(renderer->m_particleSimulationTask.operator->())->UnbindViewportRange(viewprt);
			}
		};

		struct RendererSet : public virtual Object {
			std::mutex lock;
			std::unordered_map<ParticleRenderer*, size_t> rendererIndex;
			Stacktor<RendererData, 1u> rendererData;
			EventInstance<ParticleRenderer*> onAdded;
			EventInstance<ParticleRenderer*> onRemoved;
			EventInstance<> onSynch;
		};

#pragma warning(disable: 4250)
		class TransformBuffers 
			: public virtual GraphicsObjectDescriptor::ViewportData
			, public virtual ObjectCache<Reference<const Object>>::StoredObject
			, public virtual Graphics::InstanceBuffer {
		private:
			const Reference<SceneContext> m_sceneContext;
			const Reference<MeshBuffers> m_meshBuffers;
			const Reference<Material::CachedInstance> m_cachedMaterialInstance;
			const Reference<const ViewportDescriptor> m_viewport;
			const Reference<RendererSet> m_rendererSet;
			size_t m_instanceCount = 0u;
			TransformBuffers* const m_self;

			Graphics::ArrayBufferReference<ParticleInstanceBufferGenerator::InstanceData> m_instanceBuffer;
			Graphics::IndirectDrawBufferReference m_indirectBuffer;
			size_t m_lastIndexCount = 0u;

			void BindRendererBuffers(ParticleRenderer* renderer) {
				dynamic_cast<ParticleInstanceBufferGenerator*>(renderer->m_particleSimulationTask.operator->())
					->BindViewportRange(m_viewport, m_instanceBuffer, m_indirectBuffer);
			}

			void OnRendererRemoved(ParticleRenderer* renderer) {
				RendererData data = {};
				data.renderer = renderer;
				data.Unbind(m_viewport);
			}

			void OnGraphicsSynch() {
				const size_t instanceCount = [&]() -> size_t {
					if (m_rendererSet->rendererData.Size() <= 0u) return 0u;
					const RendererData& data = m_rendererSet->rendererData[m_rendererSet->rendererData.Size() - 1u];
					return data.instanceEndIndex;
				}();
				m_instanceCount = m_rendererSet->rendererData.Size();

				// (Re)Create transform buffer if needed:
				bool instanceBufferChanged = false;
				if (m_instanceBuffer == nullptr || m_instanceBuffer->ObjectCount() < instanceCount) {
					m_instanceBuffer = m_sceneContext->Graphics()->Device()->CreateArrayBuffer<ParticleInstanceBufferGenerator::InstanceData>(instanceCount);
					if (m_instanceBuffer == nullptr) {
						m_sceneContext->Log()->Fatal(
							"ParticleRenderer::Helpers::OnGraphicsSynch Failed to allocate instance transform buffer! [File: '", __FILE__, "'; Line: ", __LINE__);
						m_instanceCount = 0u;
						return;
					}
					instanceBufferChanged = true;
				}

				// (Re)Create indirect buffer if needed:
				bool indirectBufferChanged = false;
				if (m_indirectBuffer == nullptr || m_indirectBuffer->ObjectCount() < m_rendererSet->rendererData.Size()) {
					m_indirectBuffer = m_sceneContext->Graphics()->Device()->CreateIndirectDrawBuffer(
						Math::Max(m_indirectBuffer == nullptr ? size_t(1u) : (m_indirectBuffer->ObjectCount() << 1u), m_rendererSet->rendererData.Size()));
					if (m_indirectBuffer == nullptr) {
						m_sceneContext->Log()->Fatal("ParticleRenderer::Helpers::Update Failed to allocate indirect draw buffer! [File: '", __FILE__, "'; Line: ", __LINE__);
						m_instanceCount = 0u;
						return;
					}
					indirectBufferChanged = true;
				}

				// Update DrawIndirectCommands:
				{
					const size_t indexCount = m_meshBuffers->IndexBuffer()->ObjectCount();
					if (indirectBufferChanged || m_lastIndexCount != indexCount) {
						Graphics::DrawIndirectCommand command = {};
						command.indexCount = static_cast<uint32_t>(indexCount);
						Graphics::DrawIndirectCommand* commands = m_indirectBuffer->MapCommands();
						Graphics::DrawIndirectCommand* const end = commands + m_indirectBuffer->ObjectCount();
						while (commands < end) {
							(*commands) = command;
							commands++;
						}
						m_indirectBuffer->Unmap(true);
						m_lastIndexCount = indexCount;
					}
				}

				// Update instance buffer generator tasks:
				if (instanceBufferChanged || indirectBufferChanged) {
					RendererData* ptr = m_rendererSet->rendererData.Data();
					RendererData* const end = ptr + m_rendererSet->rendererData.Size();
					while (ptr < end) {
						BindRendererBuffers(ptr->renderer);
						ptr++;
					}
				}
			}

		public:
			inline TransformBuffers(
				const TriMeshRenderer::Configuration& desc, 
				MeshBuffers* meshBuffers, 
				Material::CachedInstance* cachedMaterialInstance, 
				const ViewportDescriptor* viewport,
				RendererSet* rendererSet)
				: GraphicsObjectDescriptor::ViewportData(desc.material->Shader(), desc.layer, desc.geometryType)
				, m_sceneContext(desc.context)
				, m_meshBuffers(meshBuffers)
				, m_cachedMaterialInstance(cachedMaterialInstance)
				, m_viewport(viewport)
				, m_rendererSet(rendererSet)
				, m_self(this) {
				std::unique_lock<std::mutex> lock(m_rendererSet->lock);
				m_rendererSet->onAdded.operator Jimara::Event<Jimara::ParticleRenderer*>& () += Callback(&TransformBuffers::BindRendererBuffers, this);
				m_rendererSet->onRemoved.operator Jimara::Event<Jimara::ParticleRenderer*>& () += Callback(&TransformBuffers::OnRendererRemoved, this);
				m_rendererSet->onSynch.operator Jimara::Event<>& () += Callback(&TransformBuffers::OnGraphicsSynch, this);
				OnGraphicsSynch();
			}

			inline virtual ~TransformBuffers() {
				std::unique_lock<std::mutex> lock(m_rendererSet->lock);
				m_rendererSet->onAdded.operator Jimara::Event<Jimara::ParticleRenderer*>& () -= Callback(&TransformBuffers::BindRendererBuffers, this);
				m_rendererSet->onRemoved.operator Jimara::Event<Jimara::ParticleRenderer*>& () -= Callback(&TransformBuffers::OnRendererRemoved, this);
				m_rendererSet->onSynch.operator Jimara::Event<>& () -= Callback(&TransformBuffers::OnGraphicsSynch, this);
				for (size_t i = 0; i < m_rendererSet->rendererData.Size(); i++)
					m_rendererSet->rendererData[i].Unbind(m_viewport);
			}

			inline virtual size_t AttributeCount()const override { return 3; }

			inline virtual Graphics::InstanceBuffer::AttributeInfo Attribute(size_t index)const {
				static const Graphics::InstanceBuffer::AttributeInfo INFOS[] = {
					{ Graphics::InstanceBuffer::AttributeInfo::Type::MAT_4X4, 3, offsetof(ParticleInstanceBufferGenerator::InstanceData, transform) },
					{ Graphics::InstanceBuffer::AttributeInfo::Type::FLOAT4, 7, offsetof(ParticleInstanceBufferGenerator::InstanceData, color) },
					{ Graphics::InstanceBuffer::AttributeInfo::Type::FLOAT4, 8, offsetof(ParticleInstanceBufferGenerator::InstanceData, tilingAndOffset) }
				};
				return INFOS[index];
			}

			inline virtual size_t BufferElemSize()const override { return sizeof(ParticleInstanceBufferGenerator::InstanceData); }

			inline virtual Reference<Graphics::ArrayBuffer> Buffer() override { return m_instanceBuffer; }


#pragma region __ShaderResourceBindingSet__
			/** ShaderResourceBindingSet: */

			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override {
				// Note: Maybe index would make this bit faster, but binding count is expected to be low and this function is invoked only once per resource per pipeline creation...
				for (size_t i = 0; i < m_cachedMaterialInstance->ConstantBufferCount(); i++)
					if (m_cachedMaterialInstance->ConstantBufferName(i) == name) return m_cachedMaterialInstance->ConstantBuffer(i);
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override {
				// Note: Maybe index would make this bit faster, but binding count is expected to be low and this function is invoked only once per resource per pipeline creation...
				for (size_t i = 0; i < m_cachedMaterialInstance->StructuredBufferCount(); i++)
					if (m_cachedMaterialInstance->StructuredBufferName(i) == name) return m_cachedMaterialInstance->StructuredBuffer(i);
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string& name)const override {
				// Note: Maybe index would make this bit faster, but binding count is expected to be low and this function is invoked only once per resource per pipeline creation...
				for (size_t i = 0; i < m_cachedMaterialInstance->TextureSamplerCount(); i++)
					if (m_cachedMaterialInstance->TextureSamplerName(i) == name) return m_cachedMaterialInstance->TextureSampler(i);
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string&)const override {
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string&)const override {
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string&)const override {
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string&)const override {
				return nullptr;
			}
#pragma endregion


#pragma region __GraphicsObjectDescriptor__
			/** GraphicsObjectDescriptor::ViewportData */

			inline virtual AABB Bounds()const override {
				// __TODO__: Implement this crap!
				return AABB();
			}

			inline virtual size_t VertexBufferCount()const override { return 1; }
			inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t)const override { return m_meshBuffers; }

			inline virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer()const override { return m_meshBuffers->IndexBuffer(); }
			inline virtual size_t IndexCount()const override { return m_meshBuffers->IndexBuffer()->ObjectCount(); }

			inline virtual Graphics::IndirectDrawBufferReference IndirectBuffer()const override { return m_indirectBuffer; }

			inline virtual size_t InstanceBufferCount()const override { return 1; }
			inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index)const override { return m_self; }
			inline virtual size_t InstanceCount()const override { return m_instanceCount; }

			inline virtual Reference<Component> GetComponent(size_t instanceId, size_t)const override {
				std::unique_lock<std::mutex> lock(m_rendererSet->lock);
				const size_t minEndIndex = (instanceId + 1);
				size_t rendererIndex = BinarySearch_LE(m_rendererSet->rendererData.Size(), 
					[&](size_t index) {
						return m_rendererSet->rendererData[index].instanceEndIndex > minEndIndex;
					});
				if (rendererIndex >= m_rendererSet->rendererData.Size()) {
					if (m_rendererSet->rendererData.Size() <= 0u)
						return nullptr;
					else rendererIndex = 0u;
				}
				const RendererData& data = m_rendererSet->rendererData[rendererIndex];
				if (data.instanceEndIndex < minEndIndex)
					rendererIndex++;
				if (rendererIndex < m_rendererSet->rendererData.Size())
					return m_rendererSet->rendererData[rendererIndex].renderer;
				else return nullptr;
			}
#pragma endregion
		};


		class PipelineDescriptor 
			: public virtual GraphicsObjectDescriptor
			, public virtual ObjectCache<TriMeshRenderer::Configuration>::StoredObject
			, public virtual ObjectCache<Reference<const Object>>
			, public virtual JobSystem::Job {
		private:
			const TriMeshRenderer::Configuration m_desc;
			const Reference<GraphicsObjectDescriptor::Set> m_graphicsObjectSet;
			const bool m_isInstanced;
			const Reference<Material::CachedInstance> m_cachedMaterialInstance;
			const Reference<MeshBuffers> m_meshBuffers;
			Reference<TransformBuffers> m_transformBuffers;
			
			Reference<GraphicsObjectDescriptor::Set::ItemOwner> m_owner;
			const Reference<RendererSet> m_rendererSet = Object::Instantiate<RendererSet>();

		public:
			inline PipelineDescriptor(const TriMeshRenderer::Configuration& desc, bool isInstanced)
				: m_desc(desc), m_isInstanced(isInstanced)
				, m_graphicsObjectSet(GraphicsObjectDescriptor::Set::GetInstance(desc.context))
				, m_cachedMaterialInstance(Object::Instantiate<Material::CachedInstance>(desc.material))
				, m_meshBuffers(Object::Instantiate<MeshBuffers>(desc)) {
				if (m_desc.mesh != nullptr)
					m_transformBuffers = Object::Instantiate<TransformBuffers>(m_desc, m_meshBuffers, m_cachedMaterialInstance, nullptr, m_rendererSet);
			}

			inline virtual ~PipelineDescriptor() {}

			inline const TriMeshRenderer::Configuration& Descriptor()const { return m_desc; }
			inline const bool IsInstanced()const { return m_isInstanced; }

			inline void AddRenderer(ParticleRenderer* renderer) {
				if (renderer == nullptr) return;
				std::unique_lock<std::mutex> lock(m_rendererSet->lock);
				if (m_rendererSet->rendererIndex.find(renderer) != m_rendererSet->rendererIndex.end()) return;

				{
					m_rendererSet->rendererIndex.insert(std::make_pair(renderer, m_rendererSet->rendererData.Size()));
					RendererData data = {};
					data.renderer = renderer;
					if (m_rendererSet->rendererData.Size() > 0u)
						data.instanceEndIndex = m_rendererSet->rendererData[m_rendererSet->rendererData.Size() - 1u].instanceEndIndex;
					m_rendererSet->rendererData.Push(data);
					m_rendererSet->onAdded(renderer);
				}

				if (m_rendererSet->rendererIndex.size() == 1u) {
					if (m_owner != nullptr)
						m_desc.context->Log()->Fatal(
							"ParticleRenderer::Helpers::PipelineDescriptor::AddRenderer - m_owner expected to be nullptr! [File: '", __FILE__, "'; Line: ", __LINE__);
					m_owner = Object::Instantiate<GraphicsObjectDescriptor::Set::ItemOwner>(this);
					m_graphicsObjectSet->Add(m_owner);
					m_desc.context->Graphics()->SynchPointJobs().Add(this);
				}
			}

			inline void RemoveRenderer(ParticleRenderer* renderer) {
				if (renderer == nullptr) return;
				std::unique_lock<std::mutex> lock(m_rendererSet->lock);
				decltype(m_rendererSet->rendererIndex)::iterator it = m_rendererSet->rendererIndex.find(renderer);
				if (it == m_rendererSet->rendererIndex.end()) return;


				{
					const size_t index = it->second;
					m_rendererSet->rendererIndex.erase(it);
					it = decltype(m_rendererSet->rendererIndex)::iterator();
					RendererData& data = m_rendererSet->rendererData[index];
					if (index < (m_rendererSet->rendererData.Size() - 1u)) {
						std::swap(data.renderer, m_rendererSet->rendererData[m_rendererSet->rendererData.Size() - 1u].renderer);
						m_rendererSet->rendererIndex[data.renderer] = index;
					}
					m_rendererSet->rendererData.Pop();
					m_rendererSet->onRemoved(renderer);
				}

				if (m_rendererSet->rendererIndex.size() <= 0u) {
					if (m_owner == nullptr)
						m_desc.context->Log()->Fatal(
							"ParticleRenderer::Helpers::PipelineDescriptor::RemoveRenderer - m_owner expected to be non-nullptr! [File: '", __FILE__, "'; Line: ", __LINE__);
					m_desc.context->Graphics()->SynchPointJobs().Remove(this);
					m_graphicsObjectSet->Remove(m_owner);
					m_owner = nullptr;
				}
			}


			inline virtual Reference<const ViewportData> GetViewportData(const ViewportDescriptor* viewport) override { 
				if (m_transformBuffers != nullptr)
					return m_transformBuffers;
				else if (viewport == nullptr)
					return nullptr;
				else return GetCachedOrCreate(viewport, false, [&]()->Reference<ViewportData> {
					return Object::Instantiate<TransformBuffers>(m_desc, m_meshBuffers, m_cachedMaterialInstance, viewport, m_rendererSet);
					});
			}


		protected:
#pragma region __JobSystem_Job__
			virtual void Execute()final override {
				// Update material and mesh buffers:
				{
					m_cachedMaterialInstance->Update();
					m_meshBuffers->Update();
				}

				std::unique_lock<std::mutex> lock(m_rendererSet->lock);

				// Update transforms and boundaries:
				{
					RendererData* ptr = m_rendererSet->rendererData.Data();
					RendererData* const end = ptr + m_rendererSet->rendererData.Size();
					size_t lastInstanceIndex = 0u;
					size_t indirectIndex = 0u;
					while (ptr < end) {
						const ParticleRenderer* renderer = ptr->renderer;
						const Transform* transform = renderer->GetTransfrom();
						dynamic_cast<ParticleInstanceBufferGenerator*>(renderer->m_particleSimulationTask.operator->())
							->Configure((transform != nullptr) ? transform->WorldMatrix() : Math::Identity(), lastInstanceIndex, indirectIndex);
						lastInstanceIndex += renderer->ParticleBudget();
						ptr->instanceEndIndex = lastInstanceIndex;
						ptr++;
						indirectIndex++;
					}
				}

				// Synch per-viewport:
				m_rendererSet->onSynch();
			}
			virtual void CollectDependencies(Callback<Job*>)override {}
#pragma endregion
		};
#pragma warning(default: 4250)

		class PipelineDescriptorInstancer : public virtual ObjectCache<TriMeshRenderer::Configuration> {
		public:
			inline static Reference<PipelineDescriptor> GetDescriptor(const TriMeshRenderer::Configuration& desc) {
				static PipelineDescriptorInstancer instance;
				return instance.GetCachedOrCreate(desc, false,
					[&]() -> Reference<PipelineDescriptor> { return Object::Instantiate<PipelineDescriptor>(desc, true); });
			}
		};

		inline static bool UpdateParticleBuffers(ParticleRenderer* self, size_t budget) {
			if (self->Destroyed()) {
				budget = 0u;
				self->m_simulationStep = nullptr;
			}
			if (budget == self->ParticleBudget()) return false;
			{
				self->m_buffers = nullptr;
				self->m_particleStateBuffer = nullptr;
			}
			if (budget > 0u) {
				self->m_buffers = Object::Instantiate<ParticleBuffers>(self->m_systemInfo, budget);
				self->m_particleStateBuffer = self->m_buffers->GetBuffer(ParticleState::BufferId());
			}
			if (self->m_simulationStep != nullptr)
				dynamic_cast<ParticleSimulationStep*>(self->m_simulationStep.operator->())->SetBuffers(self->m_buffers);
			if (self->m_particleSimulationTask != nullptr)
				dynamic_cast<ParticleInstanceBufferGenerator*>(self->m_particleSimulationTask.operator->())->SetBuffers(self->m_buffers);
			return true;
		}

		class InitializationStepListSerializer : public virtual Serialization::SerializerList::From<ParticleInitializationStepKernel::Task> {
		private:
			struct StepInfo {
				Reference<const ParticleInitializationTask::Factory::Set> factories;
				ParticleInitializationStepKernel::Task* step = nullptr;
				size_t taskIndex;
			};

			struct StepSerializer : public virtual Serialization::SerializerList::From<StepInfo> {
				inline StepSerializer() : Serialization::ItemSerializer("Step", "Initialization Step") {}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, StepInfo* target)const override {
					Reference<ParticleInitializationTask> task = (target->taskIndex < target->step->InitializationTaskCount())
						? target->step->InitializationTask(target->taskIndex) : nullptr;
					const ParticleInitializationTask::Factory* const initialFactory = (task == nullptr) ? nullptr : target->factories->FindFactory(task);
					Reference<const ParticleInitializationTask::Factory> factory = initialFactory;
					static const ParticleInitializationTask::Factory::RegisteredInstanceSerializer serializer("Type", "Task Type");
					serializer.GetFields(recordElement, &factory);
					if (factory != initialFactory) {
						if (factory != nullptr)
							task = factory->CreateInstance(target->step->Context());
						else task = nullptr;
						target->step->SetInitializationTask(target->taskIndex, task);
					}
					if (task != nullptr) task->GetFields(recordElement);
				}
			};

			inline InitializationStepListSerializer() : Serialization::ItemSerializer("Initialization", "Initialization Steps") {}

		public:
			static const InitializationStepListSerializer* Instance() {
				static const InitializationStepListSerializer instance;
				return &instance;
			}
			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, ParticleInitializationStepKernel::Task* target)const override {
				static thread_local std::vector<StepInfo> steps;
				StepInfo stepInfo = {};
				{
					stepInfo.factories = ParticleInitializationTask::Factory::All();
					stepInfo.step = target;
					stepInfo.taskIndex = 0u;
				}
				steps.clear();
				static const StepSerializer stepSerializer;
				while (true) {
					const size_t initialTaskCount = target->InitializationTaskCount();
					auto isLast = [&]() { return stepInfo.taskIndex >= target->InitializationTaskCount(); };
					bool wasLast = isLast();
					steps.push_back(stepInfo);
					recordElement(stepSerializer.Serialize(steps.back()));
					if (wasLast && isLast()) break;
					else if (initialTaskCount <= target->InitializationTaskCount())
						stepInfo.taskIndex++;
				}
				steps.clear();
			}
		};

		class SystemInfo : public virtual ParticleSystemInfo {
		private:
			mutable std::atomic<size_t> m_lastFrameIndex;
			mutable SpinLock m_lock;
			mutable Matrix4 m_matrix;

			inline void Update()const {
				const size_t frameIndex = Context()->FrameIndex();
				if (m_lastFrameIndex.load() == frameIndex) return;
				std::unique_lock<SpinLock> lock(m_lock);
				if (m_lastFrameIndex.load() == frameIndex) return;
				m_matrix = (transform == nullptr) ? Math::Identity() : transform->WorldMatrix();
				m_lastFrameIndex = frameIndex;
			}

		public:
			Reference<const Transform> transform;

			inline SystemInfo(SceneContext* context) : ParticleSystemInfo(context) {
				m_lastFrameIndex = context->FrameIndex() - 1u;
			}
			inline virtual ~SystemInfo() {}

			inline virtual Matrix4 WorldTransform()const {
				Update();
				return m_matrix;
			}
		};

		class TimestepListSerializer : public virtual Serialization::SerializerList::From<ParticleSimulationStep> {
		private:
			struct StepInfo {
				Reference<const ParticleTimestepTask::Factory::Set> factories;
				ParticleSimulationStep* step = nullptr;
				size_t taskIndex;
			};

			struct StepSerializer : public virtual Serialization::SerializerList::From<StepInfo> {
				inline StepSerializer() : Serialization::ItemSerializer("Step", "Time Step") {}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, StepInfo* target)const override {
					Reference<ParticleTimestepTask> task = (target->taskIndex < target->step->TimestepTaskCount())
						? target->step->TimestepTask(target->taskIndex) : nullptr;
					const ParticleTimestepTask::Factory* const initialFactory = (task == nullptr) ? nullptr : target->factories->FindFactory(task);
					Reference<const ParticleTimestepTask::Factory> factory = initialFactory;
					static const ParticleTimestepTask::Factory::RegisteredInstanceSerializer serializer("Type", "Task Type");
					serializer.GetFields(recordElement, &factory);
					if (factory != initialFactory) {
						if (factory != nullptr)
							task = factory->CreateInstance(target->step->InitializationStep());
						else task = nullptr;
						target->step->SetTimestepTask(target->taskIndex, task);
					}
					if (task != nullptr) task->GetFields(recordElement);
				}
			};

			inline TimestepListSerializer() : Serialization::ItemSerializer("Timestep", "Timestep Steps") {}

		public:
			static const TimestepListSerializer* Instance() {
				static const TimestepListSerializer instance;
				return &instance;
			}
			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, ParticleSimulationStep* target)const override {
				static thread_local std::vector<StepInfo> steps;
				StepInfo stepInfo = {};
				{
					stepInfo.factories = ParticleTimestepTask::Factory::All();
					stepInfo.step = target;
					stepInfo.taskIndex = 0u;
				}
				steps.clear();
				static const StepSerializer stepSerializer;
				while (true) {
					const size_t initialTaskCount = target->TimestepTaskCount();
					auto isLast = [&]() { return stepInfo.taskIndex >= target->TimestepTaskCount(); };
					bool wasLast = isLast();
					steps.push_back(stepInfo);
					recordElement(stepSerializer.Serialize(steps.back()));
					if (wasLast && isLast()) break;
					else if (initialTaskCount <= target->TimestepTaskCount())
						stepInfo.taskIndex++;
				}
				steps.clear();
			}
		};

		static void UpdateEmission(ParticleRenderer* self) {
			self->m_timeSinceLastEmission += self->Context()->Time()->ScaledDeltaTime();
			uint32_t particleCount = static_cast<uint32_t>(self->m_timeSinceLastEmission * self->m_emissionRate);
			if (self->m_buffers != nullptr) 
				self->m_buffers->SetSpawnedParticleCount(particleCount);
			if (particleCount > 0u)
				self->m_timeSinceLastEmission -= particleCount / self->m_emissionRate;
		}
	};

	ParticleRenderer::ParticleRenderer(Component* parent, const std::string_view& name, size_t particleBudget)
		: Component(parent, name)
		, m_systemInfo(Object::Instantiate<Helpers::SystemInfo>(parent->Context()))
		, m_simulationStep(Object::Instantiate<ParticleSimulationStep>(parent->Context())) {
		// __TODO__: Implement this crap!
		SetParticleBudget(particleBudget);
	}

	ParticleRenderer::~ParticleRenderer() {
		SetParticleBudget(0u);
		// __TODO__: Implement this crap!
	}

	size_t ParticleRenderer::ParticleBudget()const { return (m_buffers == nullptr) ? 0u : m_buffers->ParticleBudget(); }

	void ParticleRenderer::SetParticleBudget(size_t budget) {
		if (Helpers::UpdateParticleBuffers(this, budget))
			OnTriMeshRendererDirty();
	}

	void ParticleRenderer::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		TriMeshRenderer::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(ParticleBudget, SetParticleBudget, "Particle Budget", "Maximal number of particles within the system");
			JIMARA_SERIALIZE_FIELD_GET_SET(EmissionRate, SetEmissionRate, "Emission Rate", "Particles emitted per second");
			if (m_simulationStep != nullptr) {
				recordElement(Helpers::InitializationStepListSerializer::Instance()->Serialize(
					dynamic_cast<ParticleSimulationStep*>(m_simulationStep.operator->())->InitializationStep()));
				recordElement(Helpers::TimestepListSerializer::Instance()->Serialize(
					dynamic_cast<ParticleSimulationStep*>(m_simulationStep.operator->())));
			}
			// __TODO__: Implement this crap!
		};
	}

	void ParticleRenderer::OnTriMeshRendererDirty() {
		Helpers::UpdateParticleBuffers(this, ParticleBudget());
		const bool rendererShouldExist = ActiveInHeirarchy() && m_buffers != nullptr;
		const TriMeshRenderer::Configuration desc(this);
		{
			Helpers::PipelineDescriptor* currentPipelineDescriptor = dynamic_cast<Helpers::PipelineDescriptor*>(m_pipelineDescriptor.operator->());
			if (currentPipelineDescriptor != nullptr && (
				(!rendererShouldExist) ||
				(currentPipelineDescriptor->IsInstanced() != IsInstanced()) || 
				(currentPipelineDescriptor->Descriptor() != desc))) {
				currentPipelineDescriptor->RemoveRenderer(this);
				m_pipelineDescriptor = nullptr;
				m_particleSimulationTask = nullptr;
			}
		}
		if (rendererShouldExist && m_pipelineDescriptor == nullptr && MaterialInstance() != nullptr) {
			{
				Reference<ParticleInstanceBufferGenerator> instanceBufferGenerator = Object::Instantiate<ParticleInstanceBufferGenerator>(m_simulationStep);
				m_particleSimulationTask = instanceBufferGenerator;
				instanceBufferGenerator->SetBuffers(m_buffers);
			}
			{
				Reference<Helpers::PipelineDescriptor> descriptor;
				if (IsInstanced()) descriptor = Helpers::PipelineDescriptorInstancer::GetDescriptor(desc);
				else descriptor = Object::Instantiate<Helpers::PipelineDescriptor>(desc, false);
				descriptor->AddRenderer(this);
				m_pipelineDescriptor = descriptor;
			}
		}
		{
			const Callback<> updateCallback(ParticleRenderer::Helpers::UpdateEmission, this);
			if (rendererShouldExist)
				Context()->Graphics()->OnGraphicsSynch() += updateCallback;
			else Context()->Graphics()->OnGraphicsSynch() -= updateCallback;
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleRenderer>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<ParticleRenderer> serializer("Jimara/Graphics/ParticleRenderer", "Particle Renderer");
		report(&serializer);
	}
}
