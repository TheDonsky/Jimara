#include "ForwardLightingModel.h"
#include "../GraphicsEnvironment.h"
#include "../../Lights/LightDataBuffer.h"
#include "../../Lights/LightTypeIdBuffer.h"


namespace Jimara {
	namespace {
		class ForwardRenderer : public virtual Graphics::ImageRenderer {
		private:
			const Reference<const LightingModel::ViewportDescriptor> m_viewport;
			const Reference<const LightDataBuffer> m_lightDataBuffer;
			const Reference<const LightTypeIdBuffer> m_lightTypeIdBuffer;
			Reference<GraphicsEnvironment> m_environment;
			
		public:
			inline ForwardRenderer(const LightingModel::ViewportDescriptor* viewport) 
				: m_viewport(viewport)
				, m_lightDataBuffer(LightDataBuffer::Instance(viewport->Context()))
				, m_lightTypeIdBuffer(LightTypeIdBuffer::Instance(viewport->Context())) {
				// __TODO__: Implement this crap!
			}

			inline virtual ~ForwardRenderer() {
				// __TODO__: Implement this crap!
			}

			inline virtual Reference<Object> CreateEngineData(Graphics::RenderEngineInfo* engineInfo) {
				// __TODO__: Implement this crap!
				return nullptr;
			}

			inline virtual void Render(Object* engineData, Graphics::Pipeline::CommandBufferInfo bufferInfo) {
				// __TODO__: Implement this crap!
			}
		};
	}

	Reference<ForwardLightingModel> ForwardLightingModel::Instance() {
		static ForwardLightingModel model;
		return &model;
	}

	Reference<Graphics::ImageRenderer> ForwardLightingModel::CreateRenderer(const ViewportDescriptor* viewport) {
		return Object::Instantiate<ForwardRenderer>(viewport);
	}
}
