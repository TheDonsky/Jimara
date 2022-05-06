#pragma once
namespace Jimara { class LightingModel; }
#include "../RenderStack.h"
#include "../ViewportDescriptor.h"
#include "../SceneObjects/GraphicsLayer.h"
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
		/// <param name="layers"> Rendered layer mask </param>
		/// <param name="flags"> Clear/Resolve flags </param>
		/// <returns> New instance of a renderer if successful, nullptr otherwise </returns>
		virtual Reference<RenderStack::Renderer> CreateRenderer(const ViewportDescriptor* viewport, GraphicsLayerMask layers, Graphics::RenderPass::Flags flags) = 0;
	};
}
