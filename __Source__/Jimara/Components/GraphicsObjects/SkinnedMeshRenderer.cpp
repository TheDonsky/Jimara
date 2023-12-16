#include "SkinnedMeshRenderer.h"
#include "../../Core/Collections/ObjectSet.h"
#include "../../Graphics/Data/GraphicsMesh.h"
#include "../../Data/Materials/StandardLitShaderInputs.h"
#include "../../Environment/GraphicsSimulation/CombinedGraphicsSimulationKernel.h"
#include "../../Environment/Rendering/SceneObjects/Objects/GraphicsObjectDescriptor.h"


namespace Jimara {
	namespace {
#pragma warning(disable: 4250)
		class SkinnedMeshRenderPipelineDescriptor
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

			struct SkinnedMeshVertex {
				alignas(16) Vector3 position = {};
				alignas(16) Vector3 normal = {};
				alignas(16) Vector2 uv = {};
				alignas(4) uint32_t objectIndex = 0;
			};
			static_assert(sizeof(MeshVertex) == sizeof(SkinnedMeshVertex));

			using BindlessBinding = Reference<const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding>;
			template<typename ReportError>
			inline void SetBindlessBinding(
				BindlessBinding& binding, Graphics::ArrayBuffer* buffer, uint32_t& index, 
				bool& hasNullEntries, const ReportError& reportCouldNotBindError) {
				if (binding == nullptr || binding->BoundObject() != buffer) {
					if (buffer != nullptr) {
						binding = m_desc.context->Graphics()->Bindless().Buffers()->GetBinding(buffer);
						if (binding == nullptr) reportCouldNotBindError();
					}
					else binding = nullptr;
				}
				if (binding != nullptr)
					index = binding->Index();
				else hasNullEntries = true;
			}

			class CombinedDeformationTask : public virtual GraphicsSimulation::Task {
			private:
				class Kernel : public virtual GraphicsSimulation::Kernel {
				public:
					struct SimulationTaskSettings {
						alignas(4) uint32_t taskThreadCount = 0u;
						alignas(4) uint32_t boneCount = 0u;
						alignas(4) uint32_t vertexBufferIndex = 0u;
						alignas(4) uint32_t boneWeightIndex = 0u;
						alignas(4) uint32_t weightStartIdIndex = 0u;
						alignas(4) uint32_t bonePoseOffsetIndex = 0u;
						alignas(4) uint32_t resultBufferIndex = 0u;
					};
					inline Kernel() : GraphicsSimulation::Kernel(sizeof(SimulationTaskSettings)) {}
					static const Kernel* Instance() {
						static const Kernel instance;
						return &instance;
					}
					inline virtual Reference<GraphicsSimulation::KernelInstance> CreateInstance(SceneContext* context)const override {
						static const Graphics::ShaderClass COMBINED_DEFORM_KERNEL_SHADER_CLASS(
							"Jimara/Components/GraphicsObjects/SkinnedMeshRenderer_CombinedDeformation");
						return CombinedGraphicsSimulationKernel<SimulationTaskSettings>::Create(context, &COMBINED_DEFORM_KERNEL_SHADER_CLASS, {});
					}
				};

				uint32_t boneCount = 0u;
				BindlessBinding vertexBuffer;
				BindlessBinding boneWeights;
				BindlessBinding weightStart;
				BindlessBinding bonePoseOffset;
				BindlessBinding resultBuffer;

			public:
				inline CombinedDeformationTask(SceneContext* context) 
					: GraphicsSimulation::Task(Kernel::Instance(), context) {};

				inline void Clear() {
					Kernel::SimulationTaskSettings settings = {};
					SetSettings(settings);
				};

				inline void Flush(SkinnedMeshRenderPipelineDescriptor* owner) {
					bool hasNullEntries = false;
					auto setBinding = [&](BindlessBinding& binding, Graphics::ArrayBuffer* buffer, uint32_t& index, const char* name) {
						owner->SetBindlessBinding(binding, buffer, index, hasNullEntries, [&]() {
							Context()->Log()->Error(
								"SkinnedMeshRenderPipelineDescriptor::CombinedDeformationTask::Flush - ",
								"Failed to get binding for '", name, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							});
					};
					Kernel::SimulationTaskSettings settings = {};
					settings.boneCount = static_cast<uint32_t>(owner->m_boneInverseReferencePoses.size() + 1u);
					setBinding(vertexBuffer, owner->m_meshVertices, settings.vertexBufferIndex, "vertexBuffer");
					setBinding(boneWeights, owner->m_boneWeights, settings.boneWeightIndex, "boneWeights");
					setBinding(weightStart, owner->m_boneWeightStartIds, settings.weightStartIdIndex, "weightStart");
					bonePoseOffset = owner->m_cachedBoneOffsets[owner->m_boneOffsetIndex];
					if (bonePoseOffset != nullptr) settings.bonePoseOffsetIndex = bonePoseOffset->Index();
					else hasNullEntries = true;
					setBinding(resultBuffer, owner->m_deformedVertexBinding->BoundObject(), settings.resultBufferIndex, "resultBuffer");
					settings.taskThreadCount = hasNullEntries ? 0u : static_cast<uint32_t>(resultBuffer->BoundObject()->ObjectCount());
					SetSettings(settings);
				}
			};
			const Reference<CombinedDeformationTask> m_combinedDeformationTask;
			GraphicsSimulation::TaskBinding m_activeDeformationTask;
			size_t m_activeDeformationTaskSleepCounter = 0u;

			class CombinedIndexGenerationTask : public virtual GraphicsSimulation::Task {
			private:
				class Kernel : public virtual GraphicsSimulation::Kernel {
				public:
					/// <summary> Settings per batch </summary>
					struct SimulationTaskSettings {
						alignas(4) uint32_t taskThreadCount = 0u;
						alignas(4) uint32_t vertexCount = 0u;
						alignas(4) uint32_t meshId = 0u;
						alignas(4) uint32_t deformedId = 0u;
					};
					inline Kernel() : GraphicsSimulation::Kernel(sizeof(SimulationTaskSettings)) {}
					static const Kernel* Instance() {
						static const Kernel instance;
						return &instance;
					}
					inline virtual Reference<GraphicsSimulation::KernelInstance> CreateInstance(SceneContext* context)const override {
						static const Graphics::ShaderClass COMBINED_DEFORM_KERNEL_SHADER_CLASS(
							"Jimara/Components/GraphicsObjects/SkinnedMeshRenderer_CombinedIndexGeneration");
						return CombinedGraphicsSimulationKernel<SimulationTaskSettings>::Create(context, &COMBINED_DEFORM_KERNEL_SHADER_CLASS, {});
					}
				};

				using Binding = Reference<const Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding>;
				Binding meshId;
				Binding deformedId;

			public:
				inline CombinedIndexGenerationTask(SceneContext* context)
					: GraphicsSimulation::Task(Kernel::Instance(), context) {};
				
				inline void Clear() {
					Kernel::SimulationTaskSettings settings = {};
					SetSettings(settings);
				};

				inline void Flush(SkinnedMeshRenderPipelineDescriptor* owner) {
					bool hasNullEntries = false;
					auto setBinding = [&](BindlessBinding& binding, Graphics::ArrayBuffer* buffer, uint32_t& index, const char* name) {
						owner->SetBindlessBinding(binding, buffer, index, hasNullEntries, [&]() {
							Context()->Log()->Error(
								"SkinnedMeshRenderPipelineDescriptor::CombinedIndexGenerationTask::Flush - ",
								"Failed to get binding for '", name, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							});
					};
					Kernel::SimulationTaskSettings settings = {};
					settings.vertexCount = (owner->m_meshVertices == nullptr) ? 0u : static_cast<uint32_t>(owner->m_meshVertices->ObjectCount());
					setBinding(meshId, owner->m_meshIndices, settings.meshId, "meshId");
					setBinding(deformedId, owner->m_deformedIndexBinding->BoundObject(), settings.deformedId, "deformedId");
					settings.taskThreadCount = hasNullEntries ? 0u : static_cast<uint32_t>(deformedId->BoundObject()->ObjectCount());
					SetSettings(settings);
				}
			};
			const Reference<CombinedIndexGenerationTask> m_combinedIndexgeneratorTask;
			GraphicsSimulation::TaskBinding m_activeIndexGenerationTask;
			size_t m_activeIndexGenerationTaskSleepCounter = 0u;

			inline void WakeTasks() {
				if (m_activeIndexGenerationTask == nullptr)
					m_activeIndexGenerationTask = m_combinedIndexgeneratorTask;
				if (m_activeDeformationTask == nullptr)
					m_activeDeformationTask = m_combinedDeformationTask;
				m_activeDeformationTaskSleepCounter = m_activeIndexGenerationTaskSleepCounter = 
					m_desc.context->Graphics()->Configuration().MaxInFlightCommandBufferCount();
			}
			
			const Reference<Graphics::GraphicsMesh> m_graphicsMesh;
			Graphics::ArrayBufferReference<MeshVertex> m_meshVertices;
			Graphics::ArrayBufferReference<uint32_t> m_meshIndices;
			Graphics::ArrayBufferReference<SkinnedTriMesh::BoneWeight> m_boneWeights;
			Graphics::ArrayBufferReference<uint32_t> m_boneWeightStartIds;
			std::vector<Matrix4> m_boneInverseReferencePoses;
			std::atomic<bool> m_meshDirty = true;

			inline void OnMeshDirty(Graphics::GraphicsMesh*) { 
				m_meshDirty = true;
				WakeTasks();
			}

			void UpdateMeshBuffers() {
				if (!m_meshDirty) return;
				const SkinnedTriMesh* mesh = dynamic_cast<const SkinnedTriMesh*>(m_desc.mesh.operator->());
				if (mesh != nullptr) {
					SkinnedTriMesh::Reader reader(mesh);
					m_graphicsMesh->GetBuffers(m_meshVertices, m_meshIndices);
					{
						m_boneInverseReferencePoses.resize(reader.BoneCount());
						for (size_t i = 0; i < m_boneInverseReferencePoses.size(); i++)
							m_boneInverseReferencePoses[i] = Math::Inverse(reader.BoneData(static_cast<uint32_t>(i)));
					}
					uint32_t lastBoneWeightId = 0;
					{
						m_boneWeightStartIds = m_desc.context->Graphics()->Device()->CreateArrayBuffer<uint32_t>(static_cast<size_t>(reader.VertCount()) + 1);
						uint32_t* boneWeightStartIds = m_boneWeightStartIds.Map();
						for (uint32_t i = 0; i < reader.VertCount(); i++) {
							boneWeightStartIds[i] = lastBoneWeightId;
							lastBoneWeightId += max(reader.WeightCount(i), uint32_t(1));
						}
						boneWeightStartIds[reader.VertCount()] = lastBoneWeightId;
						m_boneWeightStartIds->Unmap(true);
					}
					{
						m_boneWeights = m_desc.context->Graphics()->Device()->CreateArrayBuffer<SkinnedTriMesh::BoneWeight>(lastBoneWeightId);
						SkinnedTriMesh::BoneWeight* boneWeights = m_boneWeights.Map();
						lastBoneWeightId = 0;
						for (uint32_t i = 0; i < reader.VertCount(); i++) {
							const uint32_t weightCount = reader.WeightCount(i);
							if (weightCount > 0) {
								float totalMass = 0.0f;
								for (uint32_t j = 0; j < weightCount; j++) totalMass += reader.Weight(i, j).boneWeight;
								const float multiplier = (totalMass <= std::numeric_limits<float>::epsilon()) ? 1.0f : (1.0f / totalMass);
								for (uint32_t j = 0; j < weightCount; j++) {
									SkinnedTriMesh::BoneWeight boneWeight = reader.Weight(i, j);
									boneWeight.boneWeight *= multiplier;
									boneWeights[lastBoneWeightId] = boneWeight;
									lastBoneWeightId++;
								}
							}
							else {
								boneWeights[lastBoneWeightId] = SkinnedTriMesh::BoneWeight(reader.BoneCount(), 1.0f);
								lastBoneWeightId++;
							}
						}
						m_boneWeights->Unmap(true);
					}
				}
				else {
					TriMesh::Reader reader(m_desc.mesh);
					m_graphicsMesh->GetBuffers(m_meshVertices, m_meshIndices);
					m_boneInverseReferencePoses.clear();
					m_boneWeightStartIds = m_desc.context->Graphics()->Device()->CreateArrayBuffer<uint32_t>(static_cast<size_t>(reader.VertCount()) + 1);
					m_boneWeights = m_desc.context->Graphics()->Device()->CreateArrayBuffer<SkinnedTriMesh::BoneWeight>(reader.VertCount());
					uint32_t* boneWeightStartIds = m_boneWeightStartIds.Map();
					SkinnedTriMesh::BoneWeight* boneWeights = m_boneWeights.Map();
					for (uint32_t i = 0; i <= reader.VertCount(); i++) {
						boneWeightStartIds[i] = i;
						boneWeights[i] = SkinnedTriMesh::BoneWeight(0, 1.0f);
					}
					m_boneWeightStartIds->Unmap(true);
					m_boneWeights->Unmap(true);
				}

				m_renderersDirty = true;
				m_meshDirty = false;
			}

			ObjectSet<SkinnedMeshRenderer> m_renderers;
			std::vector<Reference<Component>> m_components;
			std::vector<Matrix4> m_currentOffsets;
			std::vector<Matrix4> m_lastOffsets;
			Stacktor<BindlessBinding, 4u> m_cachedBoneOffsets;
			size_t m_boneOffsetIndex = 0u;

			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_deformedVertexBinding = 
				Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_deformedIndexBinding =
				Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_instanceBufferBinding =
				Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
			bool m_renderersDirty = true;

			struct KeepComponentsAlive : public virtual Object {
				std::vector<Reference<Component>> components;
				inline void Clear(Object*) { components.clear(); }
			};
			const Reference<KeepComponentsAlive> m_keepComponentsAlive = Object::Instantiate<KeepComponentsAlive>();

			void RecalculateDeformedBuffer() {
				// Disable kernels:
				const bool stuffNotDirty = (!m_renderersDirty) && (!m_meshDirty);
				if (stuffNotDirty) {
					if (m_activeDeformationTask != nullptr) {
						if (m_activeDeformationTaskSleepCounter <= 0u) {
							m_combinedDeformationTask->Clear();
							if (m_combinedIndexgeneratorTask != nullptr)
								m_combinedIndexgeneratorTask->Clear();
							m_activeDeformationTask = nullptr;
						}
						else m_activeDeformationTaskSleepCounter--;
					}
					if (m_activeIndexGenerationTask != nullptr) {
						if (m_activeIndexGenerationTaskSleepCounter <= 0u) {
							m_combinedIndexgeneratorTask->Clear();
							m_activeIndexGenerationTask = nullptr;
						}
						else m_activeIndexGenerationTaskSleepCounter--;
					}
				}

				// Update deformation and index kernel inputs:
				if (m_renderersDirty) {
					// This shall prevent components from going out of scope prematurely:
					{
						for (size_t i = 0; i < m_components.size(); i++)
							m_keepComponentsAlive->components.push_back(m_components[i]);
						m_desc.context->ExecuteAfterUpdate(Callback(&KeepComponentsAlive::Clear, m_keepComponentsAlive.operator->()), m_keepComponentsAlive);
					}

					m_components.clear();
					for (size_t i = 0; i < m_renderers.Size(); i++)
						m_components.push_back(m_renderers[i]);

					m_deformedVertexBinding->BoundObject() = m_desc.context->Graphics()->Device()->CreateArrayBuffer<SkinnedMeshVertex>(
						m_meshVertices->ObjectCount() * m_renderers.Size());

					if (m_renderers.Size() > 1) {
						m_deformedIndexBinding->BoundObject() = m_desc.context->Graphics()->Device()->CreateArrayBuffer<uint32_t>(
							m_meshIndices->ObjectCount() * m_renderers.Size());
						m_combinedIndexgeneratorTask->Flush(this);
						m_activeIndexGenerationTask = m_combinedIndexgeneratorTask;
						m_activeIndexGenerationTaskSleepCounter = m_desc.context->Graphics()->Configuration().MaxInFlightCommandBufferCount();
					}
					else m_deformedIndexBinding->BoundObject() = m_meshIndices;

					m_lastOffsets.clear();
					m_renderersDirty = false;
				}
				else if (m_desc.isStatic) return;

				// Extract current bone offsets:
				const size_t offsetCount = m_renderers.Size() * (m_boneInverseReferencePoses.size() + 1);
				if (m_currentOffsets.size() != offsetCount) m_currentOffsets.resize(offsetCount);
				for (size_t rendererId = 0; rendererId < m_renderers.Size(); rendererId++) {
					const SkinnedMeshRenderer* renderer = m_renderers[rendererId];
					const Transform* rendererTransform = renderer->GetTransfrom();
					const Transform* rootBoneTransform = renderer->SkeletonRoot();
					const size_t bonePtr = (m_boneInverseReferencePoses.size() + 1) * rendererId;
					const Matrix4 rendererPose = (rendererTransform != nullptr) ? rendererTransform->WorldMatrix() : Math::Identity();
					if (rootBoneTransform == nullptr) {
						for (size_t boneId = 0; boneId < m_boneInverseReferencePoses.size(); boneId++) {
							Matrix4& boneOffset = m_currentOffsets[bonePtr + boneId];
							const Transform* boneTransform = renderer->Bone(boneId);
							if (boneTransform == nullptr) boneOffset = Math::Identity();
							else boneOffset = boneTransform->WorldMatrix() * m_boneInverseReferencePoses[boneId];
						}
						m_currentOffsets[bonePtr + m_boneInverseReferencePoses.size()] = Math::Identity();
					}
					else {
						const Matrix4 inverseRootPose = Math::Inverse(rootBoneTransform->WorldMatrix());
						for (size_t boneId = 0; boneId < m_boneInverseReferencePoses.size(); boneId++) {
							Matrix4& boneOffset = m_currentOffsets[bonePtr + boneId];
							boneOffset = rendererPose;
							const Transform* boneTransform = renderer->Bone(boneId);
							if (boneTransform == nullptr) continue;
							boneOffset *= inverseRootPose * boneTransform->WorldMatrix() * m_boneInverseReferencePoses[boneId];
						}
						m_currentOffsets[bonePtr + m_boneInverseReferencePoses.size()] = rendererPose;
					}
				}

				// Check if offsets are dirty:
				if (m_lastOffsets.size() == offsetCount) {
					bool dirty = false;
					for (size_t i = 0; i < offsetCount; i++)
						if (m_currentOffsets[i] != m_lastOffsets[i]) {
							dirty = true;
							for (size_t j = i; j < offsetCount; j++)
								m_lastOffsets[j] = m_currentOffsets[j];
							break;
						}
					if ((!dirty) && stuffNotDirty) 
						return;
				}
				else m_lastOffsets = m_currentOffsets;

				// Update offsets buffer:
				{
					m_boneOffsetIndex = (m_boneOffsetIndex + 1u) % m_desc.context->Graphics()->Configuration().MaxInFlightCommandBufferCount();
					if (m_cachedBoneOffsets.Size() <= m_boneOffsetIndex)
						m_cachedBoneOffsets.Resize(m_boneOffsetIndex + 1u);
					BindlessBinding& cachedBinding = m_cachedBoneOffsets[m_boneOffsetIndex];
					if (cachedBinding != nullptr && cachedBinding->BoundObject()->ObjectCount() < m_currentOffsets.size())
						cachedBinding = nullptr;
					if (cachedBinding == nullptr) {
						const Reference<Graphics::ArrayBuffer> newBuffer = m_desc.context->Graphics()->Device()->CreateArrayBuffer<Matrix4>(
							m_currentOffsets.size(), Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
						if (newBuffer != nullptr) {
							cachedBinding = m_desc.context->Graphics()->Bindless().Buffers()->GetBinding(newBuffer);
							if (cachedBinding == nullptr)
								m_desc.context->Log()->Error(
									"SkinnedMeshRenderPipelineDescriptor::RecalculateDeformedBuffer - ",
									"Failed to bind offset buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						}
					}

					Graphics::ArrayBuffer* const boneOffsets = (cachedBinding == nullptr) ? nullptr : cachedBinding->BoundObject();
					if (boneOffsets != nullptr) {
						memcpy((void*)boneOffsets->Map(), (void*)m_currentOffsets.data(), sizeof(Matrix4) * m_currentOffsets.size());
						boneOffsets->Unmap(true);
					}
					else m_desc.context->Log()->Error(
						"SkinnedMeshRenderPipelineDescriptor::RecalculateDeformedBuffer - ",
						"Failed to reallocate offset buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}

				// Register deformation task:
				m_combinedDeformationTask->Flush(this);
				if (m_combinedIndexgeneratorTask != nullptr)
					m_combinedIndexgeneratorTask->Flush(this);
				if (m_activeDeformationTask == nullptr)
					m_activeDeformationTask = m_combinedDeformationTask;
				m_activeDeformationTaskSleepCounter = m_desc.context->Graphics()->Configuration().MaxInFlightCommandBufferCount();
			}


		public:
			inline SkinnedMeshRenderPipelineDescriptor(const TriMeshRenderer::Configuration& desc, bool isInstanced)
				: GraphicsObjectDescriptor(desc.layer)
				, GraphicsObjectDescriptor::ViewportData(
					desc.context, desc.material->Shader(), desc.geometryType)
				, m_desc(desc)
				, m_graphicsObjectSet(GraphicsObjectDescriptor::Set::GetInstance(desc.context))
				, m_cachedMaterialInstance(desc.material)
				, m_combinedDeformationTask(Object::Instantiate<CombinedDeformationTask>(desc.context))
				, m_combinedIndexgeneratorTask(isInstanced ? Object::Instantiate<CombinedIndexGenerationTask>(desc.context) : nullptr)
				, m_graphicsMesh(Graphics::GraphicsMesh::Cached(desc.context->Graphics()->Device(), desc.mesh, desc.geometryType)) {
				OnMeshDirty(nullptr);
				WakeTasks();
				m_graphicsMesh->OnInvalidate() += Callback(&SkinnedMeshRenderPipelineDescriptor::OnMeshDirty, this);
			}

			inline virtual ~SkinnedMeshRenderPipelineDescriptor() {
				m_graphicsMesh->OnInvalidate() -= Callback(&SkinnedMeshRenderPipelineDescriptor::OnMeshDirty, this);
				m_activeDeformationTask = nullptr;
				m_activeIndexGenerationTask = nullptr;
			}

			const TriMeshRenderer::Configuration& BatchDescriptor()const { return m_desc; }

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
					vertexInfo.layout.bufferElementSize = sizeof(SkinnedMeshVertex);
					vertexInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
						StandardLitShaderInputs::JM_VertexPosition_Location, offsetof(SkinnedMeshVertex, position)));
					vertexInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
						StandardLitShaderInputs::JM_VertexNormal_Location, offsetof(SkinnedMeshVertex, normal)));
					vertexInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
						StandardLitShaderInputs::JM_VertexUV_Location, offsetof(SkinnedMeshVertex, uv)));
					vertexInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
						StandardLitShaderInputs::JM_ObjectIndex_Location, offsetof(SkinnedMeshVertex, objectIndex)));
					vertexInfo.binding = m_deformedVertexBinding;
				}
				{
					GraphicsObjectDescriptor::VertexBufferInfo& instanceInfo = info.vertexBuffers[1u];
					instanceInfo.layout.inputRate = Graphics::GraphicsPipeline::VertexInputInfo::InputRate::INSTANCE;
					instanceInfo.layout.bufferElementSize = sizeof(Matrix4);
					instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
						StandardLitShaderInputs::JM_ObjectTransform_Location, 0u));
					instanceInfo.binding = m_instanceBufferBinding;
				}
				info.indexBuffer = m_deformedIndexBinding;
				return info;
			}
			inline virtual size_t IndexCount()const override { return m_deformedIndexBinding->BoundObject()->ObjectCount(); }
			inline virtual size_t InstanceCount()const override { return 1; }
			inline virtual Reference<Component> GetComponent(size_t, size_t primitiveId)const override {
				if (m_meshIndices == nullptr) return nullptr;
				size_t baseIndex = primitiveId * 3;
				size_t componentId = baseIndex / m_meshIndices->ObjectCount();
				if (componentId < m_components.size()) return m_components[componentId];
				else return nullptr;
			}


		protected:
			/** JobSystem::Job: */
			virtual void CollectDependencies(Callback<Job*>)override {}

			virtual void Execute()final override {
				std::unique_lock<std::mutex> lock(m_lock);
				UpdateMeshBuffers();
				RecalculateDeformedBuffer();
				if (m_instanceBufferBinding->BoundObject() == nullptr) {
					const Graphics::ArrayBufferReference<Matrix4> buffer = m_desc.context->Graphics()->Device()->CreateArrayBuffer<Matrix4>(1);
					m_instanceBufferBinding->BoundObject() = buffer;
					(*buffer.Map()) = Math::Identity();
					buffer->Unmap(true);
				}
				m_cachedMaterialInstance.Update();
			}
		public:


			/** Writer */
			class Writer : public virtual std::unique_lock<std::mutex> {
			private:
				SkinnedMeshRenderPipelineDescriptor* const m_desc;

			public:
				inline Writer(SkinnedMeshRenderPipelineDescriptor* desc) : std::unique_lock<std::mutex>(desc->m_lock), m_desc(desc) {}

				void AddTransform(SkinnedMeshRenderer* renderer) {
					if (renderer == nullptr) return;
					if (m_desc->m_renderers.Size() == 0) {
						if (m_desc->m_owner != nullptr)
							m_desc->m_desc.context->Log()->Fatal(
								"SkinnedMeshRenderPipelineDescriptor::Writer::AddTransform - m_owner expected to be nullptr! [File: '", __FILE__, "'; Line: ", __LINE__);
						Reference<GraphicsObjectDescriptor::Set::ItemOwner> owner = Object::Instantiate<GraphicsObjectDescriptor::Set::ItemOwner>(m_desc);
						m_desc->m_owner = owner;
						m_desc->m_graphicsObjectSet->Add(owner);
						m_desc->m_desc.context->Graphics()->SynchPointJobs().Add(m_desc);
					}
					m_desc->m_renderers.Add(renderer);
					m_desc->m_renderersDirty = true;
					m_desc->WakeTasks();
				}

				void RemoveTransform(SkinnedMeshRenderer* renderer) {
					if (renderer == nullptr) return;
					m_desc->m_renderers.Remove(renderer);
					if (m_desc->m_renderers.Size() == 0) {
						if (m_desc->m_owner == nullptr)
							m_desc->m_desc.context->Log()->Fatal(
								"SkinnedMeshRenderPipelineDescriptor::Writer::RemoveTransform - m_owner expected to be non-nullptr! [File: '", __FILE__, "'; Line: ", __LINE__);
						m_desc->m_graphicsObjectSet->Remove(m_desc->m_owner);
						m_desc->m_owner = nullptr;
						m_desc->m_desc.context->Graphics()->SynchPointJobs().Remove(m_desc);
					}
					m_desc->m_renderersDirty = true;
					m_desc->WakeTasks();
				}
			};

			/** Instancer: */
			class Instancer : ObjectCache<TriMeshRenderer::Configuration> {
			public:
				inline static Reference<SkinnedMeshRenderPipelineDescriptor> GetDescriptor(const TriMeshRenderer::Configuration& desc) {
					static Instancer instance;
					return instance.GetCachedOrCreate(desc, false,
						[&]() -> Reference<SkinnedMeshRenderPipelineDescriptor> { return Object::Instantiate<SkinnedMeshRenderPipelineDescriptor>(desc, true); });
				}
			};
		};
#pragma warning(default: 4250)
	}

	SkinnedMeshRenderer::SkinnedMeshRenderer(Component* parent, const std::string_view& name,
		TriMesh* mesh, Jimara::Material* material, bool instanced, bool isStatic,
		Transform* const* bones, size_t boneCount, Transform* skeletonRoot)
		: Component(parent, name) {
		bool wasEnabled = Enabled();
		SetEnabled(false);
		SetSkeletonRoot(skeletonRoot);
		for (size_t i = 0; i < boneCount; i++)
			SetBone(i, bones[i]);
		MarkStatic(isStatic);
		RenderInstanced(instanced);
		SetMesh(mesh);
		SetMaterial(material);
		SetEnabled(wasEnabled);
	}

	SkinnedMeshRenderer::SkinnedMeshRenderer(Component* parent, const std::string_view& name,
		TriMesh* mesh, Jimara::Material* material, bool instanced, bool isStatic,
		const Reference<Transform>* bones, size_t boneCount, Transform* skeletonRoot)
		: SkinnedMeshRenderer(parent, name, mesh, material, instanced, skeletonRoot) {
		SetSkeletonRoot(skeletonRoot);
		for (size_t i = 0; i < boneCount; i++)
			SetBone(i, bones[i]);
	}

	SkinnedMeshRenderer::~SkinnedMeshRenderer() { SetSkeletonRoot(nullptr); }

	Transform* SkinnedMeshRenderer::SkeletonRoot()const { return m_skeletonRoot; }

	void SkinnedMeshRenderer::SetSkeletonRoot(Transform* skeletonRoot) {
		if (m_skeletonRoot == skeletonRoot) return;
		const Callback<Component*> onDestroyedCallback(&SkinnedMeshRenderer::OnSkeletonRootDestroyed, this);
		if (m_skeletonRoot != nullptr) m_skeletonRoot->OnDestroyed() -= onDestroyedCallback;
		m_skeletonRoot = skeletonRoot;
		if (m_skeletonRoot != nullptr) m_skeletonRoot->OnDestroyed() += onDestroyedCallback;
	}

	size_t SkinnedMeshRenderer::BoneCount()const { return m_boneCount; }

	Transform* SkinnedMeshRenderer::Bone(size_t index)const { return (index < m_boneCount) ? m_bones[index]->Bone() : nullptr; }

	void SkinnedMeshRenderer::SetBone(size_t index, Transform* bone) {
		while (index >= m_boneCount) {
			if (m_bones.size() <= index) m_bones.push_back(Object::Instantiate<BoneBinding>());
			m_boneCount++;
		}
		m_bones[index]->SetBone(bone);
		if (m_boneCount >= m_bones.size()) m_boneCount = m_bones.size() - 1;
		while (m_boneCount > 0 && m_boneCount < m_bones.size() && m_bones[m_boneCount]->Bone() == nullptr) m_boneCount--;
		if (m_boneCount >= 0 && m_boneCount < m_bones.size() && m_bones[m_boneCount]->Bone() != nullptr) m_boneCount++;
	}

	void SkinnedMeshRenderer::OnTriMeshRendererDirty() {
		const TriMeshRenderer::Configuration batchDesc(this);
		if (m_pipelineDescriptor != nullptr) {
			SkinnedMeshRenderPipelineDescriptor* descriptor = dynamic_cast<SkinnedMeshRenderPipelineDescriptor*>(m_pipelineDescriptor.operator->());
			SkinnedMeshRenderPipelineDescriptor::Writer(descriptor).RemoveTransform(this);
			m_pipelineDescriptor = nullptr;
		}
		if (ActiveInHeirarchy() && batchDesc.mesh != nullptr && batchDesc.material != nullptr) {
			Reference<SkinnedMeshRenderPipelineDescriptor> descriptor;
			if (IsInstanced()) descriptor = SkinnedMeshRenderPipelineDescriptor::Instancer::GetDescriptor(batchDesc);
			else descriptor = Object::Instantiate<SkinnedMeshRenderPipelineDescriptor>(batchDesc, false);
			SkinnedMeshRenderPipelineDescriptor::Writer(descriptor).AddTransform(this);
			m_pipelineDescriptor = descriptor;
		}
	}

	void SkinnedMeshRenderer::OnSkeletonRootDestroyed(Component*) { SetSkeletonRoot(nullptr); }



	SkinnedMeshRenderer::BoneBinding::~BoneBinding() { SetBone(nullptr); }

	Transform* SkinnedMeshRenderer::BoneBinding::Bone()const { return m_bone; }

	void SkinnedMeshRenderer::BoneBinding::SetBone(Transform* bone) {
		if (m_bone == bone) return;
		const Callback<Component*> onDestroyedCallback(&SkinnedMeshRenderer::BoneBinding::BoneDestroyed, this);
		if (m_bone != nullptr) m_bone->OnDestroyed() -= onDestroyedCallback;
		m_bone = bone;
		if (m_bone != nullptr) m_bone->OnDestroyed() += onDestroyedCallback;
	}

	void SkinnedMeshRenderer::BoneBinding::BoneDestroyed(Component*) { SetBone(nullptr); }

	class SkinnedMeshRenderer::BoneCollectionSerializer : public virtual Serialization::SerializerList::From<SkinnedMeshRenderer> {
	public:
		inline BoneCollectionSerializer() : ItemSerializer("Bones", "Deformation bone transforms") {}

		inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, SkinnedMeshRenderer* target)const final override {
			{
				static const auto serializer = Serialization::ValueSerializer<uint32_t>::For<SkinnedMeshRenderer>(
					"Count", "Number of deformation bones (not the same as SkinnedMeshRenderer::BoneCount())",
					[](SkinnedMeshRenderer* renderer) -> uint32_t { return static_cast<uint32_t>(renderer->m_bones.size()); },
					[](const uint32_t& count, SkinnedMeshRenderer* renderer) {
						while (static_cast<size_t>(count) > renderer->m_bones.size())
							renderer->m_bones.push_back(Object::Instantiate<BoneBinding>());
						renderer->m_bones.resize(static_cast<size_t>(count));
						if (renderer->m_boneCount > renderer->m_bones.size())
							renderer->m_boneCount = renderer->m_bones.size();
					});
				recordElement(serializer->Serialize(target));
			}
			{
				Transform* (*getRoot)(BoneBinding*) = [](BoneBinding* binding) { return binding->Bone(); };
				void (*setRoot)(Transform* const&, BoneBinding*) = [](Transform* const& bone, BoneBinding* binding) { binding->SetBone(bone); };
				static const Reference<const Serialization::ItemSerializer::Of<BoneBinding>> serializer = Serialization::ValueSerializer<Transform*>::Create<BoneBinding>(
					"", "Deformation bone", Function<Transform*, BoneBinding*>(getRoot), Callback<Transform* const&, BoneBinding*>(setRoot));
				for (size_t i = 0; i < target->m_bones.size(); i++)
					recordElement(serializer->Serialize(target->m_bones[i]));
				size_t minFilled = 0;
				for (size_t i = 0; i < target->m_bones.size(); i++)
					if (target->m_bones[i]->Bone() != nullptr)
						minFilled = i;
				target->m_boneCount = Math::Min(target->m_bones.size(), minFilled + 1u);
			}
		}
	};

	void SkinnedMeshRenderer::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		TriMeshRenderer::GetFields(recordElement);
		{
			Transform* (*getRoot)(SkinnedMeshRenderer*) = [](SkinnedMeshRenderer* renderer) { return renderer->SkeletonRoot(); };
			void (*setRoot)(Transform* const&, SkinnedMeshRenderer*) = [](Transform* const& root, SkinnedMeshRenderer* renderer) { renderer->SetSkeletonRoot(root); };
			static const auto serializer = Serialization::ValueSerializer<Transform*>::Create<SkinnedMeshRenderer>(
				"Skeleton Root",
				"This is optional and mostly useful if one intends to reuse bones and place many instances of the same skinned mesh at multiple places and same pose",
				Function<Transform*, SkinnedMeshRenderer*>(getRoot), Callback<Transform* const&, SkinnedMeshRenderer*>(setRoot));
			recordElement(serializer->Serialize(this));
		}
		{
			static const BoneCollectionSerializer serializer;
			recordElement(serializer.Serialize(this));
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<SkinnedMeshRenderer>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<SkinnedMeshRenderer>(
			"Skinned Mesh Renderer", "Jimara/Graphics/SkinnedMeshRenderer", "Skinned mesh renderer that can deform based on bone positions");
		report(factory);
	}
}
