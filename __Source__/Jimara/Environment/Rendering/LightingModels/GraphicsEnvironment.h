#pragma once
#include "../SceneObjects/GraphicsObjectDescriptor.h"
#include "../../../Graphics/Data/ShaderBinaries/ShaderSet.h"


namespace Jimara {
	/// <summary>
	/// Helper for creating graphics pipeline descriptors from GraphicsObjectDescriptors that share the same environment
	/// </summary>
	class JIMARA_API GraphicsEnvironment : public virtual Object {
	public:
		/// <summary>
		/// Instantiates GraphicsEnvironment
		/// </summary>
		/// <param name="shaderSet"> Shader set to load binaries from </param>
		/// <param name="environmentBindings"> Environment binding collection </param>
		/// <param name="sampleObject"> Sample object, determine the shape of environment bindings (generally, any object that is compatible, should suffice)  </param>
		/// <param name="device"> Graphics device, the environment should be compatible with </param>
		/// <returns> New instance of a GraphicsEnvironment if successful, nullptr otherwise </returns>
		static Reference<GraphicsEnvironment> Create(
			Graphics::ShaderSet* shaderSet,
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& environmentBindings,
			const GraphicsObjectDescriptor* sampleObject,
			Graphics::GraphicsDevice* device);

		/// <summary>
		/// Instantiates GraphicsEnvironment
		/// </summary>
		/// <param name="shaderSet"> Shader set to load binaries from </param>
		/// <param name="environmentBindings"> Environment binding collection </param>
		/// <param name="environmentBindingSets"> SPIRV_Binary binding sets to base environment shape on (can contain more sets than the environment actually covers) </param>
		/// <param name="environmentBindingSetCount"> Number of entries in environmentBindingSets </param>
		/// <param name="device"> Graphics device, the environment should be compatible with </param>
		/// <returns> New instance of a GraphicsEnvironment if successful, nullptr otherwise </returns>
		static Reference<GraphicsEnvironment> Create(
			Graphics::ShaderSet* shaderSet,
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& environmentBindings,
			const Graphics::ShaderResourceBindings::ShaderModuleBindingSet* environmentBindingSets, size_t environmentBindingSetCount,
			Graphics::GraphicsDevice* device);

		/// <summary>
		/// Creates a new graphics pipeline descriptor, based on a GraphicsObjectDescriptor 
		/// </summary>
		/// <param name="sceneObject"> GraphicsObjectDescriptor </param>
		/// <returns> New instance of a pipeline descriptor if successful, nullptr otherwise </returns>
		Reference<Graphics::GraphicsPipeline::Descriptor> CreateGraphicsPipelineDescriptor(const GraphicsObjectDescriptor* sceneObject);

		/// <summary> Environment descriptor </summary>
		Graphics::PipelineDescriptor* EnvironmentDescriptor()const;


	private:
		// Shader set, binaries are loaded from
		const Reference<Graphics::ShaderSet> m_shaderSet;

		// Environment binding, alongside it's 'Set By Environment' clone
		struct EnvironmentBinding {
			// Environment binding ('Set By Environment' = false; used by EnvironmentDescriptor)
			Reference<Graphics::PipelineDescriptor::BindingSetDescriptor> binding;

			// Environment binding shape ('Set By Environment' = true; used by descriptors, created with CreateGraphicsPipelineDescriptor)
			Reference<Graphics::PipelineDescriptor::BindingSetDescriptor> environmentDescriptor;
		};

		// Environment bindings
		const std::vector<EnvironmentBinding> m_environmentBindings;

		// Graphics device, the output descriptors will be compatible with
		const Reference<Graphics::GraphicsDevice> m_device;

		// Shader cache for Graphics::Shader creation/reuse
		const Reference<Graphics::ShaderCache> m_shaderCache;

		// Environment descriptor
		Reference<Graphics::PipelineDescriptor> m_environmentDecriptor;

		// Constructor
		GraphicsEnvironment(Graphics::ShaderSet* shaderSet, std::vector<EnvironmentBinding>&& environmentBindings, Graphics::GraphicsDevice* device);
	};
}
