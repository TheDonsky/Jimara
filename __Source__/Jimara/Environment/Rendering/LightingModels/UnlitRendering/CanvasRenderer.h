#pragma once
#include "../LightingModel.h"
#include "../../../../Components/UI/Canvas.h"


namespace Jimara {
	namespace UI {
		class CanvasRenderer {
		public:
			static Reference<RenderStack::Renderer> CreateFor(const Canvas* canvas);

		private:
			inline CanvasRenderer() {}

			struct Helpers;
		};
	}
}
