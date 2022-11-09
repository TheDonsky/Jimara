#pragma once
#include "../../../../Graphics/GraphicsDevice.h"
#include "../../../../Graphics/Data/ShaderBinaries/ShaderLoader.h"
#include "../../../../Graphics/Data/ShaderBinaries/ShaderResourceBindings.h"


namespace Jimara {
	/// <summary>
	/// General implementation of arbitrary Bitonic Merge Sort kernels
	/// <para/> Read BitonicSort.glh, for definitions and BitonicSort_Floats_AnySize.comp and BitonicSort_Floats_SingleStep.comp for example usage
	/// </summary>
	class JIMARA_API BitonicSortKernel : public virtual Object {
	public:
		/// <summary>
		/// Creates a bitonic sort kernel
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="bindings"> Shader resource bindings </param>
		/// <param name="maxInFlightCommandBuffers"> Maximal number of simultinuously running command buffers </param>
		/// <param name="workGroupSize"> Number of comparators per workgroup </param>
		/// <param name="singleStepShader"> Shader module for single step 
		///		(can not be null, expected to implement GetBitonicSortPair() pattern similarily to the float example case, 
		///		but can be the same as groupsharedShader if you feel lazy) </param>
		/// <param name="groupsharedShader"> Optional shader module for groupshared step group 
		///		(can be null, but providing this decreses the amount of dispatches and makes the thing run faster; 
		///		if provided, expectation is to use BitonicSortGroupsharedKernel() like the float example case) </param>
		/// <param name="bitonicSortSettingsName"> Name of the BitonicSortSettings constant buffer attachment within the shaders </param>
		/// <returns> New instance of a BitonicSortKernel </returns>
		static Reference<BitonicSortKernel> Create(
			Graphics::GraphicsDevice* device,
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& bindings,
			size_t maxInFlightCommandBuffers, size_t workGroupSize,
			Graphics::Shader* singleStepShader, Graphics::Shader* groupsharedShader = nullptr,
			const std::string_view& bitonicSortSettingsName = "bitonicSortSettings");

		/// <summary> Virtual destructor </summary>
		virtual ~BitonicSortKernel();

		/// <summary>
		/// Executes BitonicSort kernels
		/// <para/> Note: elemCount should be a power of 2; If it is not, a warning will be logged. 
		///		It is UP TO THE USER to make sure the list is of a supported size and if it is not, 
		///		one is adviced to create a temporary buffer with the size of the smallest power of 2
		///		greater than the target list size that has 'infinities' appended to it at the end.
		/// </summary>
		/// <param name="commandBuffer"> Command buffer and in-flight index </param>
		/// <param name="elemCount"> Number of elements to sort </param>
		void Execute(const Graphics::Pipeline::CommandBufferInfo& commandBuffer, size_t elemCount);

		/// <summary>
		/// Creates a bitonic sort kernel that can sort floating point array buffers
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLoader"> Shader loader </param>
		/// <param name="maxInFlightCommandBuffers"> Maximal number of simultinuously running command buffers </param>
		/// <param name="binding"> Binding for floating point buffers (bound buffer is supposed to hold floating points) </param>
		/// <returns> New instance of float sorting kernel </returns>
		static Reference<BitonicSortKernel> CreateFloatSortingKernel(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader, size_t maxInFlightCommandBuffers,
			const Graphics::ShaderResourceBindings::StructuredBufferBinding* binding);

	private:
		// Kernel configuration for a single step for Bitonic Sort network
		struct BitonicSortSettings {
			// sequenceSize = (1 << sequenceSizeBit)
			alignas(4) uint32_t sequenceSizeBit;      // 1;	2;		3;			...

			// comparizonStep = (1 << comparizonStepBit)
			alignas(4) uint32_t comparizonStepBit;    // 0;	1, 0;	2, 1, 0;    ...
		};
		static_assert(sizeof(BitonicSortSettings) == 8);
		
		// Graphics device
		const Reference<Graphics::GraphicsDevice> m_device;

		// Pipeline descriptor for single step
		const Reference<Graphics::ComputePipeline::Descriptor> m_singleStepPipelineDescriptor;

		// Pipeline descriptor for groupshared step
		const Reference<Graphics::ComputePipeline::Descriptor> m_groupsharedPipelineDescriptor;

		// Settings constant buffer
		const Graphics::BufferReference<BitonicSortSettings> m_settingsBuffer;

		// Kernel size shared betweeen pipeline descriptors
		const std::shared_ptr<Size3> m_kernelSize;

		// Maximal number of simultinuously running command buffers
		const size_t m_maxInFlightCommandBuffers;

		// Number of elements per workgroup
		const size_t m_workGroupSize;

		// Logarithm of max supported element counts per pipeline (may increase during Execute())
		std::atomic<uint32_t> m_maxListSizeBit = 0u;

		// Single step pipeline (gets recreated during Execute() if m_maxListSizeBit increases)
		Reference<Graphics::ComputePipeline> m_singleStepPipeline;

		// Groupshared step pipeline (may be the same as m_singleStepPipeline)
		Reference<Graphics::ComputePipeline> m_groupsharedPipeline;

		// Private stuff resides here
		struct Helpers;

		// Constructor Shall be private
		BitonicSortKernel(
			Graphics::GraphicsDevice* device,
			Graphics::ComputePipeline::Descriptor* singleStepPipelineDescriptor, 
			Graphics::ComputePipeline::Descriptor* groupsharedPipelineDescriptor,
			const Graphics::BufferReference<BitonicSortSettings>& settingsBuffer,
			const std::shared_ptr<Size3>& kernelSize,
			size_t maxInFlightCommandBuffers, size_t workGroupSize);
	};
}
