#pragma once
#include "../LightingModel.h"
#include "../../../../Data/ConfigurableResource.h"


namespace Jimara {
	/// <summary> Let the system know the resource type exists </summary>
	JIMARA_REGISTER_TYPE(Jimara::ForwardPlusLightingModel);

#pragma warning(disable: 4250)
	/// <summary> Forward-plus lighting model </summary>
	class JIMARA_API ForwardPlusLightingModel 
		: public virtual LightingModel
		, public virtual ConfigurableResource {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name=""> Ignored </param>
		inline ForwardPlusLightingModel(ConfigurableResource::CreateArgs = {}) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~ForwardPlusLightingModel() {}

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
#pragma warning(default: 4250)

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<ForwardPlusLightingModel>(const Callback<TypeId>& report) {
		report(TypeId::Of<LightingModel>());
		report(TypeId::Of<ConfigurableResource>());
	}
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<ForwardPlusLightingModel>(const Callback<const Object*>& report);
}
