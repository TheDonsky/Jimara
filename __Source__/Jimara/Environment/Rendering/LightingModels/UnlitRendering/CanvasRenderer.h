#pragma once
#include "../LightingModel.h"
#include "../../../../Components/UI/Canvas.h"


namespace Jimara {
	namespace UI {
		/// <summary>
		/// Renderer for UI Canvas Components
		/// </summary>
		class CanvasRenderer {
		public:
			/// <summary>
			/// Creates UI renderer (used internally by Canvas)
			/// </summary>
			/// <param name="canvas"> Target Canvas </param>
			/// <returns> New instance of an UI renderer </returns>
			static Reference<RenderStack::Renderer> CreateFor(const Canvas* canvas);

		private:
			// Constructor is private:
			inline CanvasRenderer() {}

			// Actual implementation is hidden behind Helpers struct
			struct Helpers;
		};
	}
}
