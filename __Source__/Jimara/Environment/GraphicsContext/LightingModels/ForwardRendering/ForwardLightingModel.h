#pragma once
#include "../LightingModel.h"


namespace Jimara {
	class ForwardLightingModel : public virtual LightingModel {
	public:
		static Reference<ForwardLightingModel> Instance();

		virtual Reference<Graphics::ImageRenderer> CreateRenderer(const ViewportDescriptor* viewport) override;
	};
}
