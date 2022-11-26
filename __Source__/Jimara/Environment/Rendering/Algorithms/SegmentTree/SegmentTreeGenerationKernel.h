#pragma once
#include "../../../../Graphics/GraphicsDevice.h"
#include "../../../../Graphics/Data/ShaderBinaries/ShaderLoader.h"
#include "../../../../Graphics/Data/ShaderBinaries/ShaderResourceBindings.h"


namespace Jimara {
	class JIMARA_API SegmentTreeGenerationKernel : public virtual Object {
	public:
		static Reference<SegmentTreeGenerationKernel> Create(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader,
			const Graphics::ShaderClass* generationKernelShaderClass, size_t maxInFlightCommandBuffers, size_t workGroupSize,
			const std::string_view& segmentTreeBufferBindingName = "segmentTreeBuffer",
			const std::string_view& generationKernelSettingsName = "segmentTreeGenerationSettings",
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& additionalBindings = Graphics::ShaderResourceBindings::ShaderBindingDescription());

		virtual ~SegmentTreeGenerationKernel();

		static size_t SegmentTreeBufferSize(size_t inputBufferSize);

		Reference<Graphics::ArrayBuffer> Execute(
			const Graphics::Pipeline::CommandBufferInfo& commandBuffer, Graphics::ArrayBuffer* inputBuffer, size_t inputBufferSize, bool generateInPlace);

		static Reference<SegmentTreeGenerationKernel> CreateUintSumKernel(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers);

		static Reference<SegmentTreeGenerationKernel> CreateUintProductKernel(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers);

		static Reference<SegmentTreeGenerationKernel> CreateIntSumKernel(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers);

		static Reference<SegmentTreeGenerationKernel> CreateIntProductKernel(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers);

		static Reference<SegmentTreeGenerationKernel> CreateFloatSumKernel(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers);

		static Reference<SegmentTreeGenerationKernel> CreateFloatProductKernel(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers);

	private:
		const Reference<Graphics::GraphicsDevice> m_device;
		const Reference<Graphics::ShaderResourceBindings::StructuredBufferBinding> m_resultBufferBinding;
		const Reference<Graphics::ComputePipeline::Descriptor> m_pipelineDescriptor;
		const Reference<Graphics::Buffer> m_settingsBuffer;
		const size_t m_maxInFlightCommandBuffers;
		const size_t m_workGroupSize;
		Reference<Graphics::ComputePipeline> m_pipeline;
		size_t m_maxPipelineIterations = 0u;

		struct Helpers;

		SegmentTreeGenerationKernel(
			Graphics::GraphicsDevice* device,
			Graphics::ShaderResourceBindings::StructuredBufferBinding* resultBufferBinding,
			Graphics::ComputePipeline::Descriptor* pipelineDescriptor,
			Graphics::Buffer* settingsBuffer,
			size_t maxInFlightCommandBuffers,
			size_t workGroupSize);
	};
}
