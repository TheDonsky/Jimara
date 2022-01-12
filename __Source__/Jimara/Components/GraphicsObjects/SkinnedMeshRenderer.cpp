#include "SkinnedMeshRenderer.h"
#include "../../Core/Collections/ObjectSet.h"
#include "../../Graphics/Data/GraphicsMesh.h"
#include "../../Environment/GraphicsContext/SceneObjects/GraphicsObjectDescriptor.h"


namespace Jimara {
	namespace {
		struct InstancedBatchDesc {
			Reference<SceneContext> context;
			Reference<const TriMesh> mesh;
			Reference<const Material::Instance> material;
			bool isStatic;

			inline InstancedBatchDesc(SceneContext* ctx = nullptr, const TriMesh* geometry = nullptr, const Material::Instance* mat = nullptr, bool stat = false)
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
			size_t ctxHash = std::hash<Jimara::SceneContext*>()(desc.context);
			size_t meshHash = std::hash<const Jimara::TriMesh*>()(desc.mesh);
			size_t matHash = std::hash<const Jimara::Material::Instance*>()(desc.material);
			size_t staticHash = std::hash<bool>()(desc.isStatic);
			return Jimara::MergeHashes(Jimara::MergeHashes(ctxHash, Jimara::MergeHashes(meshHash, matHash)), staticHash);
		}
	};
}

namespace Jimara {
	namespace {
		static const uint32_t MAX_DEFORM_KERNELS_IN_FLIGHT = 4;
		static const uint32_t DEFORM_KERNEL_VERTEX_BUFFER_INDEX = 0;
		static const uint32_t DEFORM_KERNEL_BONE_WEIGHTS_INDEX = 1;
		static const uint32_t DEFORM_KERNEL_WEIGHT_START_IDS_INDEX = 2;
		static const uint32_t DEFORM_KERNEL_BONE_POSE_OFFSETS_INDEX = 3;
		static const uint32_t DEFORM_KERNEL_RESULT_BUFFER_INDEX = 4;
		static const uint32_t KERNEL_BLOCK_SIZE = 512;
		static Graphics::ShaderClass DEFORM_KERNEL_SHADER_CLASS("SkinnedMeshRenderer_Deformation");
		static Graphics::ShaderClass INDEX_GENERATION_KERNEL_SHADER_CLASS("SkinnedMeshRenderer_IndexGeneration");

#pragma warning(disable: 4250)
		class SkinnedMeshRenderPipelineDescriptor
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

			struct DeformationKernelInput
				: public virtual Graphics::ComputePipeline::Descriptor
				, Graphics::PipelineDescriptor::BindingSetDescriptor {
				const Reference<Graphics::Shader> shader;
				inline DeformationKernelInput(GraphicsContext* context)
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
				inline IndexGenerationKernelInput(GraphicsContext* context)
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

			ObjectSet<const SkinnedMeshRenderer> m_renderers;
			std::vector<Matrix4> m_currentOffsets;
			std::vector<Matrix4> m_lastOffsets;
			Graphics::ArrayBufferReference<Matrix4> m_boneOffsets;
			Graphics::ArrayBufferReference<MeshVertex> m_deformedVertices;
			Graphics::ArrayBufferReference<uint32_t> m_deformedIndices;
			std::vector<Reference<Graphics::PrimaryCommandBuffer>> m_updateBuffers;
			size_t m_updateBufferIndex = 0;
			bool m_renderersDirty = true;

			void RecalculateDeformedBuffer() {
				// Command buffer management:
				Graphics::PrimaryCommandBuffer* activeCommandBuffer = nullptr;
				auto executePipeline = [&](Graphics::ComputePipeline* pipeline) {
					if (activeCommandBuffer == nullptr) {
						if (m_updateBuffers.size() <= 0)
							m_updateBuffers = m_desc.context->Graphics()->Device()->GraphicsQueue()->CreateCommandPool()->CreatePrimaryCommandBuffers(MAX_DEFORM_KERNELS_IN_FLIGHT);
						m_updateBufferIndex = (m_updateBufferIndex + 1) % m_updateBuffers.size();
						activeCommandBuffer = m_updateBuffers[m_updateBufferIndex];
						activeCommandBuffer->Reset();
						activeCommandBuffer->BeginRecording();
					}
					pipeline->Execute(Graphics::Pipeline::CommandBufferInfo(activeCommandBuffer, m_updateBufferIndex));
				};
				auto submitCommandBuffer = [&]() {
					if (activeCommandBuffer == nullptr) return;
					activeCommandBuffer->EndRecording();
					m_desc.context->Graphics()->Device()->GraphicsQueue()->ExecuteCommandBuffer(activeCommandBuffer);
					activeCommandBuffer = nullptr;
				};

				// Update deformation and index kernel inputs:
				if (m_renderersDirty) {
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
							m_indexGenerationPipeline = m_desc.context->Graphics()->Device()->CreateComputePipeline(&m_indexGenerationKernelInput, MAX_DEFORM_KERNELS_IN_FLIGHT);
						executePipeline(m_indexGenerationPipeline);
					}
					else m_deformedIndices = m_meshIndices;

					m_lastOffsets.clear();
					m_renderersDirty = false;
				}
				else if (m_desc.isStatic) {
					submitCommandBuffer();
					return;
				}

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
					if (!dirty) {
						submitCommandBuffer();
						return;
					}
				}
				else m_lastOffsets = m_currentOffsets;

				// Update offsets buffer:
				{
					memcpy((void*)m_boneOffsets.Map(), (void*)m_currentOffsets.data(), sizeof(Matrix4) * m_currentOffsets.size());
					m_boneOffsets->Unmap(true);
				}

				// Execute deformation pipeline:
				if (m_deformPipeline == nullptr) 
					m_deformPipeline = m_desc.context->Graphics()->Device()->CreateComputePipeline(&m_deformationKernelInput, MAX_DEFORM_KERNELS_IN_FLIGHT);
				executePipeline(m_deformPipeline);
				submitCommandBuffer();
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
			inline SkinnedMeshRenderPipelineDescriptor(const InstancedBatchDesc& desc)
				: GraphicsObjectDescriptor(desc.material->Shader())
				, m_desc(desc)
				, m_graphicsObjectSet(GraphicsObjectDescriptor::Set::GetInstance(desc.context))
				, m_cachedMaterialInstance(desc.material)
				, m_deformationKernelInput(desc.context->Graphics())
				, m_indexGenerationKernelInput(desc.context->Graphics())
				, m_graphicsMesh(Graphics::GraphicsMeshCache::ForDevice(desc.context->Graphics()->Device())->GetMesh(desc.mesh, false)) {
				OnMeshDirty(nullptr);
				m_graphicsMesh->OnInvalidate() += Callback(&SkinnedMeshRenderPipelineDescriptor::OnMeshDirty, this);
			}

			inline virtual ~SkinnedMeshRenderPipelineDescriptor() {
				m_graphicsMesh->OnInvalidate() -= Callback(&SkinnedMeshRenderPipelineDescriptor::OnMeshDirty, this);
				m_deformPipeline = nullptr;
				m_indexGenerationPipeline = nullptr;
			}

			const InstancedBatchDesc& BatchDescriptor()const { return m_desc; }

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
			inline virtual AABB Bounds()const override { return AABB(); /* __TODO__: Implement this crap! */ }
			inline virtual size_t VertexBufferCount()const override { return 1; }
			inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index)const override { return &m_vertexBuffer; }
			inline virtual size_t InstanceBufferCount()const override { return 1; }
			inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index)const override { return &m_instanceBuffer; }
			inline virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer()const override { return m_deformedIndices; }
			inline virtual size_t IndexCount()const override { return m_deformedIndices->ObjectCount(); }
			inline virtual size_t InstanceCount()const override { return 1; }


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

				void AddTransform(const SkinnedMeshRenderer* renderer) {
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

				void RemoveTransform(const SkinnedMeshRenderer* renderer) {
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
			class Instancer : ObjectCache<InstancedBatchDesc> {
			public:
				inline static Reference<SkinnedMeshRenderPipelineDescriptor> GetDescriptor(const InstancedBatchDesc& desc) {
					static Instancer instance;
					return instance.GetCachedOrCreate(desc, false,
						[&]() -> Reference<SkinnedMeshRenderPipelineDescriptor> { return Object::Instantiate<SkinnedMeshRenderPipelineDescriptor>(desc); });
				}
			};
		};
#pragma warning(default: 4250)
	}

	SkinnedMeshRenderer::SkinnedMeshRenderer(Component* parent, const std::string_view& name,
		const TriMesh* mesh, const Jimara::Material* material, bool instanced, bool isStatic,
		const Transform** bones, size_t boneCount, const Transform* skeletonRoot)
		: Component(parent, name) {
		SetSkeletonRoot(skeletonRoot);
		for (size_t i = 0; i < boneCount; i++)
			SetBone(i, bones[i]);
		MarkStatic(isStatic);
		RenderInstanced(instanced);
		SetMesh(mesh);
		SetMaterial(material);
	}

	SkinnedMeshRenderer::SkinnedMeshRenderer(Component* parent, const std::string_view& name,
		const TriMesh* mesh, const Jimara::Material* material, bool instanced, bool isStatic,
		const Reference<const Transform>* bones, size_t boneCount, const Transform* skeletonRoot)
		: SkinnedMeshRenderer(parent, name, mesh, material, instanced, skeletonRoot) {
		SetSkeletonRoot(skeletonRoot);
		for (size_t i = 0; i < boneCount; i++)
			SetBone(i, bones[i]);
	}

	SkinnedMeshRenderer::~SkinnedMeshRenderer() { SetSkeletonRoot(nullptr); }

	const Transform* SkinnedMeshRenderer::SkeletonRoot()const { return m_skeletonRoot; }

	void SkinnedMeshRenderer::SetSkeletonRoot(const Transform* skeletonRoot) {
		if (m_skeletonRoot == skeletonRoot) return;
		const Callback<Component*> onDestroyedCallback(&SkinnedMeshRenderer::OnSkeletonRootDestroyed, this);
		if (m_skeletonRoot != nullptr) m_skeletonRoot->OnDestroyed() -= onDestroyedCallback;
		m_skeletonRoot = skeletonRoot;
		if (m_skeletonRoot != nullptr) m_skeletonRoot->OnDestroyed() += onDestroyedCallback;
	}

	size_t SkinnedMeshRenderer::BoneCount()const { return m_boneCount; }

	const Transform* SkinnedMeshRenderer::Bone(size_t index)const { return (index < m_boneCount) ? m_bones[index]->Bone() : nullptr; }

	void SkinnedMeshRenderer::SetBone(size_t index, const Transform* bone) {
		while (index >= m_boneCount) {
			if (m_bones.size() <= index) m_bones.push_back(Object::Instantiate<BoneBinding>());
			m_boneCount++;
		}
		m_bones[index]->SetBone(bone);
		while (m_boneCount > 0 && m_boneCount < m_bones.size() && m_bones[m_boneCount]->Bone() == nullptr) m_boneCount--;
	}

	void SkinnedMeshRenderer::OnTriMeshRendererDirty() {
		const InstancedBatchDesc batchDesc(Context(), Mesh(), MaterialInstance(), IsStatic());
		if (m_pipelineDescriptor != nullptr) {
			SkinnedMeshRenderPipelineDescriptor* descriptor = dynamic_cast<SkinnedMeshRenderPipelineDescriptor*>(m_pipelineDescriptor.operator->());
			if (descriptor->BatchDescriptor() == batchDesc) return;
			SkinnedMeshRenderPipelineDescriptor::Writer(descriptor).RemoveTransform(this);
			m_pipelineDescriptor = nullptr;
		}
		if (batchDesc.mesh != nullptr && batchDesc.material != nullptr) {
			Reference<SkinnedMeshRenderPipelineDescriptor> descriptor;
			if (IsInstanced()) descriptor = SkinnedMeshRenderPipelineDescriptor::Instancer::GetDescriptor(batchDesc);
			else descriptor = Object::Instantiate<SkinnedMeshRenderPipelineDescriptor>(batchDesc);
			SkinnedMeshRenderPipelineDescriptor::Writer(descriptor).AddTransform(this);
			m_pipelineDescriptor = descriptor;
		}
	}

	void SkinnedMeshRenderer::OnSkeletonRootDestroyed(Component*) { SetSkeletonRoot(nullptr); }



	SkinnedMeshRenderer::BoneBinding::~BoneBinding() { SetBone(nullptr); }

	const Transform* SkinnedMeshRenderer::BoneBinding::Bone()const { return m_bone; }

	void SkinnedMeshRenderer::BoneBinding::SetBone(const Transform* bone) {
		if (m_bone == bone) return;
		const Callback<Component*> onDestroyedCallback(&SkinnedMeshRenderer::BoneBinding::BoneDestroyed, this);
		if (m_bone != nullptr) m_bone->OnDestroyed() -= onDestroyedCallback;
		m_bone = bone;
		if (m_bone != nullptr) m_bone->OnDestroyed() += onDestroyedCallback;
	}

	void SkinnedMeshRenderer::BoneBinding::BoneDestroyed(Component*) { SetBone(nullptr); }
}
