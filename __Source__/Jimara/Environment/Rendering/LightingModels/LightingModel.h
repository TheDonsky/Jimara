#pragma once
namespace Jimara { class LightingModel; }
#include "../RenderStack.h"
#include "../ViewportDescriptor.h"
#include <optional>

namespace Jimara {
	/// <summary>
	/// Generic interface, each scene renderer is supposed to implement;
	/// Basically, responsible for turning the data from GraphicsContext into images.
	/// </summary>
	class LightingModel : public virtual Object {
	public:
		/// <summary>
		/// Creates a scene renderer based on the viewport
		/// </summary>
		/// <param name="viewport"> Viewport descriptor </param>
		/// <returns> New instance of a renderer if successful, nullptr otherwise </returns>
		virtual Reference<RenderStack::Renderer> CreateRenderer(const ViewportDescriptor* viewport) = 0;
	};
}
