#pragma once
#include "../LightingModel.h"
#include "../../../../Data/ConfigurableResource.h"


namespace Jimara {
	/// <summary> Let the system know the resource type exists </summary>
	JIMARA_REGISTER_TYPE(Jimara::RayTracedRenderer);

#pragma warning(disable: 4250)
	/// <summary>
	/// Ray-traced lighting model
	/// </summary>
	class JIMARA_API RayTracedRenderer
		: public virtual LightingModel
		, public virtual ConfigurableResource {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name=""> Ignored </param>
		inline RayTracedRenderer(ConfigurableResource::CreateArgs = {}) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~RayTracedRenderer() {}

		/// <summary>
		/// Creates a ray-traced renderer
		/// </summary>
		/// <param name="viewport"> Render viewport descriptor </param>
		/// <param name="layers"> Rendered layer mask </param>
		/// <param name="flags"> Clear/Resolve flags </param>
		/// <returns> A new instance of a renderer </returns>
		virtual Reference<RenderStack::Renderer> CreateRenderer(const ViewportDescriptor* viewport, LayerMask layers, Graphics::RenderPass::Flags flags)const override;

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer should be reported by invoking this callback with serializer & corresonding target as parameters </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement) override;


	private:
		// Private stuff resides here
		struct Helpers;
	};
#pragma warning(default: 4250)


	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<RayTracedRenderer>(const Callback<TypeId>& report) {
		report(TypeId::Of<LightingModel>());
		report(TypeId::Of<ConfigurableResource>());
	}
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<RayTracedRenderer>(const Callback<const Object*>& report);
}
