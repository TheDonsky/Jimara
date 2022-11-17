#include "InstanceBufferGenerator.h"


namespace Jimara {
	struct ParticleInstanceBufferGenerator::Helpers {
		struct TaskDescriptor : TaskSettings {
			alignas(4) uint32_t startThreadIndex = 0u;		// Bytes [80 - 84)
			alignas(4) uint32_t endThreadIndex = 0u;		// Bytes [84 - 88)
			alignas(4) uint32_t padA = 0u, padB = 0u;		// Bytes [88 - 96)
		};

		static_assert(sizeof(TaskDescriptor) == 96u);
		static_assert(offsetof(TaskDescriptor, startThreadIndex) == 80u);

		struct BindingSetDesc : public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
			Graphics::ArrayBufferReference<TaskDescriptor> taskDescriptorBuffer;
			bool isTaskDescriptorBuffer = false;
			Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance> bindlessArrayBuffers;

			inline virtual bool SetByEnvironment()const override { return false; }
			
			inline virtual size_t ConstantBufferCount()const override { return 0u; }
			inline virtual BindingInfo ConstantBufferInfo(size_t)const override { return {}; }
			inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t)const override { return nullptr; }

			inline virtual size_t StructuredBufferCount()const override { return isTaskDescriptorBuffer ? 1u : 0u; }
			inline virtual BindingInfo StructuredBufferInfo(size_t)const override { return BindingInfo{ Graphics::StageMask(Graphics::PipelineStage::COMPUTE), 0u }; }
			inline virtual Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t)const override { return taskDescriptorBuffer; }
			
			inline virtual size_t TextureSamplerCount()const override { return 0u; }
			inline virtual BindingInfo TextureSamplerInfo(size_t)const override { return {}; }
			inline virtual Reference<Graphics::TextureSampler> Sampler(size_t)const override { return nullptr; }
			
			inline virtual bool IsBindlessArrayBufferArray()const override { return bindlessArrayBuffers != nullptr; }
			inline virtual Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance> BindlessArrayBuffers()const { return bindlessArrayBuffers; }
		};

		class PipelineDescriptor : public virtual Graphics::ComputePipeline::Descriptor {
		private:
			const Reference<BindingSetDesc> m_bindingSets[2] = {
				Object::Instantiate<BindingSetDesc>(),
				Object::Instantiate<BindingSetDesc>()
			};
			const Reference<Graphics::Shader> m_shader;
			uint32_t m_threadCount = 0u;

			inline PipelineDescriptor(SceneContext* context, Graphics::Shader* shader)
				: m_shader(shader) {
				m_bindingSets[0]->bindlessArrayBuffers = context->Graphics()->Bindless().BufferBinding();
				m_bindingSets[1]->isTaskDescriptorBuffer = true;
			}

		public:
			inline static Reference<PipelineDescriptor> Create(SceneContext* context) {
				const Reference<Graphics::ShaderSet> shaderSet = context->Graphics()->Configuration().ShaderLoader()->LoadShaderSet("");
				if (shaderSet == nullptr) {
					context->Log()->Error("ParticleInstanceBufferGenerator::Helpers::PipelineDescriptor::Create - Failed to get shader set! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}

				const Graphics::ShaderClass SHADER_CLASS("Jimara/Environment/Rendering/Particles/Kernels/InstanceBufferGenerator/InstanceBufferGenerator_Kernel");
				const Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(&SHADER_CLASS, Graphics::PipelineStage::COMPUTE);
				if (binary == nullptr) {
					context->Log()->Error("ParticleInstanceBufferGenerator::Helpers::PipelineDescriptor::Create - Failed to get shader binary for '", SHADER_CLASS.ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}

				const Reference<Graphics::ShaderCache> shaderCache = Graphics::ShaderCache::ForDevice(context->Graphics()->Device());
				if (shaderCache == nullptr) {
					context->Log()->Error("ParticleInstanceBufferGenerator::Helpers::PipelineDescriptor::Create - Failed to get shader cache! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}

				const Reference<Graphics::Shader> shader = shaderCache->GetShader(binary);
				if (shader == nullptr) {
					context->Log()->Error("ParticleInstanceBufferGenerator::Helpers::PipelineDescriptor::Create - Failed to create shader module for '", SHADER_CLASS.ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}

				Reference<PipelineDescriptor> rv = new PipelineDescriptor(context, shader);
				rv->ReleaseRef();
				return rv;
			}

			inline virtual ~PipelineDescriptor() {}

			inline virtual size_t BindingSetCount()const { return 2u; }
			inline virtual const BindingSetDescriptor* BindingSet(size_t index)const { return m_bindingSets[index]; }

			inline virtual Reference<Graphics::Shader> ComputeShader()const override { return m_shader; }
			inline virtual Size3 NumBlocks() override {
				const constexpr size_t BLOCK_SIZE = 256u;
				return Size3((m_threadCount + BLOCK_SIZE - 1u) / BLOCK_SIZE, 1u, 1u);
			}

			inline Graphics::ArrayBufferReference<TaskDescriptor>& DescriptorBuffer()const { 
				return m_bindingSets[1]->taskDescriptorBuffer;
			}

			inline void SetThreadCount(uint32_t threadCount) { m_threadCount = threadCount; }
		};

		class KernelInstance : public virtual ParticleKernel::Instance {
		private:
			const Reference<SceneContext> m_context;
			const Reference<PipelineDescriptor> m_pipelineDescriptor;
			const Reference<Graphics::ComputePipeline> m_pipeline;
			std::vector<TaskDescriptor> m_lastTaskDescriptors;

		public:
			inline KernelInstance(SceneContext* context, PipelineDescriptor* pipelineDescriptor, Graphics::ComputePipeline* pipeline)
				: m_context(context), m_pipelineDescriptor(pipelineDescriptor), m_pipeline(pipeline) {}

			inline virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const Task* const* tasks, size_t taskCount) override {
				bool taskDescriptorBufferDirty = false;
				
				// Allocate enough bytes:
				if (m_lastTaskDescriptors.size() < taskCount) {
					m_pipelineDescriptor->DescriptorBuffer() = nullptr;
					m_pipelineDescriptor->DescriptorBuffer() = m_context->Graphics()->Device()->CreateArrayBuffer<TaskDescriptor>(
						Math::Max(m_lastTaskDescriptors.size() << 1u, taskCount));
					if (m_pipelineDescriptor->DescriptorBuffer() == nullptr) {
						m_lastTaskDescriptors.clear();
						m_context->Log()->Error("ParticleInstanceBufferGenerator::Helpers::KernelInstance::Execute - Failed to allocate input buffer for the kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return;
					}
					else m_lastTaskDescriptors.resize(m_pipelineDescriptor->DescriptorBuffer()->ObjectCount());
					taskDescriptorBufferDirty = true;
				}

				// Store memory on CPU side and update kernel size:
				{
					uint32_t numberOfThreads = 0u;
					TaskDescriptor* descPtr = m_lastTaskDescriptors.data();
					const Task* const* taskPtr = tasks;
					const Task* const* const end = taskPtr + taskCount;
					while (taskPtr < end) {
						const TaskSettings settings = (*taskPtr)->GetSettings<TaskSettings>();
						if (settings.instanceEndId > settings.instanceStartId) {
							if (settings != (*descPtr)) {
								(*(TaskSettings*)descPtr) = settings;
								taskDescriptorBufferDirty = true;
							}
							{
								descPtr->startThreadIndex = numberOfThreads;
								numberOfThreads += (settings.instanceEndId - settings.instanceStartId);
								descPtr->endThreadIndex = numberOfThreads;
							}
							descPtr++;
						}
						else taskCount--;
						taskPtr++;
					}
					m_pipelineDescriptor->SetThreadCount(numberOfThreads);
				}

				// If dirty, update buffer on GPU:
				if (taskDescriptorBufferDirty && taskCount > 0u) {
					std::memcpy(m_pipelineDescriptor->DescriptorBuffer()->Map(), m_lastTaskDescriptors.data(), taskCount * sizeof(TaskDescriptor));
					m_pipelineDescriptor->DescriptorBuffer()->Unmap(true);
				}

				// Execute pipeline:
				m_pipeline->Execute(commandBufferInfo);
			}
		};
	};

	bool ParticleInstanceBufferGenerator::TaskSettings::operator!=(const TaskSettings& other)const {
		return
			particleStateBufferId != other.particleStateBufferId ||
			instanceBufferId != other.instanceBufferId ||
			instanceStartId != other.instanceStartId ||
			instanceEndId != other.instanceEndId ||
			baseTransform != other.baseTransform;
	}

	ParticleInstanceBufferGenerator::ParticleInstanceBufferGenerator() 
		: ParticleKernel(sizeof(TaskSettings)) {}

	ParticleInstanceBufferGenerator::~ParticleInstanceBufferGenerator() {}

	const ParticleInstanceBufferGenerator* ParticleInstanceBufferGenerator::Instance() {
		static const ParticleInstanceBufferGenerator instance;
		return &instance;
	}

	Reference<ParticleKernel::Instance> ParticleInstanceBufferGenerator::CreateInstance(SceneContext* context)const {
		if (context == nullptr) return nullptr;
		const Reference<Helpers::PipelineDescriptor> pipelineDescriptor = Helpers::PipelineDescriptor::Create(context);
		if (pipelineDescriptor == nullptr) {
			context->Log()->Error("ParticleInstanceBufferGenerator::CreateInstance - Pipeline Descriptor could not be created! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		const Reference<Graphics::ComputePipeline> pipeline = context->Graphics()->Device()->CreateComputePipeline(
			pipelineDescriptor, context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
		if (pipeline == nullptr) {
			context->Log()->Error("ParticleInstanceBufferGenerator::CreateInstance - Compute Pipeline could not be created! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		else return Object::Instantiate<Helpers::KernelInstance>(context, pipelineDescriptor, pipeline);
	}
}
