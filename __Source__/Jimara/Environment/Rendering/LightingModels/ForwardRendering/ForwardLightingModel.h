#pragma once
#include "../LightingModel.h"


namespace Jimara {
	/// <summary>
	/// Forward lighting model.
	/// Basically, all objects get illuminated by all light sources without any light culling
	/// </summary>
	class ForwardLightingModel : public virtual LightingModel {
	public:
		/// <summary> Singleton instance </summary>
		static Reference<ForwardLightingModel> Instance();

		/// <summary>
		/// Creates a forward renderer
		/// </summary>
		/// <param name="viewport"> Render viewport descriptor </param>
		/// <param name="layers"> Rendered layer mask </param>
		/// <returns> A new instance of a forward renderer </returns>
		virtual Reference<RenderStack::Renderer> CreateRenderer(const ViewportDescriptor* viewport, GraphicsLayerMask layers) override;
	};
}
