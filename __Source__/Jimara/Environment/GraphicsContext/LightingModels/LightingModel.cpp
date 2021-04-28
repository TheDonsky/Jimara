#include "LightingModel.h"


namespace Jimara {
	namespace {
		struct LitShaderModules {
			Reference<Graphics::Shader> vertex;
			Reference<Graphics::Shader> fragment;

			LitShaderModules(LightingModel* model, const SceneObjectDescriptor* object) {
				const LightingModel::LitShaderModules modules = model->GetShaderBinaries(object->Material()->ShaderId());
			}
		};

		class ObjectPipelineDescriptor : public virtual Graphics::GraphicsPipeline::Descriptor {
		private:
			const Reference<LightingModel> m_model;
			const Reference<const SceneObjectDescriptor> m_object;
			const LightingModel::LitShaderModules m_modules;

		public:
			inline ObjectPipelineDescriptor(LightingModel* model, const SceneObjectDescriptor* object)
				: m_model(model), m_object(object), m_modules() {

			}

			inline virtual Reference<Graphics::Shader> VertexShader() override {}
			inline virtual Reference<Graphics::Shader> FragmentShader() override {}

			inline virtual size_t VertexBufferCount() override {}

			inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index) override {}

			inline virtual size_t InstanceBufferCount() override {}
			
			inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index) override {}
			
			inline virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer() override {}

			inline virtual size_t IndexCount() override {}

			inline virtual size_t InstanceCount() override {}
		};
	}

	Reference<Graphics::GraphicsPipeline::Descriptor> LightingModel::CreatePipelineDescriptor(const SceneObjectDescriptor* descriptor) {
		// __TODO__: Implement this crap!!
		return nullptr;
	}
}
