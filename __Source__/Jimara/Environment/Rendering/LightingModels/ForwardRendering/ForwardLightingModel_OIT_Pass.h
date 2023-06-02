#pragma once
#include "../LightingModel.h"


namespace Jimara {
	/// <summary>
	/// Order-independent forward+ renderer for transparent objects.
	/// <para/> Notes:
	/// <para/>		0. This is a part of a regular forward-plus renderer and only renders transparent objects;
	/// <para/>		1. OIT Pass expects color to be cleared before it, as well as the opaque geometry to be rendered wih depth written before it starts.
	/// </summary>
	class ForwardLightingModel_OIT_Pass : public virtual LightingModel {
	public:
		/// <summary> Singleton instance </summary>
		static const ForwardLightingModel_OIT_Pass* Instance();

		/// <summary>
		/// Creates an order-independent transparent object renderer
		/// </summary>
		/// <param name="viewport"> Render viewport descriptor </param>
		/// <param name="layers"> Rendered layer mask </param>
		/// <param name="flags"> Clear/Resolve flags </param>
		/// <returns> A new instance of a forward renderer </returns>
		virtual Reference<RenderStack::Renderer> CreateRenderer(const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags)const override;

		/// <summary> 
		/// Transparent sample transparent count per pixel 
		/// <para/> Notes: 
		/// <para/>		0. Higher the better, but at the expense of VRAM and performance;
		/// <para/>		1. On a per-pixel basis, if actual fragment count exceeds SamplesPerPixel(), collective transparency will be approximated;
		/// <para/>		2. This pass DOES NOT SUPPORT hardware multisampling; do not confuse this parameter with that.
		/// </summary>
		inline uint32_t SamplesPerPixel()const { return m_samplesPerPixel.load(); }

		/// <summary>
		/// Sets transparent sample count per pixel
		/// </summary>
		/// <param name="count"> Sample count </param>
		inline void SetSamplesPerPixel(uint32_t count) { m_samplesPerPixel.store(Math::Max(count, 1u)); }

	private:
		// Samples per pixel
		std::atomic<uint32_t> m_samplesPerPixel = 4u;

		// Private functionality and most of the implementation resides here
		struct Helpers;
	};
}
