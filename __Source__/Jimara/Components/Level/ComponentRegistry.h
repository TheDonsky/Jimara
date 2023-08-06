#pragma once
#include "../Component.h"
#include "../../Data/Registry.h"


namespace Jimara {
	/// <summary> Let the system know registry exists </summary>
	JIMARA_REGISTER_TYPE(Jimara::ComponentRegistry);

	/// <summary>
	/// Registry that is also a scene component
	/// </summary>
	class JIMARA_API ComponentRegistry : public virtual Jimara::Component, public virtual Registry {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		inline ComponentRegistry(Component* parent, const std::string_view& name = "Registry") : Component(parent, name) {}
		
		/// <summary> Virtual destructor </summary>
		inline virtual ~ComponentRegistry() {}
	};

	// Type detail callbacks
	template<> JIMARA_API void TypeIdDetails::GetParentTypesOf<ComponentRegistry>(const Callback<TypeId>& report);
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<ComponentRegistry>(const Callback<const Object*>& report);
}
