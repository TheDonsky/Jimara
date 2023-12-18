#include "ParticleRenderer.h"
#include "../../Math/Random.h"
#include "../../Math/BinarySearch.h"
#include "../../Data/Geometry/GraphicsMesh.h"
#include "../../Data/Geometry/MeshGenerator.h"
#include "../../Data/Materials/StandardLitShaderInputs.h"
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

		class MeshBuffers : public virtual Object {
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
				: m_graphicsMesh(Graphics::GraphicsMesh::Cached(
					desc.context->Graphics()->Device(), (desc.mesh == nullptr) ? ViewFacingQuad() : desc.mesh, desc.geometryType))
				, m_dirty(true) {
				UpdateBuffers();
				m_graphicsMesh->OnInvalidate() += Callback<Graphics::GraphicsMesh*>(&MeshBuffers::OnMeshDirty, this);
				Update();
			}

			inline virtual ~MeshBuffers() {
				m_graphicsMesh->OnInvalidate() -= Callback<Graphics::GraphicsMesh*>(&MeshBuffers::OnMeshDirty, this);
			}

			inline const Graphics::ResourceBinding<Graphics::ArrayBuffer>* Buffer()const { return m_vertices; }

			inline const Graphics::ResourceBinding<Graphics::ArrayBuffer>* IndexBuffer()const { return m_indices; }
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
			, public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_sceneContext;
			const Reference<MeshBuffers> m_meshBuffers;
			const Reference<Material::CachedInstance> m_cachedMaterialInstance;
			const Reference<const ViewportDescriptor> m_viewport;
			const Reference<RendererSet> m_rendererSet;
			size_t m_instanceCount = 0u;
			bool m_isNew = true;

			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_instanceBufferBinding = 
				Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
			Graphics::IndirectDrawBufferReference m_indirectBuffer;
			size_t m_lastIndexCount = 0u;

			void BindRendererBuffers(ParticleRenderer* renderer)const {
				dynamic_cast<ParticleInstanceBufferGenerator*>(renderer->m_particleSimulationTask.operator->())
					->BindViewportRange(m_viewport, m_instanceBufferBinding->BoundObject(), m_indirectBuffer);
			}

			void BindAllRendererBuffers()const {
				RendererData* ptr = m_rendererSet->rendererData.Data();
				RendererData* const end = ptr + m_rendererSet->rendererData.Size();
				while (ptr < end) {
					BindRendererBuffers(ptr->renderer);
					ptr++;
				}
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
				if (m_instanceBufferBinding->BoundObject() == nullptr || m_instanceBufferBinding->BoundObject()->ObjectCount() < instanceCount) {
					m_instanceBufferBinding->BoundObject() = m_sceneContext->Graphics()->Device()
						->CreateArrayBuffer<ParticleInstanceBufferGenerator::InstanceData>(instanceCount);
					if (m_instanceBufferBinding->BoundObject() == nullptr) {
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
					const size_t indexCount = m_meshBuffers->IndexBuffer()->BoundObject()->ObjectCount();
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
				if (instanceBufferChanged || indirectBufferChanged || m_isNew) {
					BindAllRendererBuffers();
					// If a new TransformBuffers instance was created at same time as the old one went out of scope,
					// UnbindViewportRange() might be invoked without subsequent BindAllRendererBuffers() call, 
					// resulting in particle system no longer being visible.
					// m_isNew flag guarantees that BindAllRendererBuffers() is always invoked 
					// on the first graphics synch point after TransformBuffers's creation.
					m_isNew = false;
				}
			}

		public:
			inline TransformBuffers(
				const TriMeshRenderer::Configuration& desc, 
				MeshBuffers* meshBuffers, 
				Material::CachedInstance* cachedMaterialInstance, 
				const ViewportDescriptor* viewport,
				RendererSet* rendererSet)
				: GraphicsObjectDescriptor::ViewportData(
					desc.context, desc.material->Shader(), desc.geometryType)
				, m_sceneContext(desc.context)
				, m_meshBuffers(meshBuffers)
				, m_cachedMaterialInstance(cachedMaterialInstance)
				, m_viewport(viewport)
				, m_rendererSet(rendererSet) {
				std::unique_lock<std::mutex> lock(m_rendererSet->lock);
				m_rendererSet->onAdded.operator Jimara::Event<Jimara::ParticleRenderer*>& () += Callback(&TransformBuffers::BindRendererBuffers, this);
				m_rendererSet->onRemoved.operator Jimara::Event<Jimara::ParticleRenderer*>& () += Callback(&TransformBuffers::OnRendererRemoved, this);
				m_rendererSet->onSynch.operator Jimara::Event<>& () += Callback(&TransformBuffers::OnGraphicsSynch, this);
				OnGraphicsSynch();
				m_isNew = true;
			}

			inline virtual ~TransformBuffers() {
				std::unique_lock<std::mutex> lock(m_rendererSet->lock);
				m_rendererSet->onAdded.operator Jimara::Event<Jimara::ParticleRenderer*>& () -= Callback(&TransformBuffers::BindRendererBuffers, this);
				m_rendererSet->onRemoved.operator Jimara::Event<Jimara::ParticleRenderer*>& () -= Callback(&TransformBuffers::OnRendererRemoved, this);
				m_rendererSet->onSynch.operator Jimara::Event<>& () -= Callback(&TransformBuffers::OnGraphicsSynch, this);
				for (size_t i = 0; i < m_rendererSet->rendererData.Size(); i++)
					m_rendererSet->rendererData[i].Unbind(m_viewport);
			}


#pragma region __ShaderResourceBindingSet__
			/** ShaderResourceBindingSet: */
			inline virtual Graphics::BindingSet::BindingSearchFunctions BindingSearchFunctions()const override {
				return m_cachedMaterialInstance->BindingSearchFunctions();
			}
#pragma endregion


#pragma region __GraphicsObjectDescriptor__
			/** GraphicsObjectDescriptor::ViewportData */

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
					vertexInfo.binding = m_meshBuffers->Buffer();
				}
				{
					GraphicsObjectDescriptor::VertexBufferInfo& instanceInfo = info.vertexBuffers[1u];
					instanceInfo.layout.inputRate = Graphics::GraphicsPipeline::VertexInputInfo::InputRate::INSTANCE;
					instanceInfo.layout.bufferElementSize = sizeof(ParticleInstanceBufferGenerator::InstanceData);
					instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
						StandardLitShaderInputs::JM_ObjectTransform_Location, offsetof(ParticleInstanceBufferGenerator::InstanceData, transform)));
					instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
						StandardLitShaderInputs::JM_VertexColor_Location, offsetof(ParticleInstanceBufferGenerator::InstanceData, color)));
					instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
						StandardLitShaderInputs::JM_ObjectTilingAndOffset_Location, offsetof(ParticleInstanceBufferGenerator::InstanceData, tilingAndOffset)));
					instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
						StandardLitShaderInputs::JM_ObjectIndex_Location, offsetof(ParticleInstanceBufferGenerator::InstanceData, objectIndex)));
					instanceInfo.binding = m_instanceBufferBinding;
				}
				info.indexBuffer = m_meshBuffers->IndexBuffer();
				return info;
			}

			inline virtual size_t IndexCount()const override { return m_meshBuffers->IndexBuffer()->BoundObject()->ObjectCount(); }

			inline virtual Graphics::IndirectDrawBufferReference IndirectBuffer()const override { return m_indirectBuffer; }

			inline virtual size_t InstanceCount()const override { return m_instanceCount; }

			inline virtual Reference<Component> GetComponent(size_t objectIndex)const override {
				std::unique_lock<std::mutex> lock(m_rendererSet->lock);
				if (objectIndex < m_rendererSet->rendererData.Size())
					return m_rendererSet->rendererData[objectIndex].renderer;
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
			std::mutex m_viewportDataCreationLock;
			
			Reference<GraphicsObjectDescriptor::Set::ItemOwner> m_owner;
			const Reference<RendererSet> m_rendererSet = Object::Instantiate<RendererSet>();

		public:
			inline PipelineDescriptor(const TriMeshRenderer::Configuration& desc, bool isInstanced)
				: GraphicsObjectDescriptor(desc.layer)
				, m_desc(desc), m_isInstanced(isInstanced)
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


			inline virtual Reference<const ViewportData> GetViewportData(const RendererFrustrumDescriptor* frustrum) override {
				if (m_transformBuffers != nullptr)
					return m_transformBuffers;
				const ViewportDescriptor* viewport = dynamic_cast<const ViewportDescriptor*>(frustrum);
				if (viewport == nullptr)
					return nullptr;
				// Locking is necessary, since concurrent TransformBuffers instantiation 
				// will result in one of them being deleted later down the line and irreversably invoking UnbindViewportRange(), making it invisible...
				std::unique_lock<std::mutex> lock(m_viewportDataCreationLock);
				return GetCachedOrCreate(viewport, false, [&]()->Reference<ViewportData> {
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
					const bool notUpdating = (!m_desc.context->Updating());
					while (ptr < end) {
						const ParticleRenderer* renderer = ptr->renderer;
						if (notUpdating) 
							dynamic_cast<SystemInfo*>(renderer->m_systemInfo.operator->())->MakeDirty();
						dynamic_cast<ParticleInstanceBufferGenerator*>(renderer->m_particleSimulationTask.operator->())
							->Configure(lastInstanceIndex, indirectIndex);
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
				self->m_buffers = nullptr;
			if (budget > 0u)
				self->m_buffers = Object::Instantiate<ParticleBuffers>(self->m_systemInfo, budget);
			if (self->m_simulationStep != nullptr)
				dynamic_cast<ParticleSimulationStep*>(self->m_simulationStep.operator->())->SetBuffers(self->m_buffers);
			if (self->m_particleSimulationTask != nullptr)
				dynamic_cast<ParticleInstanceBufferGenerator*>(self->m_particleSimulationTask.operator->())->SetBuffers(self->m_buffers);
			return true;
		}

		
		class SystemInfo : public virtual ParticleSystemInfo {
		private:
			mutable std::atomic<uint64_t> m_lastFrameIndex;
			mutable SpinLock m_lock;
			mutable Matrix4 m_matrix;

			inline void Update()const {
				const uint64_t frameIndex = Context()->FrameIndex();
				if (m_lastFrameIndex.load() == frameIndex) return;
				std::unique_lock<SpinLock> lock(m_lock);
				if (m_lastFrameIndex.load() == frameIndex) return;
				m_matrix = (transform == nullptr) ? Math::Identity() : transform->WorldMatrix();
				m_lastFrameIndex = frameIndex;
			}

		public:
			Reference<const Transform> transform;

			inline SystemInfo(SceneContext* context) : ParticleSystemInfo(context) {
				MakeDirty();
			}
			inline virtual ~SystemInfo() {}

			inline void MakeDirty() {
				m_lastFrameIndex = Context()->FrameIndex() - 1u;
			}

			inline virtual Matrix4 WorldTransform()const override {
				Update();
				return m_matrix;
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
		, m_systemInfo(Object::Instantiate<Helpers::SystemInfo>(parent->Context())) {
		m_simulationStep = Object::Instantiate<ParticleSimulationStep>(m_systemInfo);
		ParticleSimulationStep* simulationStep = dynamic_cast<ParticleSimulationStep*>(m_simulationStep.operator->());
		simulationStep->InitializationStep()->InitializationTasks().SetLayerCount(1u);
		simulationStep->TimestepTasks().SetLayerCount(1u);
		SetParticleBudget(particleBudget);
	}

	ParticleRenderer::~ParticleRenderer() {
		SetParticleBudget(0u);
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
			{
				bool simulateInLocalSpace = m_systemInfo->HasFlag(ParticleSystemInfo::Flag::SIMULATE_IN_LOCAL_SPACE);
				JIMARA_SERIALIZE_FIELD(simulateInLocalSpace, "Simulate In LocalSpace", "Will cause simulation of this system to run in local space");
				m_systemInfo->SetFlag(ParticleSystemInfo::Flag::SIMULATE_IN_LOCAL_SPACE, simulateInLocalSpace);
			}
			if (m_simulationStep != nullptr) {
				ParticleSimulationStep* simulationStep = dynamic_cast<ParticleSimulationStep*>(m_simulationStep.operator->());
				JIMARA_SERIALIZE_FIELD(simulationStep->InitializationStep()->InitializationTasks(), "Initialization", "Initialization Steps");
				JIMARA_SERIALIZE_FIELD(simulationStep->TimestepTasks(), "Timestep", "Timestep Steps");
			}
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
		{
			dynamic_cast<Helpers::SystemInfo*>(m_systemInfo.operator->())->transform = rendererShouldExist ? GetTransfrom() : nullptr;
			m_systemInfo->SetFlag(ParticleSystemInfo::Flag::INDEPENDENT_PARTICLE_ROTATION, desc.mesh == nullptr);
		}
		if (rendererShouldExist && m_pipelineDescriptor == nullptr && MaterialInstance() != nullptr) {
			{
				Reference<ParticleInstanceBufferGenerator> instanceBufferGenerator = Object::Instantiate<ParticleInstanceBufferGenerator>(
					dynamic_cast<ParticleSimulationStep*>(m_simulationStep.operator->()));
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
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<ParticleRenderer>(
			"Particle Renderer", "Jimara/Graphics/ParticleRenderer", "A renderer, responsible for simulating and rendering particle systems");
		report(factory);
	}
}
