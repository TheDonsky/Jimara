#pragma once
#include "../../../../Graphics/GraphicsDevice.h"
#include "../../../../Graphics/Data/ShaderBinaries/ShaderResourceBindings.h"


namespace Jimara {
	class JIMARA_API BitonicSortKernel : public virtual Object {
	public:
		static Reference<BitonicSortKernel> Create(
			Graphics::GraphicsDevice* device,
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings,
			size_t maxInFlightCommandBuffers, size_t workGroupSize,
			Graphics::Shader* singleStepShader, Graphics::Shader* groupsharedShader = nullptr,
			const std::string_view& bitonicSortSettingsName = "bitonicSortSettings");

		virtual ~BitonicSortKernel();

		void Execute(const Graphics::Pipeline::CommandBufferInfo& commandBuffer, size_t elemCount);

	private:
		/// <summary> Kernel configuration for a single step for Bitonic Sort network </summary>
		struct BitonicSortSettings {
			/// <summary> sequenceSize = (1 << sequenceSizeBit) </summary>
			alignas(4) uint32_t sequenceSizeBit;      // 1;	2;		3;			...

			/// <summary> comparizonStep = (1 << comparizonStepBit) </summary>
			alignas(4) uint32_t comparizonStepBit;    // 0;	1, 0;	2, 1, 0;    ...
		};
		static_assert(sizeof(BitonicSortSettings) == 8);
		
		const Reference<Graphics::GraphicsDevice> m_device;
		const Reference<Graphics::ComputePipeline::Descriptor> m_singleStepPipelineDescriptor;
		const Reference<Graphics::ComputePipeline::Descriptor> m_groupsharedPipelineDescriptor;
		const Graphics::BufferReference<BitonicSortSettings> m_settingsBuffer;
		const std::shared_ptr<Size3> m_kernelSize;
		const size_t m_maxInFlightCommandBuffers;
		const size_t m_workGroupSize;
		std::atomic<uint32_t> m_maxListSizeBit;
		Reference<Graphics::ComputePipeline> m_singleStepPipeline;
		Reference<Graphics::ComputePipeline> m_groupsharedPipeline;

		struct Helpers;

		BitonicSortKernel(
			Graphics::GraphicsDevice* device,
			Graphics::ComputePipeline::Descriptor* singleStepPipelineDescriptor, 
			Graphics::ComputePipeline::Descriptor* groupsharedPipelineDescriptor,
			const Graphics::BufferReference<BitonicSortSettings>& settingsBuffer,
			const std::shared_ptr<Size3>& kernelSize,
			size_t maxInFlightCommandBuffers, size_t workGroupSize);
	};
}
