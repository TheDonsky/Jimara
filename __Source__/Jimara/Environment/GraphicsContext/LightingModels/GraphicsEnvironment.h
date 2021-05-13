#pragma once
#include "../SceneObjects/SceneObjectDescriptor.h"
#include "../../../Graphics/Data/ShaderBinaries/ShaderSet.h"


namespace Jimara {
	class GraphicsEnvironment : public virtual Object {
	public:
		static Reference<GraphicsEnvironment> Create(
			Graphics::ShaderSet* shaderSet,
			const Graphics::ShaderResourceBindings::ShaderResourceBindingSet& environmentBindings,
			const Graphics::ShaderResourceBindings::ShaderModuleBindingSet* environmentBindingSets, size_t environmentBindingSetCount,
			Graphics::GraphicsDevice* device);

		Reference<Graphics::GraphicsPipeline::Descriptor> CreateGraphicsPipelineDescriptor(const SceneObjectDescriptor* sceneObject);

		Graphics::PipelineDescriptor* EnvironmentDescriptor()const;

	private:
		const Reference<Graphics::ShaderSet> m_shaderSet;
		struct EnvironmentBinding {
			Reference<Graphics::PipelineDescriptor::BindingSetDescriptor> binding;
			Reference<Graphics::PipelineDescriptor::BindingSetDescriptor> environmentDescriptor;
		};
		const std::vector<EnvironmentBinding> m_environmentBindings;
		const Reference<Graphics::GraphicsDevice> m_device;
		const Reference<Graphics::ShaderCache> m_shaderCache;
		Reference<Graphics::PipelineDescriptor> m_environmentDecriptor;

		GraphicsEnvironment(Graphics::ShaderSet* shaderSet, std::vector<EnvironmentBinding>&& environmentBindings, Graphics::GraphicsDevice* device);
	};
}
