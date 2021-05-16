#pragma once
namespace Jimara { class LightingModel; }
#include "../../../Components/Camera.h"

namespace Jimara {
	class LightingModel : public virtual Object {
	public:
		virtual Reference<Graphics::ImageRenderer> CreateRenderer(Camera* camera) = 0;
	};
}
