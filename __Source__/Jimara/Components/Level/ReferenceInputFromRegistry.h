#pragma once
#include "RegistryReference.h"
#include "../../Core/Systems/InputProvider.h"


namespace Jimara {
	/// <summary> Let the system know about ComponentFromRegistry </summary>
	JIMARA_REGISTER_TYPE(Jimara::ComponentFromRegistry);

#pragma warning(disable: 4250)
	/// <summary>
	/// Generic object reference input from registry
	/// </summary>
	/// <typeparam name="Type"> Input/Reference type </typeparam>
	template<typename Type>
	class ReferenceInputFromRegistry
		: public virtual RegistryReference<Type>
		, public virtual InputProvider<Reference<Type>> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Name </param>
		inline ReferenceInputFromRegistry(Component* parent, const std::string_view& name = TypeId::Of<ReferenceInputFromRegistry>().Name()) 
			: Component(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~ReferenceInputFromRegistry() {}

		/// <summary>
		/// Provides StoredObject as "input" value;
		/// </summary>
		/// <returns> Stored Object </returns>
		inline virtual std::optional<Reference<Type>> GetInput() override { 
			const Reference<Type> item = RegistryReference<Type>::StoredObject();
			if (item == nullptr)
				return std::nullopt;
			else return item;
		}
	};

	/// <summary>
	/// Type details for ReferenceInputFromRegistry<Type>
	/// </summary>
	/// <typeparam name="Type"> Referenced type </typeparam>
	template<typename Type>
	struct TypeIdDetails::TypeDetails<ReferenceInputFromRegistry<Type>> {
		/// <summary>
		/// Reports parent type (Component)
		/// </summary>
		/// <param name="reportParentType"> Type will be reported through this </param>
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<RegistryReference<Type>>());
			reportParentType(TypeId::Of<InputProvider<Reference<Type>>>());
		}
		/// <summary> Reports attributes </summary>
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Component reference input from Registry
	/// </summary>
	class JIMARA_API ComponentFromRegistry : public virtual ReferenceInputFromRegistry<Component> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Name </param>
		inline ComponentFromRegistry(Component* parent, const std::string_view& name = "ComponentFromRegistry")
			: Component(parent, name), ReferenceInputFromRegistry<Component>(parent, name) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~ComponentFromRegistry() {}
	};
#pragma warning(default: 4250)

	// Type detail callbacks
	template<> JIMARA_API void TypeIdDetails::GetParentTypesOf<ComponentFromRegistry>(const Callback<TypeId>& report);
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<ComponentFromRegistry>(const Callback<const Object*>& report);
}
