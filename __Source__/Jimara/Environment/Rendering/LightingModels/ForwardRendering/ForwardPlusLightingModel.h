#pragma once
#include "../LightingModel.h"


namespace Jimara {
	/// <summary> Forward-plus lighting model </summary>
	class JIMARA_API ForwardPlusLightingModel : public virtual LightingModel {
	public:
		/// <summary> Singleton instance </summary>
		static ForwardPlusLightingModel* Instance();

		/// <summary>
		/// Creates a forward-plus renderer
		/// </summary>
		/// <param name="viewport"> Render viewport descriptor </param>
		/// <param name="layers"> Rendered layer mask </param>
		/// <param name="flags"> Clear/Resolve flags </param>
		/// <returns> A new instance of a forward renderer </returns>
		virtual Reference<RenderStack::Renderer> CreateRenderer(const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags)const override;

	private:
		// Private stuff resides here
		struct Helpers;
	};
}
