#include "SkinnedMeshRenderer.h"
#include "../../Core/Collections/ObjectSet.h"
#include "../../Data/Geometry/GraphicsMesh.h"
#include "../../Data/Materials/StandardLitShaderInputs.h"
#include "../../Environment/Rendering/Culling/FrustrumAABB/FrustrumAABBCulling.h"
#include "../../Environment/GraphicsSimulation/CombinedGraphicsSimulationKernel.h"
#include "../../Environment/Rendering/SceneObjects/Objects/GraphicsObjectDescriptor.h"


namespace Jimara {
	struct SkinnedMeshRenderer::Helpers {

		struct InstanceBoundaryData {
			Matrix4 transform = Math::Identity();
			AABB localBounds = AABB(Vector3(0.0f), Vector3(0.0f));
			float minOnScreenSize = 0.0f;
			float maxOnScreenSize = -1.0f;
		};

		struct SkinnedMeshVertex {
			alignas(16) Vector3 position = {};
			alignas(16) Vector3 normal = {};
			alignas(16) Vector2 uv = {};
			alignas(4) uint32_t objectIndex = 0;
		};
		static_assert(sizeof(MeshVertex) == sizeof(SkinnedMeshVertex));

		struct SkinnedMeshInstanceData {
			alignas(16) Matrix4 transform = Math::Identity();
			alignas(16) Vector4 vertexColor = Vector4(1.0f);
			alignas(16) Vector4 tilingAndOffset = Vector4(1.0f, 1.0f, 0.0f, 0.0f);
		};

		class SkinnedMeshRendererViewportData;

#pragma warning(disable: 4250)
		class SkinnedMeshRenderPipelineDescriptor
			: public virtual ObjectCache<TriMeshRenderer::Configuration>::StoredObject
			, public virtual ObjectCache<Reference<const Object>>
			, public virtual GraphicsObjectDescriptor
			, public virtual JobSystem::Job {
		private:
			friend class SkinnedMeshRendererViewportData;

			const TriMeshRenderer::Configuration m_desc;
			const Reference<GraphicsObjectDescriptor::Set> m_graphicsObjectSet;
			// __TODO__: This is not fully safe... stores self-reference; some refactor down the line would be adviced
			Reference<GraphicsObjectDescriptor::Set::ItemOwner> m_owner = nullptr;
			const Reference<Material::CachedInstance> m_cachedMaterialInstance;
			std::mutex m_lock;

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
						static const constexpr std::string_view COMBINED_DEFORM_KERNEL_SHADER_PATH(
							"Jimara/Components/GraphicsObjects/SkinnedMeshRenderer_CombinedDeformation.comp");
						return CombinedGraphicsSimulationKernel<SimulationTaskSettings>::Create(context, COMBINED_DEFORM_KERNEL_SHADER_PATH, {});
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
						static const constexpr std::string_view COMBINED_DEFORM_KERNEL_SHADER_PATH(
							"Jimara/Components/GraphicsObjects/SkinnedMeshRenderer_CombinedIndexGeneration.comp");
						return CombinedGraphicsSimulationKernel<SimulationTaskSettings>::Create(context, COMBINED_DEFORM_KERNEL_SHADER_PATH, {});
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

			bool UpdateMeshBuffers() {
				if (!m_meshDirty) 
					return false;
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
				return true;
			}

			ObjectSet<SkinnedMeshRenderer> m_renderers;
			std::vector<Reference<SkinnedMeshRenderer>> m_components;
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

			Stacktor<InstanceBoundaryData, 4u> m_instanceBoundaries;
			AABB m_combinedBoundaries = AABB(Vector3(0.0f), Vector3(0.0f));

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
				else if ((m_desc.flags & TriMeshRenderer::Flags::STATIC) != TriMeshRenderer::Flags::NONE) return;

				// Extract current bone offsets:
				const size_t offsetCount = m_renderers.Size() * (m_boneInverseReferencePoses.size() + 1);
				if (m_currentOffsets.size() != offsetCount) m_currentOffsets.resize(offsetCount);
				for (size_t rendererId = 0; rendererId < m_renderers.Size(); rendererId++) {
					const SkinnedMeshRenderer* renderer = m_renderers[rendererId];
					const Transform* rendererTransform = renderer->GetTransform();
					const Transform* rootBoneTransform = renderer->SkeletonRoot();
					const size_t bonePtr = (m_boneInverseReferencePoses.size() + 1) * rendererId;
					const Matrix4 rendererPose = (rendererTransform != nullptr) 
						? rendererTransform->FrameCachedWorldMatrix() : Math::Identity();
					if (rootBoneTransform == nullptr) {
						for (size_t boneId = 0; boneId < m_boneInverseReferencePoses.size(); boneId++) {
							Matrix4& boneOffset = m_currentOffsets[bonePtr + boneId];
							const Transform* boneTransform = renderer->Bone(boneId);
							if (boneTransform == nullptr) boneOffset = Math::Identity();
							else boneOffset = boneTransform->FrameCachedWorldMatrix() * m_boneInverseReferencePoses[boneId];
						}
						m_currentOffsets[bonePtr + m_boneInverseReferencePoses.size()] = Math::Identity();
					}
					else {
						const Matrix4 inverseRootPose = Math::Inverse(rootBoneTransform->FrameCachedWorldMatrix());
						for (size_t boneId = 0; boneId < m_boneInverseReferencePoses.size(); boneId++) {
							Matrix4& boneOffset = m_currentOffsets[bonePtr + boneId];
							boneOffset = rendererPose;
							const Transform* boneTransform = renderer->Bone(boneId);
							if (boneTransform == nullptr) continue;
							boneOffset *= inverseRootPose * boneTransform->FrameCachedWorldMatrix() * m_boneInverseReferencePoses[boneId];
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


			inline void UpdateInstanceBoundaryData() {
				bool transformsDirty = (
					((m_desc.flags & TriMeshRenderer::Flags::STATIC) == TriMeshRenderer::Flags::NONE) || 
					m_instanceBoundaries.Size() != m_components.size());
				m_instanceBoundaries.Resize(m_components.size());
				for (size_t i = 0u; i < m_instanceBoundaries.Size(); i++) {
					InstanceBoundaryData& boundaryData = m_instanceBoundaries[i];
					const SkinnedMeshRenderer* renderer = m_components[i];
					boundaryData.localBounds = renderer->GetLocalBoundaries();
					{
						const RendererCullingOptions& cullingOptions = renderer->CullingOptions();
						boundaryData.minOnScreenSize = 
							(cullingOptions.onScreenSizeRangeEnd < 0.0f) ? cullingOptions.onScreenSizeRangeStart 
							: Math::Min(cullingOptions.onScreenSizeRangeStart, cullingOptions.onScreenSizeRangeEnd);
						boundaryData.maxOnScreenSize =
							(cullingOptions.onScreenSizeRangeEnd < 0.0f) ? cullingOptions.onScreenSizeRangeEnd
							: Math::Max(cullingOptions.onScreenSizeRangeStart, cullingOptions.onScreenSizeRangeEnd);
					}
					if (transformsDirty) {
						const Transform* transform = renderer->GetTransform();
						boundaryData.transform = (transform == nullptr) ? Math::Identity() : transform->FrameCachedWorldMatrix();
					}
					const AABB worldBounds = boundaryData.transform * boundaryData.localBounds;
					if (i <= 0u)
						m_combinedBoundaries = worldBounds;
					else {
						m_combinedBoundaries.start.x = Math::Min(m_combinedBoundaries.start.x, worldBounds.start.x);
						m_combinedBoundaries.start.y = Math::Min(m_combinedBoundaries.start.y, worldBounds.start.y);
						m_combinedBoundaries.start.z = Math::Min(m_combinedBoundaries.start.z, worldBounds.start.z);
						m_combinedBoundaries.end.x = Math::Max(m_combinedBoundaries.end.x, worldBounds.end.x);
						m_combinedBoundaries.end.y = Math::Max(m_combinedBoundaries.end.y, worldBounds.end.y);
						m_combinedBoundaries.end.z = Math::Max(m_combinedBoundaries.end.z, worldBounds.end.z);
					}
				}
			}


		public:
			inline SkinnedMeshRenderPipelineDescriptor(const TriMeshRenderer::Configuration& desc, bool isInstanced)
				: GraphicsObjectDescriptor(desc.material->Shader(), desc.layer)
				, m_desc(desc)
				, m_graphicsObjectSet(GraphicsObjectDescriptor::Set::GetInstance(desc.context))
				, m_cachedMaterialInstance(desc.material->CreateCachedInstance())
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

			/** GraphicsObjectDescriptor */
			inline virtual Reference<const GraphicsObjectDescriptor::ViewportData> GetViewportData(const RendererFrustrumDescriptor* frustrum) override;


		protected:
			/** JobSystem::Job: */
			virtual void CollectDependencies(Callback<Job*>)override {}

			virtual void Execute()final override {
				std::unique_lock<std::mutex> lock(m_lock);
				if (m_renderersDirty)
					m_instanceBoundaries.Clear();
				UpdateMeshBuffers();
				RecalculateDeformedBuffer();
				if (m_instanceBufferBinding->BoundObject() == nullptr) {
					const Graphics::ArrayBufferReference<SkinnedMeshInstanceData> buffer =
						m_desc.context->Graphics()->Device()->CreateArrayBuffer<SkinnedMeshInstanceData>(1);
					m_instanceBufferBinding->BoundObject() = buffer;
					(*buffer.Map()) = SkinnedMeshInstanceData();
					buffer->Unmap(true);
				}
				UpdateInstanceBoundaryData();
				m_cachedMaterialInstance->Update();
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
					return instance.GetCachedOrCreate(desc,
						[&]() -> Reference<SkinnedMeshRenderPipelineDescriptor> { return Object::Instantiate<SkinnedMeshRenderPipelineDescriptor>(desc, true); });
				}
			};
		};
	};



	class SkinnedMeshRenderer::Helpers::SkinnedMeshRendererViewportData
		: public virtual GraphicsObjectDescriptor::ViewportData
		, public virtual ObjectCache<Reference<const Object>>::StoredObject {
	private:
		struct TaskSettings {
			alignas(4) uint32_t taskThreadCount = 0u;
			alignas(4) uint32_t vertexCount = 0u;
			alignas(4) uint32_t meshId = 0u;
			alignas(4) uint32_t deformedId = 0u;
			alignas(4) uint32_t baseObjectIndexId = 0u;
		};

		class SimulationKernelInstance : public virtual GraphicsSimulation::KernelInstance {
		private:
			const Reference<SceneContext> m_context;
			const Reference<GraphicsSimulation::KernelInstance> m_combinedKernel;
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_objectIndexBufferData;
			std::vector<uint32_t> m_objectIndexBuffer;

		public:
			inline SimulationKernelInstance(
				SceneContext* context, 
				GraphicsSimulation::KernelInstance* combinedKernel,
				Graphics::ResourceBinding<Graphics::ArrayBuffer>* objectIndexBufferData)
				: m_context(context)
				, m_combinedKernel(combinedKernel)
				, m_objectIndexBufferData(objectIndexBufferData){
				assert(m_context != nullptr);
				assert(m_combinedKernel != nullptr);
				assert(m_objectIndexBufferData != nullptr);
			}

			inline virtual ~SimulationKernelInstance() {}

			virtual void Execute(Graphics::InFlightBufferInfo commandBufferInfo, 
				const GraphicsSimulation::Task* const* tasks, size_t taskCount) final override {
				// Update all tasks:
				{
					m_objectIndexBuffer.clear();
					const GraphicsSimulation::Task* const* ptr = tasks;
					const GraphicsSimulation::Task* const* const end = tasks + taskCount;
					while (ptr < end) {
						const GraphicsSimulation::Task* task = *ptr;
						dynamic_cast<const SimulationTask*>(task)->Update(m_objectIndexBuffer, commandBufferInfo);
						ptr++;
					}
					if (m_objectIndexBuffer.empty())
						return; // Empty exit, in case nothing needs to be done
				}

				// Upload m_objectIndexBuffer to m_objectIndexBufferData:
				{
					if (m_objectIndexBufferData->BoundObject() == nullptr ||
						m_objectIndexBufferData->BoundObject()->ObjectCount() < m_objectIndexBuffer.size()) {
						size_t count = (m_objectIndexBufferData->BoundObject() == nullptr) ? size_t(1u) :
							Math::Max(m_objectIndexBufferData->BoundObject()->ObjectCount(), size_t(1u));
						while (count < m_objectIndexBuffer.size())
							count <<= 1u;
						m_objectIndexBufferData->BoundObject() = m_context->Graphics()->Device()->CreateArrayBuffer<uint32_t>(count);
						if (m_objectIndexBufferData->BoundObject() == nullptr) {
							m_context->Log()->Error(
								"SkinnedMeshRenderer::Helpers::SkinnedMeshRendererViewportData::SimulationKernelInstance::Execute - ",
								"Failed to allocate object index buffer data! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return;
						}
					}
					std::memcpy(
						m_objectIndexBufferData->BoundObject()->Map(), 
						(const void*)m_objectIndexBuffer.data(),
						m_objectIndexBuffer.size() * sizeof(uint32_t));
					m_objectIndexBufferData->BoundObject()->Unmap(true);
				}

				// Run combined kernel:
				m_combinedKernel->Execute(commandBufferInfo, tasks, taskCount);
			}
		};

		class SimulationKernel : public virtual GraphicsSimulation::Kernel {
		public:
			inline SimulationKernel() : GraphicsSimulation::Kernel(sizeof(TaskSettings)) {}

			inline virtual ~SimulationKernel() {}

			inline virtual Reference<GraphicsSimulation::KernelInstance> CreateInstance(SceneContext* context)const final override {
				const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> objectIndexBufferData = 
					Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
				Graphics::BindingSet::BindingSearchFunctions searchFn = {};
				auto findObjectIndexBuffer = [&](const auto& info) -> Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> {
					if (info.name == "culledObjectIndices")
						return objectIndexBufferData;
					else return nullptr;
				};
				searchFn.structuredBuffer = &findObjectIndexBuffer;

				static const constexpr std::string_view SHADER_PATH(
					"Jimara/Components/GraphicsObjects/SkinnedMeshRenderer_CombinedIndexGeneration_Culled.comp");
				const Reference<GraphicsSimulation::KernelInstance> combinedKernel =
					CombinedGraphicsSimulationKernel<TaskSettings>::Create(context, SHADER_PATH, searchFn);
				if (combinedKernel == nullptr) {
					context->Log()->Error(
						"SkinnedMeshRenderer::Helpers::SkinnedMeshRendererViewportData::SimulationKernel::CreateInstance - ",
						"Failed to create combined kernel instance! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}

				return Object::Instantiate<SimulationKernelInstance>(context, combinedKernel, objectIndexBufferData);
			}

			static SimulationKernel* Instance() {
				static SimulationKernel instance;
				return &instance;
			}
		};

		struct SimulationTask : public virtual GraphicsSimulation::Task {
			mutable SpinLock m_pipelineDescLock;
			Reference<SkinnedMeshRenderPipelineDescriptor> m_pipelineDescriptorRef;
			const Reference<const RendererFrustrumDescriptor> m_frustrum;
			SimulationTask* const self;

			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_indexBufferBinding =
				Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>();
			mutable std::atomic<size_t> m_indexCount = 0u;

			using BindlessBinding = SkinnedMeshRenderPipelineDescriptor::BindlessBinding;
			mutable Graphics::ArrayBufferReference<uint32_t> m_culledIndexBuffer;
			mutable BindlessBinding m_meshId;
			mutable BindlessBinding m_deformedId;

			// First m_liveIndexCount entries correspond to 'alive' indices, 
			// followed by m_liveInstanceRangeBuffer[m_liveInstanceCount] = 1 as a last entry for 'counts'.
			mutable Graphics::ArrayBufferReference<uint32_t> m_liveInstanceRangeBuffers;
			mutable std::atomic<size_t> m_liveInstanceRangeStagingBufferStride = 0u;
			mutable std::atomic<size_t> m_liveInstanceRangeBufferOffset = 0u;
			mutable std::atomic<size_t> m_liveInstanceCount = 0u;

			inline SimulationTask(
				SkinnedMeshRenderPipelineDescriptor* pipelineDesc,
				const RendererFrustrumDescriptor* frustrumDesc)
				: GraphicsSimulation::Task(SimulationKernel::Instance(), pipelineDesc->m_desc.context)
				, m_pipelineDescriptorRef(pipelineDesc)
				, m_frustrum(frustrumDesc)
				, self(this) {
				assert(m_pipelineDescriptorRef != nullptr);
				std::unique_lock<std::mutex> lock(m_pipelineDescriptorRef->m_lock);
				m_indexBufferBinding->BoundObject() = m_pipelineDescriptorRef->m_deformedIndexBinding->BoundObject();
				m_indexCount = (frustrumDesc == nullptr || (frustrumDesc->Flags() & RendererFrustrumFlags::PRIMARY) != RendererFrustrumFlags::NONE) ?
					m_indexBufferBinding->BoundObject()->ObjectCount() : size_t(0u);
				SetSettings(TaskSettings());
			}
			inline virtual ~SimulationTask() {}

			inline void Update(std::vector<uint32_t>& includedIndices, const Graphics::InFlightBufferInfo& commandBuffer)const {
				auto clearBindings = [&]() {
					m_meshId = nullptr;
					m_deformedId = nullptr;
					m_culledIndexBuffer = nullptr;
					m_liveInstanceRangeBuffers = nullptr;
					m_liveInstanceCount = 0u;
					self->SetSettings(TaskSettings());
				};

				m_indexCount = 0u;

				Reference<SkinnedMeshRenderPipelineDescriptor> pipelineDescriptor;
				{
					std::unique_lock<SpinLock> lock(m_pipelineDescLock);
					pipelineDescriptor = m_pipelineDescriptorRef;
				}

				// If there's no frustrum or no default index buffer, we pick the entire index buffer:
				if (pipelineDescriptor == nullptr || m_frustrum == nullptr || 
					pipelineDescriptor->m_deformedIndexBinding->BoundObject() == nullptr) {
					if (pipelineDescriptor != nullptr) {
						m_indexBufferBinding->BoundObject() = pipelineDescriptor->m_deformedIndexBinding->BoundObject();
						m_indexCount = m_indexBufferBinding->BoundObject()->ObjectCount();
					}
					return clearBindings();
				}

				// Get frustrum info:
				const Matrix4 frustrumMatrix = m_frustrum->FrustrumTransform();
				const RendererFrustrumDescriptor* const viewportFrustrum = m_frustrum->ViewportFrustrumDescriptor();
				const Matrix4 viewportMatrix = (viewportFrustrum == nullptr) ? frustrumMatrix : viewportFrustrum->FrustrumTransform();

				// Boundary check:
				const auto& bounds = pipelineDescriptor->m_instanceBoundaries;
				if (bounds.Size() <= 0u)
					return clearBindings();
				auto checkBounds = [&](size_t boundIndex) {
					const auto& bnd = bounds[boundIndex];
					return Culling::FrustrumAABBCulling::Test(frustrumMatrix, viewportMatrix,
						bnd.transform, bnd.localBounds, bnd.minOnScreenSize, bnd.maxOnScreenSize);
				};

				// If there's only one object, no kernel dispatch is necessary, we just check if the enrire thing has to be displayed or not:
				if (bounds.Size() == 1u) {
					m_indexBufferBinding->BoundObject() = pipelineDescriptor->m_deformedIndexBinding->BoundObject();
					m_indexCount = checkBounds(0u) ? m_indexBufferBinding->BoundObject()->ObjectCount() : 0u;
					clearBindings();
					return;
				}

				// Cull individual boundaries and set task settings:
				const size_t baseIndex = includedIndices.size();
				if (Culling::FrustrumAABBCulling::TestVisible(
					frustrumMatrix, Math::Identity(), pipelineDescriptor->m_combinedBoundaries))
					for (size_t i = 0u; i < bounds.Size(); i++)
						if (checkBounds(i))
							includedIndices.push_back(static_cast<uint32_t>(i));
				const size_t liveEntryCount = (includedIndices.size() - baseIndex);

				// (Re)Allocate culled index buffer if needed:
				m_indexCount = liveEntryCount *
					((pipelineDescriptor->m_meshIndices == nullptr) ? size_t(0u) : pipelineDescriptor->m_meshIndices->ObjectCount());
				if (m_culledIndexBuffer == nullptr || m_culledIndexBuffer->ObjectCount() < m_indexCount) {
					size_t allocSize = (m_culledIndexBuffer == nullptr) ? size_t(1u) : Math::Max(m_culledIndexBuffer->ObjectCount(), size_t(1u));
					while (allocSize < m_indexCount)
						allocSize <<= 1u;
					allocSize = Math::Min(allocSize, pipelineDescriptor->m_deformedIndexBinding->BoundObject()->ObjectCount());
					m_culledIndexBuffer = Context()->Graphics()->Device()->CreateArrayBuffer<uint32_t>(allocSize);
					if (m_culledIndexBuffer == nullptr) {
						Context()->Log()->Error(
							"SkinnedMeshRenderer::Helpers::SkinnedMeshRendererViewportData::SimulationTask::Update - ",
							"Failed to allocate culled index buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						m_indexBufferBinding->BoundObject() = pipelineDescriptor->m_deformedIndexBinding->BoundObject();
						m_indexCount = 0u;
						clearBindings();
						return;
					}
				}

				// (Re)Allocate m_liveInstanceRangeBuffer if needed:
				if (m_liveInstanceRangeBuffers == nullptr ||
					m_liveInstanceRangeStagingBufferStride < (liveEntryCount + 1u)) {
					size_t allocSize = 1u;
					while (allocSize <= (liveEntryCount + 1u))
						allocSize <<= 1u;
					m_liveInstanceRangeBuffers = Context()->Graphics()->Device()->CreateArrayBuffer<uint32_t>(
						allocSize * Math::Max(Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount(), size_t(1u)),
						Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
					if (m_liveInstanceRangeBuffers == nullptr) {
						Context()->Log()->Error(
							"SkinnedMeshRenderer::Helpers::SkinnedMeshRendererViewportData::SimulationTask::Update - ",
							"Failed to allocate live instance range atging buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						m_liveInstanceRangeStagingBufferStride = 0u;
						m_liveInstanceRangeBuffers = nullptr;
						m_liveInstanceCount = 0u;
					}
					else m_liveInstanceRangeStagingBufferStride = allocSize;
				}

				// Update m_liveInstanceRangeBuffer:
				if (m_liveInstanceRangeBuffers != nullptr) {
					assert(m_liveInstanceRangeBuffers != nullptr);
					assert(m_liveInstanceRangeBuffers->ObjectCount() >=
						(m_liveInstanceRangeStagingBufferStride * (commandBuffer.inFlightBufferId + 1u)));
					assert(m_liveInstanceRangeStagingBufferStride > liveEntryCount);
					const size_t srcElemOffsetCount = (m_liveInstanceRangeStagingBufferStride * commandBuffer.inFlightBufferId);
					{
						uint32_t* data = m_liveInstanceRangeBuffers.Map() + srcElemOffsetCount;
						for (size_t i = 0u; i < liveEntryCount; i++)
							data[i] = includedIndices[baseIndex + i];
						data[liveEntryCount] = 1u;
						m_liveInstanceRangeBuffers->Unmap(true);
					}
					m_liveInstanceRangeBufferOffset = (m_liveInstanceRangeStagingBufferStride * commandBuffer.inFlightBufferId);
					m_liveInstanceCount = liveEntryCount;
				}
				else {
					m_liveInstanceCount = 0u;
					m_liveInstanceRangeBufferOffset = 0u;
				}

				// Update settings if successful:
				m_indexBufferBinding->BoundObject() = m_culledIndexBuffer;
				TaskSettings settings = {};
				bool hasNullEntries = false;
				auto setBinding = [&](BindlessBinding& binding, Graphics::ArrayBuffer* buffer, uint32_t& index, const char* name) {
					pipelineDescriptor->SetBindlessBinding(binding, buffer, index, hasNullEntries, [&]() {
						Context()->Log()->Error(
							"SkinnedMeshRenderer::Helpers::SkinnedMeshRendererViewportData::SimulationTask::Update - ",
							"Failed to get binding for '", name, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						});
					};
				settings.vertexCount = (pipelineDescriptor->m_meshVertices == nullptr)
					? 0u : static_cast<uint32_t>(pipelineDescriptor->m_meshVertices->ObjectCount());
				setBinding(m_meshId, pipelineDescriptor->m_meshIndices, settings.meshId, "meshId");
				setBinding(m_deformedId, m_culledIndexBuffer, settings.deformedId, "deformedId");
				settings.taskThreadCount = hasNullEntries ? 0u : static_cast<uint32_t>(m_indexCount);
				settings.baseObjectIndexId = static_cast<uint32_t>(baseIndex);
				self->SetSettings(settings);
			}
		};
		const Reference<SimulationTask> m_simulationTask;
		GraphicsSimulation::TaskBinding m_taskBinding;

	public:
		inline SkinnedMeshRendererViewportData(
			SkinnedMeshRenderPipelineDescriptor* pipelineDesc,
			const RendererFrustrumDescriptor* frustrumDesc)
			: GraphicsObjectDescriptor::ViewportData(pipelineDesc->m_desc.geometryType)
			, m_simulationTask(Object::Instantiate<SimulationTask>(pipelineDesc, frustrumDesc)) {
			m_taskBinding = m_simulationTask;
		}

		inline virtual ~SkinnedMeshRendererViewportData() {
			m_taskBinding = nullptr;
			std::unique_lock<SpinLock> lock(m_simulationTask->m_pipelineDescLock);
			m_simulationTask->m_pipelineDescriptorRef = nullptr;
		}

		inline virtual Graphics::BindingSet::BindingSearchFunctions BindingSearchFunctions()const override {
			return m_simulationTask->m_pipelineDescriptorRef->m_cachedMaterialInstance->BindingSearchFunctions();
		}

		inline virtual GraphicsObjectDescriptor::VertexInputInfo VertexInput()const override {
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
				vertexInfo.binding = m_simulationTask->m_pipelineDescriptorRef->m_deformedVertexBinding;
			}
			{
				GraphicsObjectDescriptor::VertexBufferInfo& instanceInfo = info.vertexBuffers[1u];
				instanceInfo.layout.inputRate = Graphics::GraphicsPipeline::VertexInputInfo::InputRate::INSTANCE;
				instanceInfo.layout.bufferElementSize = sizeof(SkinnedMeshInstanceData);
				instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
					StandardLitShaderInputs::JM_ObjectTransform_Location, offsetof(SkinnedMeshInstanceData, transform)));
				instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
					StandardLitShaderInputs::JM_VertexColor_Location, offsetof(SkinnedMeshInstanceData, vertexColor)));
				instanceInfo.layout.locations.Push(Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo(
					StandardLitShaderInputs::JM_ObjectTilingAndOffset_Location, offsetof(SkinnedMeshInstanceData, tilingAndOffset)));
				instanceInfo.binding = m_simulationTask->m_pipelineDescriptorRef->m_instanceBufferBinding;
			}
			info.indexBuffer = m_simulationTask->m_indexBufferBinding;
			return info;
		}
		inline virtual size_t IndexCount()const override { return m_simulationTask->m_indexCount; }
		inline virtual size_t InstanceCount()const override { return 1; }

		inline virtual void GetGeometry(GraphicsObjectDescriptor::GeometryDescriptor& descriptor)const override {
			// JM_VertexPosition:
			{
				const auto& meshVertices = m_simulationTask->m_pipelineDescriptorRef->m_meshVertices;
				descriptor.vertexPositions.buffer = m_simulationTask->m_pipelineDescriptorRef->m_deformedVertexBinding->BoundObject();
				descriptor.vertexPositions.bufferOffset = static_cast<uint32_t>(offsetof(SkinnedMeshVertex, position));
				descriptor.vertexPositions.numEntriesPerInstance = (meshVertices == nullptr)
					? 0u : static_cast<uint32_t>(meshVertices->ObjectCount());
				descriptor.vertexPositions.perVertexStride = static_cast<uint32_t>(sizeof(SkinnedMeshVertex));
				descriptor.vertexPositions.perInstanceStride =
					static_cast<uint32_t>(descriptor.vertexPositions.numEntriesPerInstance * sizeof(SkinnedMeshVertex));
			}

			// Index buffer:
			{
				descriptor.indexBuffer.buffer = m_simulationTask->m_pipelineDescriptorRef->m_meshIndices;
				descriptor.indexBuffer.baseIndexOffset = 0u;
				descriptor.indexBuffer.indexCount = (descriptor.indexBuffer.buffer == nullptr) ? 0u :
					static_cast<uint32_t>(descriptor.indexBuffer.buffer->Size() / sizeof(uint32_t));
			}

			// JM_ObjectTransform:
			{
				descriptor.instanceTransforms.buffer = m_simulationTask->m_pipelineDescriptorRef->m_instanceBufferBinding->BoundObject();
				descriptor.instanceTransforms.bufferOffset = static_cast<uint32_t>(offsetof(SkinnedMeshInstanceData, transform));
				descriptor.instanceTransforms.elemStride = 0u;
			}

			// Instances:
			{
				descriptor.instances.count = static_cast<uint32_t>(m_simulationTask->m_pipelineDescriptorRef->m_components.size());
				descriptor.instances.liveInstanceRangeBuffer = m_simulationTask->m_liveInstanceRangeBuffers;
				descriptor.instances.firstInstanceIndexOffset =
					static_cast<uint32_t>(m_simulationTask->m_liveInstanceRangeBufferOffset * sizeof(uint32_t));
				descriptor.instances.firstInstanceIndexStride = sizeof(uint32_t);
				descriptor.instances.instanceCountOffset = static_cast<uint32_t>(m_simulationTask->m_liveInstanceCount * sizeof(uint32_t));
				descriptor.instances.instanceCountStride = 0u;
				descriptor.instances.liveInstanceEntryCount = (descriptor.instances.liveInstanceRangeBuffer != nullptr)
					? static_cast<uint32_t>(m_simulationTask->m_liveInstanceCount.load())
					: (descriptor.indexBuffer.indexCount > 0u)
					? static_cast<uint32_t>(m_simulationTask->m_indexCount / descriptor.indexBuffer.indexCount) : 0u;
			}

			// Flags:
			{
				descriptor.flags = GraphicsObjectDescriptor::GeometryFlags::NONE;
				if ((m_simulationTask->m_pipelineDescriptorRef->m_desc.flags & Flags::STATIC) != Flags::NONE)
					descriptor.flags |= (
						GraphicsObjectDescriptor::GeometryFlags::VERTEX_POSITION_CONSTANT |
						GraphicsObjectDescriptor::GeometryFlags::INSTANCE_TRANSFORM_CONSTANT);
			}
		}

		inline virtual Reference<Component> GetComponent(size_t objectIndex)const override {
			const auto& components = m_simulationTask->m_pipelineDescriptorRef->m_components;
			if (objectIndex < components.size())
				return components[objectIndex];
			else return nullptr;
		}
	};

	Reference<const GraphicsObjectDescriptor::ViewportData> SkinnedMeshRenderer::Helpers::SkinnedMeshRenderPipelineDescriptor::GetViewportData(
		const RendererFrustrumDescriptor* frustrum) {
		if ((frustrum != nullptr) &&
			((frustrum->Flags() & RendererFrustrumFlags::SHADOWMAPPER) != RendererFrustrumFlags::NONE) &&
			((m_desc.flags & TriMeshRenderer::Flags::CAST_SHADOWS) == TriMeshRenderer::Flags::NONE))
			return nullptr;
		return GetCachedOrCreate(frustrum, [&]() { return Object::Instantiate<SkinnedMeshRendererViewportData>(this, frustrum); });
	}
#pragma warning(default: 4250)





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
		GetLocalBoundaries();
		const TriMeshRenderer::Configuration batchDesc(this);
		if (m_pipelineDescriptor != nullptr) {
			Helpers::SkinnedMeshRenderPipelineDescriptor* descriptor = 
				dynamic_cast<Helpers::SkinnedMeshRenderPipelineDescriptor*>(m_pipelineDescriptor.operator->());
			Helpers::SkinnedMeshRenderPipelineDescriptor::Writer(descriptor).RemoveTransform(this);
			m_pipelineDescriptor = nullptr;
		}
		if (ActiveInHierarchy() && batchDesc.mesh != nullptr && batchDesc.material != nullptr && batchDesc.material->Shader() != nullptr) {
			Reference<Helpers::SkinnedMeshRenderPipelineDescriptor> descriptor;
			if (IsInstanced()) descriptor = Helpers::SkinnedMeshRenderPipelineDescriptor::Instancer::GetDescriptor(batchDesc);
			else descriptor = Object::Instantiate<Helpers::SkinnedMeshRenderPipelineDescriptor>(batchDesc, false);
			Helpers::SkinnedMeshRenderPipelineDescriptor::Writer(descriptor).AddTransform(this);
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
				Reference<Transform>(*getRoot)(BoneBinding*) = [](BoneBinding* binding) -> Reference<Transform> { return binding->Bone(); };
				void (*setRoot)(const Reference<Transform>&, BoneBinding*) = [](const Reference<Transform>& bone, BoneBinding* binding) { binding->SetBone(bone); };
				static const Reference<const Serialization::ItemSerializer::Of<BoneBinding>> serializer =
					Serialization::ValueSerializer<Reference<Transform>>::Create<BoneBinding>(
						"", "Deformation bone", Function<Reference<Transform>, BoneBinding*>(getRoot), Callback<const Reference<Transform>&, BoneBinding*>(setRoot));
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
			Reference<Transform>(*getRoot)(SkinnedMeshRenderer*) = [](SkinnedMeshRenderer* renderer) -> Reference<Transform> { return renderer->SkeletonRoot(); };
			void (*setRoot)(const Reference<Transform>&, SkinnedMeshRenderer*) = [](const Reference<Transform>& root, SkinnedMeshRenderer* renderer) { renderer->SetSkeletonRoot(root); };
			static const auto serializer = Serialization::ValueSerializer<Reference<Transform>>::Create<SkinnedMeshRenderer>(
				"Skeleton Root",
				"This is optional and mostly useful if one intends to reuse bones and place many instances of the same skinned mesh at multiple places and same pose",
				Function<Reference<Transform>, SkinnedMeshRenderer*>(getRoot), Callback<const Reference<Transform>&, SkinnedMeshRenderer*>(setRoot));
			recordElement(serializer->Serialize(this));
		}
		{
			static const BoneCollectionSerializer serializer;
			recordElement(serializer.Serialize(this));
		}
		{
			static const RendererCullingOptions::ConfigurableOptions::Serializer serializer("Culling Options", "Renderer cull/visibility options");
			recordElement(serializer.Serialize(&m_cullingOptions));
		}
	}

	void SkinnedMeshRenderer::GetSerializedActions(Callback<Serialization::SerializedCallback> report) {
		TriMeshRenderer::GetSerializedActions(report);
	}

	AABB SkinnedMeshRenderer::GetLocalBoundaries()const {
		Reference<TriMeshBoundingBox> bbox;
		{
			std::unique_lock<SpinLock> lock(m_meshBoundsLock);
			if (m_meshBounds == nullptr || m_meshBounds->TargetMesh() != Mesh())
				m_meshBounds = TriMeshBoundingBox::GetFor(Mesh());
			bbox = m_meshBounds;
		}
		AABB bounds = (bbox == nullptr) ? AABB(Vector3(0.0f), Vector3(0.0f)) : bbox->GetBoundaries();
		const RendererCullingOptions& cullingOptions = m_cullingOptions;
		const Vector3 start = bounds.start - cullingOptions.boundaryThickness + cullingOptions.boundaryOffset;
		const Vector3 end = bounds.end + cullingOptions.boundaryThickness + cullingOptions.boundaryOffset;
		return AABB(
			Vector3(Math::Min(start.x, end.x), Math::Min(start.y, end.y), Math::Min(start.z, end.z)),
			Vector3(Math::Max(start.x, end.x), Math::Max(start.y, end.y), Math::Max(start.z, end.z)));
	}

	AABB SkinnedMeshRenderer::GetBoundaries()const {
		const AABB localBoundaries = GetLocalBoundaries();
		const Transform* transform = GetTransform();
		return (transform == nullptr) ? localBoundaries : (transform->WorldMatrix() * localBoundaries);
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<SkinnedMeshRenderer>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<SkinnedMeshRenderer>(
			"Skinned Mesh Renderer", "Jimara/Graphics/SkinnedMeshRenderer", "Skinned mesh renderer that can deform based on bone positions");
		report(factory);
	}
}
