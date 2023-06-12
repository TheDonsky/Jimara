#pragma once
#include "../../Graphics/GraphicsDevice.h"
#include "../../Graphics/Data/ShaderBinaries/ShaderLoader.h"


namespace Jimara {
	/// <summary>
	/// A simple wrapper for a compute shader, alongside it's bindings
	/// </summary>
	class JIMARA_API SimpleComputeKernel : public virtual Object {
	public:
		/// <summary>
		/// Creates an instance of a SimpleComputeKernel
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLoader"> Shader bytecode loader </param>
		/// <param name="bindingPool"> Binding pool (optionally, one can create internal binding pool using the override and maxInFlightCommandBufers) </param>
		/// <param name="computeShader"> Address of the compute shader </param>
		/// <param name="bindings"> Binding search functions </param>
		/// <returns> New instance of a SimpleComputeKernel </returns>
		static Reference<SimpleComputeKernel> Create(
			Graphics::GraphicsDevice* device,
			Graphics::ShaderLoader* shaderLoader,
			Graphics::BindingPool* bindingPool,
			const Graphics::ShaderClass* computeShader,
			const Graphics::BindingSet::BindingSearchFunctions& bindings);

		/// <summary>
		/// Creates an instance of a SimpleComputeKernel
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="shaderLoader"> Shader bytecode loader </param>
		/// <param name="maxInFlightCommandBuffers"> Maximal number of simultanuous in-flight command buffers (optionally, one can use a pre-created binding pool instead) </param>
		/// <param name="computeShader"> Address of the compute shader </param>
		/// <param name="bindings"> Binding search functions </param>
		/// <returns> New instance of a SimpleComputeKernel </returns>
		static Reference<SimpleComputeKernel> Create(
			Graphics::GraphicsDevice* device, 
			Graphics::ShaderLoader* shaderLoader,
			size_t maxInFlightCommandBuffers,
			const Graphics::ShaderClass* computeShader,
			const Graphics::BindingSet::BindingSearchFunctions& bindings);

		/// <summary> Virtual destructor </summary>
		virtual ~SimpleComputeKernel();

		/// <summary>
		/// Updates binding sets, binds them and dispatches the pipeline
		/// </summary>
		/// <param name="commandBuffer"> In-flight command buffer info </param>
		/// <param name="workgroupCount"> Number of workgroups </param>
		void Dispatch(const Graphics::InFlightBufferInfo& commandBuffer, const Size3& workgroupCount)const;

	private:
		// Constructor is private!
		SimpleComputeKernel();

		// Actual implementation resides in here...
		struct Helpers;
	};
}
