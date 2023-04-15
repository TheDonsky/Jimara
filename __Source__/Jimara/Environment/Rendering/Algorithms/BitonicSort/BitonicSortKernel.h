#pragma once
#include "../CachedGraphicsBindings.h"
#include "../../../../Graphics/Data/ShaderBinaries/ShaderLoader.h"


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
			const Graphics::BindingSet::BindingSearchFunctions& bindings,
			size_t maxInFlightCommandBuffers, size_t workGroupSize,
			Graphics::SPIRV_Binary* singleStepShader, 
			Graphics::SPIRV_Binary* groupsharedShader = nullptr,
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
		void Execute(const Graphics::InFlightBufferInfo& commandBuffer, size_t elemCount);

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
			const Graphics::ResourceBinding<Graphics::ArrayBuffer>* binding);

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

		// Pipeline for single step
		const Reference<Graphics::ComputePipeline> m_singleStepPipeline;

		// Pipeline for groupshared step
		const Reference<Graphics::ComputePipeline> m_groupsharedPipeline;

		// Cached bindings for single step pipeline
		const Reference<CachedGraphicsBindings> m_singleStepBindings;

		// Cached bindings for groupshared step
		const Reference<CachedGraphicsBindings> m_groupsharedStepBindings;

		// Binding pool
		const Reference<Graphics::BindingPool> m_bindingPool;

		// Settings constant buffer
		const Graphics::BufferReference<BitonicSortSettings> m_settingsBuffer;

		// Maximal number of simultinuously running command buffers
		const size_t m_maxInFlightCommandBuffers;

		// Number of elements per workgroup
		const size_t m_workGroupSize;

		// Binding sets allocated for single step pipeline
		Stacktor<Reference<Graphics::BindingSet>, 16u> m_singleStepBindingSets;

		// Binding steps allocated for groupshared step
		Stacktor<Reference<Graphics::BindingSet>, 4u> m_groupsharedStepBindingSets;

		// Constructor Shall be private
		BitonicSortKernel(
			Graphics::GraphicsDevice* device,
			Graphics::ComputePipeline* singleStepPipeline,
			Graphics::ComputePipeline* groupsharedPipeline,
			CachedGraphicsBindings* singleStepBindings,
			CachedGraphicsBindings* groupsharedStepBindings,
			Graphics::BindingPool* bindingPool,
			const Graphics::BufferReference<BitonicSortSettings>& settingsBuffer,
			size_t maxInFlightCommandBuffers, size_t workGroupSize);
	};
}
