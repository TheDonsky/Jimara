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
		/// <returns> A new instance of a forward renderer </returns>
		virtual Reference<Scene::GraphicsContext::Renderer> CreateRenderer(const ViewportDescriptor* viewport) override;
	};
}
