#pragma once
#include "RegistryReference.h"
#include "../../Core/Systems/InputProvider.h"


namespace Jimara {
	/// <summary> Let the system know about ComponentFromRegistry </summary>
	JIMARA_REGISTER_TYPE(Jimara::ComponentFromRegistry);

	/// <summary> Let the system know about FieldFromRegistry </summary>
	JIMARA_REGISTER_TYPE(Jimara::FieldFromRegistry);

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

	/// <summary>
	/// Component, that observes registry changes ans assigns it to other Component's field based on the field name
	/// </summary>
	class JIMARA_API FieldFromRegistry : public virtual ReferenceInputFromRegistry<Object> {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Name </param>
		FieldFromRegistry(Component* parent, const std::string_view& name = "FieldFromRegistry");

		/// <summary> Virtual destructor </summary>
		virtual ~FieldFromRegistry();

		/// <summary> 
		/// Parent Component's Object Reference field of this name will be linked to the registry entry 
		/// <para/> Keep in mind, that actual linked field changes may be delayed by a frame for some safety reasons;
		/// <para/> Currently, 'nested' fields are not supported.
		/// </summary>
		inline const std::string& FieldName()const { return m_fieldName; }

		/// <summary>
		/// Changes target field name
		/// </summary>
		/// <param name="name"> Field name to use </param>
		void SetFieldName(const std::string_view& name);

		/// <summary> If set, this flag will also allow FieldFromRegistry to clear parent fields when the registry has no entry </summary>
		inline bool ClearIfNull()const { return m_assignIfNull; }

		/// <summary>
		/// Sets AssignIfNull flag
		/// </summary>
		/// <param name="assign"> If set, this flag will also allow FieldFromRegistry to clear parent fields when the registry has no entry </param>
		void ClearIfNull(bool assign);

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

	private:
		std::atomic_uint64_t m_scheduledCounter = 0u;
		std::string m_fieldName;
		std::atomic_bool m_assignIfNull = false;

		struct Helpers;
		inline std::string_view GetFieldName()const { return m_fieldName; } // For serialization..
	};
#pragma warning(default: 4250)

	// Type detail callbacks
	template<> JIMARA_API void TypeIdDetails::GetParentTypesOf<ComponentFromRegistry>(const Callback<TypeId>& report);
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<ComponentFromRegistry>(const Callback<const Object*>& report);
	template<> JIMARA_API void TypeIdDetails::GetParentTypesOf<FieldFromRegistry>(const Callback<TypeId>& report);
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<FieldFromRegistry>(const Callback<const Object*>& report);
}
