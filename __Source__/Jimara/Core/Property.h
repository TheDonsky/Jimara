#pragma once
#include "Function.h"
#include <type_traits>


namespace Jimara {
	/// <summary>
	/// Setter/getter pair (similar to C# Properties)
	/// </summary>
	/// <typeparam name="ValueType"> Property value type </typeparam>
	template<typename ValueType>
	class Property {
	public:
		/// <summary> No matter, if ValueType is a type, a reference or a const reference, assignment will always work with const referenc </summary>
		using AssignedType = const std::remove_const_t<std::remove_reference_t<ValueType>>&;

		/// <summary> If value type is a reference or a const reference, evaluated value will always be a const reference, otherwise a non-const instance will be returned </summary>
		using EvaluatedType = std::conditional_t<std::is_reference_v<ValueType>, AssignedType, std::remove_const_t<ValueType>>;

		/// <summary>
		/// Copy-Constructor
		/// </summary>
		/// <param name=""> Property to copy </param>
		inline Property(const Property&) = default;

		/// <summary>
		/// Move-Constructor
		/// </summary>
		/// <param name=""> Property to move </param>
		inline Property(Property&&) = default;

		/// <summary>
		/// Sets value to that from the other property
		/// </summary>
		/// <param name="value"> Property to set value from </param>
		/// <returns> self </returns>
		Property& operator=(const Property& value) {
			return (*this) = value.operator EvaluatedType();
		}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="target"> Just a pointer to the underlying value </param>
		inline Property(std::remove_const_t<std::remove_reference_t<ValueType>>* target)
			: m_get(Property::Dereference, target)
			, m_set(std::is_const_v<std::remove_reference_t<ValueType>> 
				? Callback<AssignedType>([](AssignedType) {})
				: Callback<AssignedType>(Property::Assign, target)) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="target"> Just a reference to the underlying value </param>
		inline Property(std::remove_const_t<std::remove_reference_t<ValueType>>& target) : Property(&target) {}

		/// <summary>
		/// Constructor (get-only; setter will do nothing)
		/// </summary>
		/// <param name="target"> Just a pointer to the underlying value </param>
		inline Property(const std::remove_const_t<std::remove_reference_t<ValueType>>* target)
			: m_get(Property::Dereference, target)
			, m_set([](AssignedType) { }) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="target"> Just a reference to the underlying value </param>
		inline Property(const std::remove_const_t<std::remove_reference_t<ValueType>>& target) : Property(&target) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <typeparam name="GetFn"> Function<EvaluatedType>/EvaluatedType(*)() </typeparam>
		/// <typeparam name="SetFn"> Callback<AssignedType>/void(*)(AssignedType) </typeparam>
		/// <param name="get"> 'Get' function </param>
		/// <param name="set"> 'Set' function </param>
		template<typename GetFn, typename SetFn>
		inline Property(const GetFn& get, const SetFn& set) : m_get(get), m_set(set) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <typeparam name="GetFn"> EvaluatedType(TargetType::* method)()const/EvaluatedType(TargetType::* method)()/EvaluatedType(*)(TargetType*) </typeparam>
		/// <typeparam name="SetFn"> void(TargetType::* method)(AssignedType)/void(*)(TargetType*, AssignedType) </typeparam>
		/// <typeparam name="TargetType"> Object type </typeparam>
		/// <param name="get"> 'Get' function </param>
		/// <param name="set"> 'Set' function </param>
		/// <param name="target"> UserData/target object (depending if the functions are members or not) </param>
		template<typename GetFn, typename SetFn, typename TargetType>
		inline Property(const GetFn& get, const SetFn& set, TargetType* target) : m_get(get, target), m_set(set, target) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <typeparam name="GetFn"> EvaluatedType(TargetType::* method)()const/EvaluatedType(TargetType::* method)()/EvaluatedType(*)(TargetType*) </typeparam>
		/// <typeparam name="SetFn"> void(TargetType::* method)(AssignedType)/void(*)(TargetType*, AssignedType) </typeparam>
		/// <typeparam name="TargetType"> Object type </typeparam>
		/// <param name="get"> 'Get' function </param>
		/// <param name="set"> 'Set' function </param>
		/// <param name="target"> UserData/target object (depending if the functions are members or not) </param>
		template<typename GetFn, typename SetFn, typename TargetType>
		inline Property(const GetFn& get, const SetFn& set, TargetType& target) : Property(get, set, &target) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <typeparam name="GetFn"> EvaluatedType(TargetType::* method)()const/EvaluatedType(TargetType::* method)()/EvaluatedType(*)(TargetType*) </typeparam>
		/// <typeparam name="SetFn"> void(TargetType::* method)(AssignedType)/void(*)(TargetType*, AssignedType) </typeparam>
		/// <typeparam name="TargetType"> Object type </typeparam>
		/// <param name="get"> 'Get' function </param>
		/// <param name="set"> 'Set' function </param>
		/// <param name="target"> UserData/target object (depending if the functions are members or not) </param>
		template<typename GetFn, typename SetFn, typename TargetType>
		inline Property(const GetFn& get, const SetFn& set, const TargetType* target) : m_get(get, target), m_set(set, target) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <typeparam name="GetFn"> EvaluatedType(TargetType::* method)()const/EvaluatedType(TargetType::* method)()/EvaluatedType(*)(TargetType*) </typeparam>
		/// <typeparam name="SetFn"> void(TargetType::* method)(AssignedType)/void(*)(TargetType*, AssignedType) </typeparam>
		/// <typeparam name="TargetType"> Object type </typeparam>
		/// <param name="get"> 'Get' function </param>
		/// <param name="set"> 'Set' function </param>
		/// <param name="target"> UserData/target object (depending if the functions are members or not) </param>
		template<typename GetFn, typename SetFn, typename TargetType>
		inline Property(const GetFn& get, const SetFn& set, const TargetType& target) : m_get(get, target), m_set(set, target) {}

		/// <summary> Invokes getter and 'type-casts' as the return value </summary>
		inline operator EvaluatedType()const { return m_get(); }

		/// <summary>
		/// Sets property value
		/// </summary>
		/// <param name="value"> New value to assign </param>
		/// <returns> Self </returns>
		inline Property& operator=(AssignedType value) { m_set(value); return (*this); }


	private:
		// Getter
		Function<EvaluatedType> m_get;
		
		// Setter
		Callback<AssignedType> m_set;

		// Basic getter for simple reference case:
		inline static EvaluatedType Dereference(const std::remove_const_t<std::remove_reference_t<ValueType>>* target) { return *target; }

		// Basic setter for simple reference case:
		inline static void Assign(std::remove_const_t<std::remove_reference_t<ValueType>>* target, AssignedType value) { (*target) = value; }
	};

	// A few static checks for sanity
	static_assert(!std::is_same_v<std::remove_reference_t<int&>, int&>);
	static_assert(std::is_same_v<std::remove_reference_t<int>, int>);
	static_assert(std::is_reference_v<int&>);
	static_assert(!std::is_reference_v<int>);
	static_assert(std::is_const_v<const int>);
	static_assert(!std::is_const_v<int>);
	static_assert(std::is_same_v<Property<int>::AssignedType, const int&>);
	static_assert(std::is_same_v<Property<int>::EvaluatedType, int>);
	static_assert(std::is_reference_v<Property<int>::AssignedType>);
	static_assert(std::is_same_v<Property<const int>::AssignedType, const int&>);
	static_assert(std::is_same_v<Property<const int>::EvaluatedType, int>);
	static_assert(std::is_reference_v<Property<const int>::AssignedType>);
	static_assert(std::is_same_v<Property<int&>::AssignedType, const int&>);
	static_assert(std::is_same_v<Property<int&>::EvaluatedType, const int&>);
	static_assert(std::is_reference_v<Property<int&>::AssignedType>);
	static_assert(std::is_same_v<Property<const int&>::AssignedType, const int&>);
	static_assert(std::is_same_v<Property<const int&>::EvaluatedType, const int&>);
	static_assert(std::is_reference_v<Property<const int&>::AssignedType>);
}
