#pragma once
#include "../LightingModel.h"


namespace Jimara {
	/// <summary>
	/// Opaque geometry pass for Forward plus lighting model.
	/// </summary>
	class JIMARA_API ForwardLightingModel_Opaque_Pass : public virtual LightingModel {
	public:
		/// <summary> Singleton instance </summary>
		static ForwardLightingModel_Opaque_Pass* Instance();

		/// <summary>
		/// Creates a forward plus renderer's opaque pass
		/// </summary>
		/// <param name="viewport"> Render viewport descriptor </param>
		/// <param name="layers"> Rendered layer mask </param>
		/// <param name="flags"> Clear/Resolve flags </param>
		/// <returns> A new instance of a forward renderer </returns>
		virtual Reference<RenderStack::Renderer> CreateRenderer(const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags)const override;
	
	private:
		// Private stuff resides in here
		struct Helpers;
	};
}
