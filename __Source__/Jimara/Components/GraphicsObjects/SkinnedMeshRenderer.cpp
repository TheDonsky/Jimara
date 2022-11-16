#include "SkinnedMeshRenderer.h"
#include "../../Core/Collections/ObjectSet.h"
#include "../../Graphics/Data/GraphicsMesh.h"
#include "../../Environment/Rendering/SceneObjects/GraphicsObjectDescriptor.h"


namespace Jimara {
	namespace {
		static const uint32_t DEFORM_KERNEL_VERTEX_BUFFER_INDEX = 0;
		static const uint32_t DEFORM_KERNEL_BONE_WEIGHTS_INDEX = 1;
		static const uint32_t DEFORM_KERNEL_WEIGHT_START_IDS_INDEX = 2;
		static const uint32_t DEFORM_KERNEL_BONE_POSE_OFFSETS_INDEX = 3;
		static const uint32_t DEFORM_KERNEL_RESULT_BUFFER_INDEX = 4;
		static const uint32_t KERNEL_BLOCK_SIZE = 512;
		static Graphics::ShaderClass DEFORM_KERNEL_SHADER_CLASS("Jimara/Components/GraphicsObjects/SkinnedMeshRenderer_Deformation");
		static Graphics::ShaderClass INDEX_GENERATION_KERNEL_SHADER_CLASS("Jimara/Components/GraphicsObjects/SkinnedMeshRenderer_IndexGeneration");

#pragma warning(disable: 4250)
		class SkinnedMeshRenderPipelineDescriptor
			: public virtual ObjectCache<TriMeshRenderer::Configuration>::StoredObject
			, public virtual GraphicsObjectDescriptor
			, public virtual JobSystem::Job {
		private:
			const TriMeshRenderer::Configuration m_desc;
			const Reference<GraphicsObjectDescriptor::Set> m_graphicsObjectSet;
			// __TODO__: This is not fully safe... stores self-reference; some refactor down the line would be adviced
			Reference<GraphicsObjectDescriptor::Set::ItemOwner> m_owner = nullptr;
			Material::CachedInstance m_cachedMaterialInstance;
			std::mutex m_lock;

			struct DeformationKernelInput
				: public virtual Graphics::ComputePipeline::Descriptor
				, Graphics::PipelineDescriptor::BindingSetDescriptor {
				const Reference<Graphics::Shader> shader;
				inline DeformationKernelInput(Scene::GraphicsContext* context)
					: shader(
						Graphics::ShaderCache::ForDevice(context->Device())->GetShader(
							context->Configuration().ShaderLoader()->LoadShaderSet("")->GetShaderModule(
						&DEFORM_KERNEL_SHADER_CLASS, Graphics::PipelineStage::COMPUTE))) {}

				// Buffers
				Graphics::BufferReference<uint32_t> boneCountBuffer;
				Reference<Graphics::ArrayBuffer> structuredBuffers[5];

				// Graphics::PipelineDescriptor:
				virtual size_t BindingSetCount()const override { return 1; }
				virtual const Graphics::PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t index)const override { return this; }

				// Graphics::PipelineDescriptor::BindingSetDescriptor:
				virtual bool SetByEnvironment()const override { return false; }
				virtual size_t ConstantBufferCount()const override { return 1; }
				virtual BindingInfo ConstantBufferInfo(size_t index)const override { return BindingInfo{ Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 0 }; }
				virtual Reference<Graphics::Buffer> ConstantBuffer(size_t index)const override { return boneCountBuffer; }
				virtual size_t StructuredBufferCount()const override { return 5; }
				virtual BindingInfo StructuredBufferInfo(size_t index)const override { return BindingInfo{ Graphics::StageMask(Graphics::PipelineStage::COMPUTE), static_cast<uint32_t>(index) + 1 }; }
				virtual Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t index)const override { return structuredBuffers[index]; }
				virtual size_t TextureSamplerCount()const override { return 0; }
				virtual BindingInfo TextureSamplerInfo(size_t index)const override { return BindingInfo{ Graphics::StageMask(Graphics::PipelineStage::NONE), 0 }; }
				virtual Reference<Graphics::TextureSampler> Sampler(size_t index)const override { return nullptr; }

				// Graphics::ComputePipeline::Descriptor:
				virtual Reference<Graphics::Shader> ComputeShader()const override { return shader; }
				virtual Size3 NumBlocks() override {
					return Size3(static_cast<uint32_t>(
						(structuredBuffers[DEFORM_KERNEL_RESULT_BUFFER_INDEX]->ObjectCount() + KERNEL_BLOCK_SIZE - 1) / KERNEL_BLOCK_SIZE), 1, 1);
				}
			} m_deformationKernelInput;
			Reference<Graphics::ComputePipeline> m_deformPipeline;

			struct IndexGenerationKernelInput
				: public virtual Graphics::ComputePipeline::Descriptor
				, Graphics::PipelineDescriptor::BindingSetDescriptor {
				const Reference<Graphics::Shader> shader;
				inline IndexGenerationKernelInput(Scene::GraphicsContext* context)
					: shader(Graphics::ShaderCache::ForDevice(context->Device())->GetShader(context->
						Configuration().ShaderLoader()->LoadShaderSet("")->GetShaderModule(
						&INDEX_GENERATION_KERNEL_SHADER_CLASS, Graphics::PipelineStage::COMPUTE))) {}

				// Buffers
				Graphics::BufferReference<uint32_t> vertexCountBuffer;
				Reference<Graphics::ArrayBuffer> structuredBuffers[2];

				// Graphics::PipelineDescriptor:
				virtual size_t BindingSetCount()const override { return 1; }
				virtual const Graphics::PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t index)const override { return this; }

				// Graphics::PipelineDescriptor::BindingSetDescriptor:
				virtual bool SetByEnvironment()const override { return false; }
				virtual size_t ConstantBufferCount()const override { return 1; }
				virtual BindingInfo ConstantBufferInfo(size_t index)const override { return BindingInfo{ Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 0 }; }
				virtual Reference<Graphics::Buffer> ConstantBuffer(size_t index)const override { return vertexCountBuffer; }
				virtual size_t StructuredBufferCount()const override { return 2; }
				virtual BindingInfo StructuredBufferInfo(size_t index)const override { return BindingInfo{ Graphics::StageMask(Graphics::PipelineStage::COMPUTE), static_cast<uint32_t>(index) + 1 }; }
				virtual Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t index)const override { return structuredBuffers[index]; }
				virtual size_t TextureSamplerCount()const override { return 0; }
				virtual BindingInfo TextureSamplerInfo(size_t index)const override { return BindingInfo{ Graphics::StageMask(Graphics::PipelineStage::NONE), 0 }; }
				virtual Reference<Graphics::TextureSampler> Sampler(size_t index)const override { return nullptr; }

				// Graphics::ComputePipeline::Descriptor:
				virtual Reference<Graphics::Shader> ComputeShader()const override { return shader; }
				virtual Size3 NumBlocks() override { return Size3(static_cast<uint32_t>((structuredBuffers[1]->ObjectCount() + KERNEL_BLOCK_SIZE - 1) / KERNEL_BLOCK_SIZE), 1, 1); }
			} m_indexGenerationKernelInput;
			Reference<Graphics::ComputePipeline> m_indexGenerationPipeline;
			
			const Reference<Graphics::GraphicsMesh> m_graphicsMesh;
			Graphics::ArrayBufferReference<MeshVertex> m_meshVertices;
			Graphics::ArrayBufferReference<uint32_t> m_meshIndices;
			Graphics::ArrayBufferReference<SkinnedTriMesh::BoneWeight> m_boneWeights;
			Graphics::ArrayBufferReference<uint32_t> m_boneWeightStartIds;
			std::vector<Matrix4> m_boneInverseReferencePoses;
			std::atomic<bool> m_meshDirty = true;

			inline void OnMeshDirty(Graphics::GraphicsMesh*) { m_meshDirty = true; }
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

				{
					if (m_deformationKernelInput.boneCountBuffer == nullptr)
						m_deformationKernelInput.boneCountBuffer = m_desc.context->Graphics()->Device()->CreateConstantBuffer<uint32_t>();
					m_deformationKernelInput.boneCountBuffer.Map() = static_cast<uint32_t>(m_boneInverseReferencePoses.size() + 1);
					m_deformationKernelInput.boneCountBuffer->Unmap(true);
				}
				m_deformationKernelInput.structuredBuffers[DEFORM_KERNEL_VERTEX_BUFFER_INDEX] = m_meshVertices;
				m_deformationKernelInput.structuredBuffers[DEFORM_KERNEL_BONE_WEIGHTS_INDEX] = m_boneWeights;
				m_deformationKernelInput.structuredBuffers[DEFORM_KERNEL_WEIGHT_START_IDS_INDEX] = m_boneWeightStartIds;

				m_renderersDirty = true;
				m_meshDirty = false;
			}

			ObjectSet<SkinnedMeshRenderer> m_renderers;
			std::vector<Reference<Component>> m_components;
			std::vector<Matrix4> m_currentOffsets;
			std::vector<Matrix4> m_lastOffsets;
			Graphics::ArrayBufferReference<Matrix4> m_boneOffsets;
			Graphics::ArrayBufferReference<MeshVertex> m_deformedVertices;
			Graphics::ArrayBufferReference<uint32_t> m_deformedIndices;
			bool m_renderersDirty = true;

			struct KeepComponentsAlive : public virtual Object {
				std::vector<Reference<Component>> components;
				inline void Clear(Object*) { components.clear(); }
			};
			const Reference<KeepComponentsAlive> m_keepComponentsAlive = Object::Instantiate<KeepComponentsAlive>();

			void RecalculateDeformedBuffer() {
				// Command buffer execution:
				auto executePipeline = [&](Graphics::ComputePipeline* pipeline) {
					pipeline->Execute(m_desc.context->Graphics()->GetWorkerThreadCommandBuffer());
				};

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

					m_boneOffsets = m_desc.context->Graphics()->Device()->CreateArrayBuffer<Matrix4>((m_boneInverseReferencePoses.size() + 1) * m_renderers.Size());
					m_deformationKernelInput.structuredBuffers[DEFORM_KERNEL_BONE_POSE_OFFSETS_INDEX] = m_boneOffsets;
					
					m_deformedVertices = m_desc.context->Graphics()->Device()->CreateArrayBuffer<MeshVertex>(m_meshVertices->ObjectCount() * m_renderers.Size());
					m_deformationKernelInput.structuredBuffers[DEFORM_KERNEL_RESULT_BUFFER_INDEX] = m_deformedVertices;

					if (m_renderers.Size() > 1) {
						m_deformedIndices = m_desc.context->Graphics()->Device()->CreateArrayBuffer<uint32_t>(m_meshIndices->ObjectCount() * m_renderers.Size());
						{
							if (m_indexGenerationKernelInput.vertexCountBuffer == nullptr)
								m_indexGenerationKernelInput.vertexCountBuffer = m_desc.context->Graphics()->Device()->CreateConstantBuffer<uint32_t>();
							m_indexGenerationKernelInput.vertexCountBuffer.Map() = static_cast<uint32_t>(m_meshVertices->ObjectCount());
							m_indexGenerationKernelInput.vertexCountBuffer->Unmap(true);
						}
						m_indexGenerationKernelInput.structuredBuffers[0] = m_meshIndices;
						m_indexGenerationKernelInput.structuredBuffers[1] = m_deformedIndices;
						if (m_indexGenerationPipeline == nullptr) 
							m_indexGenerationPipeline = m_desc.context->Graphics()->Device()->CreateComputePipeline(
								&m_indexGenerationKernelInput, m_desc.context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
						executePipeline(m_indexGenerationPipeline);
					}
					else m_deformedIndices = m_meshIndices;

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
					if (!dirty) return;
				}
				else m_lastOffsets = m_currentOffsets;

				// Update offsets buffer:
				{
					memcpy((void*)m_boneOffsets.Map(), (void*)m_currentOffsets.data(), sizeof(Matrix4) * m_currentOffsets.size());
					m_boneOffsets->Unmap(true);
				}

				// Execute deformation pipeline:
				if (m_deformPipeline == nullptr) 
					m_deformPipeline = m_desc.context->Graphics()->Device()->CreateComputePipeline(
						&m_deformationKernelInput, m_desc.context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
				executePipeline(m_deformPipeline);
			}

			
			// Mesh data:
			struct VertexBuffer : public virtual Graphics::VertexBuffer {
				Graphics::ArrayBufferReference<MeshVertex> buffer;

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
				inline virtual Reference<Graphics::ArrayBuffer> Buffer() override { return buffer; }
			} mutable m_vertexBuffer;

			// Instancing data:
			struct InstanceBuffer : public virtual Graphics::InstanceBuffer {
				Graphics::ArrayBufferReference<Matrix4> buffer;

				inline virtual size_t AttributeCount()const override { return 1; }
				inline virtual Graphics::InstanceBuffer::AttributeInfo Attribute(size_t)const {
					return { Graphics::InstanceBuffer::AttributeInfo::Type::MAT_4X4, 3, 0 };
				}
				inline virtual size_t BufferElemSize()const override { return sizeof(Matrix4); }
				inline virtual Reference<Graphics::ArrayBuffer> Buffer() override { return buffer; }
			} mutable m_instanceBuffer;


		public:
			inline SkinnedMeshRenderPipelineDescriptor(const TriMeshRenderer::Configuration& desc)
				: GraphicsObjectDescriptor(desc.material->Shader(), desc.layer, desc.geometryType)
				, m_desc(desc)
				, m_graphicsObjectSet(GraphicsObjectDescriptor::Set::GetInstance(desc.context))
				, m_cachedMaterialInstance(desc.material)
				, m_deformationKernelInput(desc.context->Graphics())
				, m_indexGenerationKernelInput(desc.context->Graphics())
				, m_graphicsMesh(Graphics::GraphicsMesh::Cached(desc.context->Graphics()->Device(), desc.mesh, desc.geometryType)) {
				OnMeshDirty(nullptr);
				m_graphicsMesh->OnInvalidate() += Callback(&SkinnedMeshRenderPipelineDescriptor::OnMeshDirty, this);
			}

			inline virtual ~SkinnedMeshRenderPipelineDescriptor() {
				m_graphicsMesh->OnInvalidate() -= Callback(&SkinnedMeshRenderPipelineDescriptor::OnMeshDirty, this);
				m_deformPipeline = nullptr;
				m_indexGenerationPipeline = nullptr;
			}

			const TriMeshRenderer::Configuration& BatchDescriptor()const { return m_desc; }

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


			/** GraphicsObjectDescriptor */
			inline virtual AABB Bounds()const override { return AABB(); /* __TODO__: Implement this crap! */ }
			inline virtual size_t VertexBufferCount()const override { return 1; }
			inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index)const override { return &m_vertexBuffer; }
			inline virtual size_t InstanceBufferCount()const override { return 1; }
			inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index)const override { return &m_instanceBuffer; }
			inline virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer()const override { return m_deformedIndices; }
			inline virtual size_t IndexCount()const override { return m_deformedIndices->ObjectCount(); }
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
				m_vertexBuffer.buffer = m_deformedVertices;
				if (m_instanceBuffer.buffer == nullptr) {
					m_instanceBuffer.buffer = m_desc.context->Graphics()->Device()->CreateArrayBuffer<Matrix4>(1);
					(*m_instanceBuffer.buffer.Map()) = Math::Identity();
					m_instanceBuffer.buffer->Unmap(true);
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
				}
			};

			/** Instancer: */
			class Instancer : ObjectCache<TriMeshRenderer::Configuration> {
			public:
				inline static Reference<SkinnedMeshRenderPipelineDescriptor> GetDescriptor(const TriMeshRenderer::Configuration& desc) {
					static Instancer instance;
					return instance.GetCachedOrCreate(desc, false,
						[&]() -> Reference<SkinnedMeshRenderPipelineDescriptor> { return Object::Instantiate<SkinnedMeshRenderPipelineDescriptor>(desc); });
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
			else descriptor = Object::Instantiate<SkinnedMeshRenderPipelineDescriptor>(batchDesc);
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
				target->m_boneCount = minFilled + 1;
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
		static const ComponentSerializer::Of<SkinnedMeshRenderer> serializer("Jimara/Graphics/SkinnedMeshRenderer", "Skinned Mesh Renderer");
		report(&serializer);
	}
}
