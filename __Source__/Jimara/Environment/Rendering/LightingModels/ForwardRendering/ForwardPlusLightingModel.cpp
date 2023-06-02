#include "ForwardPlusLightingModel.h"
#include "ForwardLightingModel.h"
#include "ForwardLightingModel_OIT_Pass.h"


namespace Jimara {
	struct ForwardPlusLightingModel::Helpers {
		class Renderer : public virtual RenderStack::Renderer {
		private:
			const Reference<RenderStack::Renderer> m_opaquePass;
			const Reference<RenderStack::Renderer> m_transparentPass;

		public:
			inline Renderer(RenderStack::Renderer* opaquePass, RenderStack::Renderer* transparentPass)
				: m_opaquePass(opaquePass), m_transparentPass(transparentPass) {}

			inline void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) final override {
				m_opaquePass->Render(commandBufferInfo, images);
				m_transparentPass->Render(commandBufferInfo, images);
			}

			inline virtual void GetDependencies(Callback<JobSystem::Job*> report) final override {
				m_opaquePass->GetDependencies(report);
				m_transparentPass->GetDependencies(report);
			}
		};
	};

	ForwardPlusLightingModel* ForwardPlusLightingModel::Instance() {
		static ForwardPlusLightingModel instance;
		return &instance;
	}

	Reference<RenderStack::Renderer> ForwardPlusLightingModel::CreateRenderer(const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags)const {
		if (viewport == nullptr)
			return nullptr;

		auto fail = [&](const auto&... message) {
			viewport->Context()->Log()->Error("ForwardPlusLightingModel::CreateRenderer - ", message...);
			return nullptr;
		};

		const Reference<RenderStack::Renderer> opaquePass = ForwardLightingModel::Instance()->CreateRenderer(
			viewport, layers, flags | Graphics::RenderPass::Flags::RESOLVE_COLOR | Graphics::RenderPass::Flags::RESOLVE_DEPTH);
		if (opaquePass == nullptr)
			return fail("Failed to create render pass for opaque objects! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		
		const Reference<RenderStack::Renderer> transparentPass = ForwardLightingModel_OIT_Pass::Instance()->CreateRenderer(
			viewport, layers, Graphics::RenderPass::Flags::NONE);
		if (transparentPass == nullptr)
			return fail("Failed to create render pass for transparent objects! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		return Object::Instantiate<Helpers::Renderer>(opaquePass, transparentPass);
	}
}

