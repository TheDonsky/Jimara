#pragma once
#include "../LightingModel.h"


namespace Jimara {
	class ObjectIdRenderer : public virtual LightingModel {
	public:
		/// <summary> Singleton instance </summary>
		static Reference<ObjectIdRenderer> Instance();

		/// <summary>
		/// Creates an object id renderer
		/// </summary>
		/// <param name="viewport"> Render viewport descriptor </param>
		/// <returns> A new instance of an object id renderer </returns>
		virtual Reference<Scene::GraphicsContext::Renderer> CreateRenderer(const ViewportDescriptor* viewport) override;
	};
}
