#pragma once
#include "../../../../Graphics/GraphicsDevice.h"
#include "../../../../Graphics/Data/ShaderBinaries/ShaderLoader.h"
#include "../../../../Graphics/Data/ShaderBinaries/ShaderResourceBindings.h"


namespace Jimara {
	/// <summary>
	/// An Object, capable of generating segment trees from arbitrary GPU buffers using compute pipelienes based on SegmentTree.glh
	/// <para/> Note: Read through SegmentTree.glh if you want to create a compatible kernel.
	/// </summary>
	class JIMARA_API SegmentTreeGenerationKernel : public virtual Object {
	public:
		/// <summary>
		/// Creates an instance of SegmentTreeGenerationKernel
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLoader"> Shader binary loader </param>
		/// <param name="generationKernelShaderClass"> Shader class of the kernel </param>
		/// <param name="maxInFlightCommandBuffers"> Maximal number of simultinuously running command buffers </param>
		/// <param name="workGroupSize"> Number of threads per workgroup </param>
		/// <param name="segmentTreeBufferBindingName"> Name of the segment tree content buffer binding inide the compute shader (binding will be provided vy the SegmentTreeGenerationKernel) </param>
		/// <param name="generationKernelSettingsName"> Name of the segment tree generator settings cbuffer binding inide the compute shader (binding will be provided vy the SegmentTreeGenerationKernel) </param>
		/// <param name="additionalBindings"> If operators rely on additional bindings, they can be provided thhrough this object </param>
		/// <returns> A new instance of SegmentTreeGenerationKernel if successful; nullptr otherwise </returns>
		static Reference<SegmentTreeGenerationKernel> Create(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader,
			const Graphics::ShaderClass* generationKernelShaderClass, size_t maxInFlightCommandBuffers, size_t workGroupSize,
			const std::string_view& segmentTreeBufferBindingName = "segmentTreeBuffer",
			const std::string_view& generationKernelSettingsName = "segmentTreeGenerationSettings",
			const Graphics::BindingSet::Descriptor::BindingSearchFunctions& additionalBindings = {});

		/// <summary> Virtual destructor </summary>
		virtual ~SegmentTreeGenerationKernel();

		/// <summary>
		/// Calculates the required buffer size (elem count) for a segment tree based on the source list size
		/// </summary>
		/// <param name="inputBufferSize"> Number of elements within the input list </param>
		/// <returns> Elem count required for the tree buffer </returns>
		static size_t SegmentTreeBufferSize(size_t inputBufferSize);

		/// <summary>
		/// Executes pipelines that will generate the segment tree on given command buffer and returns the buffer 
		/// which will hold the content of the segment tree once the command buffer is executed.
		/// <para/> Notes:
		/// <para/> 0. If generateInPlace flag is set, result buffer will be the same as the inputBuffer, but the user has to guarantee that 
		///		it's size is at least SegmentTreeBufferSize() or the call will fail;
		/// <para/> 1. If inputBuffer happens to be the result buffer from the previous call, system will behave as if generateInPlace flag was set;
		/// <para/> 2. User is responsible for making sure the input buffer content is formatted correctly; SegmentTreeGenerationKernel knows nothing about it.
		/// </summary>
		/// <param name="commandBuffer"> Command buffer and in-flight index </param>
		/// <param name="inputBuffer"> 'Source' buffer, the segment tree will be built from </param>
		/// <param name="inputBufferSize"> Number of used elements within the inputBuffer 
		///		(if inputBufferSize is greater than inputBuffer->ObjectCount(), inputBufferSize will be truncated) </param>
		/// <param name="generateInPlace"> If true, result buffer (return value) will be the same as the input buffer </param>
		/// <returns> Buffer, that will contain the segment tree once the command buffer gerts executed </returns>
		Reference<Graphics::ArrayBuffer> Execute(
			const Graphics::InFlightBufferInfo& commandBuffer, Graphics::ArrayBuffer* inputBuffer, size_t inputBufferSize, bool generateInPlace);

		/// <summary>
		/// Creates a SegmentTreeGenerationKernel for uint32_t buffers with '+' operator
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLoader"> Shader binary loader </param>
		/// <param name="maxInFlightCommandBuffers"> Maximal number of simultinuously running command buffers </param>
		/// <returns> A new instance of SegmentTreeGenerationKernel if successful; nullptr otherwise </returns>
		static Reference<SegmentTreeGenerationKernel> CreateUintSumKernel(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers);

		/// <summary>
		/// Creates a SegmentTreeGenerationKernel for uint32_t buffers with '*' operator
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLoader"> Shader binary loader </param>
		/// <param name="maxInFlightCommandBuffers"> Maximal number of simultinuously running command buffers </param>
		/// <returns> A new instance of SegmentTreeGenerationKernel if successful; nullptr otherwise </returns>
		static Reference<SegmentTreeGenerationKernel> CreateUintProductKernel(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers);

		/// <summary>
		/// Creates a SegmentTreeGenerationKernel for int32_t buffers with '+' operator
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLoader"> Shader binary loader </param>
		/// <param name="maxInFlightCommandBuffers"> Maximal number of simultinuously running command buffers </param>
		/// <returns> A new instance of SegmentTreeGenerationKernel if successful; nullptr otherwise </returns>
		static Reference<SegmentTreeGenerationKernel> CreateIntSumKernel(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers);

		/// <summary>
		/// Creates a SegmentTreeGenerationKernel for int32_t buffers with '*' operator
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLoader"> Shader binary loader </param>
		/// <param name="maxInFlightCommandBuffers"> Maximal number of simultinuously running command buffers </param>
		/// <returns> A new instance of SegmentTreeGenerationKernel if successful; nullptr otherwise </returns>
		static Reference<SegmentTreeGenerationKernel> CreateIntProductKernel(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers);

		/// <summary>
		/// Creates a SegmentTreeGenerationKernel for float buffers with '+' operator
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLoader"> Shader binary loader </param>
		/// <param name="maxInFlightCommandBuffers"> Maximal number of simultinuously running command buffers </param>
		/// <returns> A new instance of SegmentTreeGenerationKernel if successful; nullptr otherwise </returns>
		static Reference<SegmentTreeGenerationKernel> CreateFloatSumKernel(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers);

		/// <summary>
		/// Creates a SegmentTreeGenerationKernel for float buffers with '*' operator
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLoader"> Shader binary loader </param>
		/// <param name="maxInFlightCommandBuffers"> Maximal number of simultinuously running command buffers </param>
		/// <returns> A new instance of SegmentTreeGenerationKernel if successful; nullptr otherwise </returns>
		static Reference<SegmentTreeGenerationKernel> CreateFloatProductKernel(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers);

	private:
		// Graphics device
		const Reference<Graphics::GraphicsDevice> m_device;

		// Respource binding for the result buffer
		const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_resultBufferBinding;

		// Binding pool
		const Reference<Graphics::BindingPool> m_bindingPool;

		// Compute pipeline
		const Reference<Graphics::Experimental::ComputePipeline> m_pipeline;

		// Cached bindings for future binding set creation
		const Reference<const Object> m_cachedBindings;

		// Settings buffer, bound to the pipeline
		const Reference<Graphics::Buffer> m_settingsBuffer;

		// Maximal number of simultinuously running command buffers
		const size_t m_maxInFlightCommandBuffers;

		// Number of threads per workgroup
		const size_t m_workGroupSize;

		// Binding sets
		Stacktor<Reference<Graphics::BindingSet>, 16u> m_bindingSets;

		// Maximal number of iterations currently supported by the existing pipeline
		//size_t m_maxPipelineIterations = 0u;

		// Some private stuff resides here
		struct Helpers;

		// Constructor is private and should be private!
		SegmentTreeGenerationKernel(
			Graphics::GraphicsDevice* device,
			Graphics::ResourceBinding<Graphics::ArrayBuffer>* resultBufferBinding,
			Graphics::BindingPool* bindingPool,
			Graphics::Experimental::ComputePipeline* pipeline,
			const Object* cachedBindings,
			Graphics::Buffer* settingsBuffer,
			size_t maxInFlightCommandBuffers,
			size_t workGroupSize);
	};
}
