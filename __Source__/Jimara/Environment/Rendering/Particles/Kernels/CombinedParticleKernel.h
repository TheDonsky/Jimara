#pragma once
#include "../ParticleKernel.h"


namespace Jimara {
	template<typename TaskSettings>
	class JIMARA_API CombinedParticleKernel : public virtual ParticleKernel::Instance {
	public:
		inline static Reference<CombinedParticleKernel> Create(SceneContext* context, const Graphics::ShaderClass* shaderClass);

		inline virtual ~CombinedParticleKernel() {}

		inline virtual void Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const ParticleKernel::Task* const* tasks, size_t taskCount) override;





	private:
		struct TaskDescriptor {
			TaskSettings taskSettings = {};
			alignas(16) Size3 taskBoundaries;
		};

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
			inline static Reference<PipelineDescriptor> Create(SceneContext* context, const Graphics::ShaderClass* shaderClass) {
				auto fail = [&](auto... message) {
					context->Log()->Error("CombinedParticleKernel<", TypeId::Of<TaskSettings>().Name(), ">::PipelineDescriptor::Create - ", message...);
					return nullptr;
				};

				const Reference<Graphics::ShaderSet> shaderSet = context->Graphics()->Configuration().ShaderLoader()->LoadShaderSet("");
				if (shaderSet == nullptr) return fail("Failed to get shader set! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				const Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(shaderClass, Graphics::PipelineStage::COMPUTE);
				if (binary == nullptr) return fail("Failed to get shader binary for '", shaderClass->ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				const Reference<Graphics::ShaderCache> shaderCache = Graphics::ShaderCache::ForDevice(context->Graphics()->Device());
				if (shaderCache == nullptr) return fail("Failed to get shader cache! [File: ", __FILE__, "; Line: ", __LINE__, "]");

				const Reference<Graphics::Shader> shader = shaderCache->GetShader(binary);
				if (shader == nullptr) return fail("Failed to create shader module for '", shaderClass->ShaderPath(), "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");

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

	private:
		const Reference<SceneContext> m_context;
		const Reference<PipelineDescriptor> m_pipelineDescriptor;
		const Reference<Graphics::ComputePipeline> m_pipeline;
		std::vector<TaskDescriptor> m_lastTaskDescriptors;

		inline CombinedParticleKernel(SceneContext* context, PipelineDescriptor* pipelineDescriptor, Graphics::ComputePipeline* pipeline)
			: m_context(context), m_pipelineDescriptor(pipelineDescriptor), m_pipeline(pipeline) {}
	};


	template<typename TaskSettings>
	inline Reference<CombinedParticleKernel<TaskSettings>> CombinedParticleKernel<TaskSettings>::Create(SceneContext* context, const Graphics::ShaderClass* shaderClass) {
		if (context == nullptr) return nullptr;
		const Reference<PipelineDescriptor> pipelineDescriptor = PipelineDescriptor::Create(context, shaderClass);
		if (pipelineDescriptor == nullptr) {
			context->Log()->Error("CombinedParticleKernel<", TypeId::Of<TaskSettings>().Name(), ">::Create - Pipeline Descriptor could not be created![File:", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		const Reference<Graphics::ComputePipeline> pipeline = context->Graphics()->Device()->CreateComputePipeline(
			pipelineDescriptor, context->Graphics()->Configuration().MaxInFlightCommandBufferCount());
		if (pipeline == nullptr) {
			context->Log()->Error("CombinedParticleKernel<", TypeId::Of<TaskSettings>().Name(), ">::Create - Compute Pipeline could not be created! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		const Reference<CombinedParticleKernel> instance = new CombinedParticleKernel(context, pipelineDescriptor, pipeline);
		instance->ReleaseRef();
		return instance;
	}

	template<typename TaskSettings>
	inline void CombinedParticleKernel<TaskSettings>::Execute(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, const ParticleKernel::Task* const* tasks, size_t taskCount) {
		bool taskDescriptorBufferDirty = false;

		// Allocate enough bytes:
		if (m_lastTaskDescriptors.size() < taskCount) {
			m_pipelineDescriptor->DescriptorBuffer() = nullptr;
			m_pipelineDescriptor->DescriptorBuffer() = m_context->Graphics()->Device()->CreateArrayBuffer<TaskDescriptor>(
				Math::Max(m_lastTaskDescriptors.size() << 1u, taskCount));
			if (m_pipelineDescriptor->DescriptorBuffer() == nullptr) {
				m_lastTaskDescriptors.clear();
				m_context->Log()->Error("CombinedParticleKernel<", TypeId::Of<TaskSettings>().Name(), ">::Execute - Failed to allocate input buffer for the kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
			else m_lastTaskDescriptors.resize(m_pipelineDescriptor->DescriptorBuffer()->ObjectCount());
			taskDescriptorBufferDirty = true;
		}

		// Store memory on CPU side and update kernel size:
		{
			uint32_t numberOfThreads = 0u;
			TaskDescriptor* descPtr = m_lastTaskDescriptors.data();
			const ParticleKernel::Task* const* taskPtr = tasks;
			const ParticleKernel::Task* const* const end = taskPtr + taskCount;
			while (taskPtr < end) {
				const TaskSettings settings = (*taskPtr)->GetSettings<TaskSettings>();
				if (settings.particleCount > 0u) {
					if (std::memcmp((void*)&settings, (void*)(&descPtr->taskSettings), sizeof(TaskSettings)) != 0) {
						descPtr->taskSettings = settings;
						taskDescriptorBufferDirty = true;
					}
					{
						descPtr->taskBoundaries.x = numberOfThreads;
						numberOfThreads += settings.particleCount;
						descPtr->taskBoundaries.y = numberOfThreads;
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
}
