#pragma once
#include "Function.h"


namespace Jimara {
	/// <summary>
	/// Setter/getter pair (similar to C# Properties)
	/// </summary>
	/// <typeparam name="ValueType"> Property value type </typeparam>
	template<typename ValueType>
	class Property {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="target"> Just a pointer to the underlying value </param>
		inline Property(ValueType* target)
			: m_get(Property::Dereference, target)
			, m_set(Property::Assign, target) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="target"> Just a reference to the underlying value </param>
		inline Property(ValueType& target) : Property(&target) {}

		/// <summary>
		/// Constructor (get-only; setter will do nothing)
		/// </summary>
		/// <param name="target"> Just a pointer to the underlying value </param>
		inline Property(const ValueType* target)
			: m_get(Property::Dereference, target)
			, m_set([](const ValueType&) { }) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <typeparam name="GetFn"> Function<ValueType>/ValueType(*)() </typeparam>
		/// <typeparam name="SetFn"> Callback<const ValueType&>/void(*)(const ValueType&) </typeparam>
		/// <param name="get"> 'Get' function </param>
		/// <param name="set"> 'Set' function </param>
		template<typename GetFn, typename SetFn>
		inline Property(const GetFn& get, const SetFn& set) : m_get(get), m_set(set) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <typeparam name="GetFn"> ValueType(TargetType::* method)()const/ValueType(TargetType::* method)()/ValueType(*)(TargetType*) </typeparam>
		/// <typeparam name="SetFn"> void(TargetType::* method)(const ValueType&)/void(*)(TargetType*, const ValueType&) </typeparam>
		/// <typeparam name="TargetType"> Object type </typeparam>
		/// <param name="get"> 'Get' function </param>
		/// <param name="set"> 'Set' function </param>
		/// <param name="target"> UserData/target object (depending if the functions are members or not) </param>
		template<typename GetFn, typename SetFn, typename TargetType>
		inline Property(const GetFn& get, const SetFn& set, TargetType* target) : m_get(get, target), m_set(set, target) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <typeparam name="GetFn"> ValueType(TargetType::* method)()const/ValueType(TargetType::* method)()/ValueType(*)(TargetType*) </typeparam>
		/// <typeparam name="SetFn"> void(TargetType::* method)(const ValueType&)/void(*)(TargetType*, const ValueType&) </typeparam>
		/// <typeparam name="TargetType"> Object type </typeparam>
		/// <param name="get"> 'Get' function </param>
		/// <param name="set"> 'Set' function </param>
		/// <param name="target"> UserData/target object (depending if the functions are members or not) </param>
		template<typename GetFn, typename SetFn, typename TargetType>
		inline Property(const GetFn& get, const SetFn& set, TargetType& target) : Property(get, set, &target) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <typeparam name="GetFn"> ValueType(TargetType::* method)()const/ValueType(TargetType::* method)()/ValueType(*)(TargetType*) </typeparam>
		/// <typeparam name="SetFn"> void(TargetType::* method)(const ValueType&)/void(*)(TargetType*, const ValueType&) </typeparam>
		/// <typeparam name="TargetType"> Object type </typeparam>
		/// <param name="get"> 'Get' function </param>
		/// <param name="set"> 'Set' function </param>
		/// <param name="target"> UserData/target object (depending if the functions are members or not) </param>
		template<typename GetFn, typename SetFn, typename TargetType>
		inline Property(const GetFn& get, const SetFn& set, const TargetType* target) : m_get(get, target), m_set(set, target) {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <typeparam name="GetFn"> ValueType(TargetType::* method)()const/ValueType(TargetType::* method)()/ValueType(*)(TargetType*) </typeparam>
		/// <typeparam name="SetFn"> void(TargetType::* method)(const ValueType&)/void(*)(TargetType*, const ValueType&) </typeparam>
		/// <typeparam name="TargetType"> Object type </typeparam>
		/// <param name="get"> 'Get' function </param>
		/// <param name="set"> 'Set' function </param>
		/// <param name="target"> UserData/target object (depending if the functions are members or not) </param>
		template<typename GetFn, typename SetFn, typename TargetType>
		inline Property(const GetFn& get, const SetFn& set, const TargetType& target) : m_get(get, target), m_set(set, target) {}

		/// <summary> Invokes getter and 'type-casts' as the return value </summary>
		inline operator ValueType()const { return m_get(); }

		/// <summary>
		/// Sets property value
		/// </summary>
		/// <param name="value"> New value to assign </param>
		/// <returns> Self </returns>
		inline Property& operator=(const ValueType& value) { m_set(value); return (*this); }


	private:
		// Getter
		Function<ValueType> m_get;
		
		// Setter
		Callback<const ValueType&> m_set;

		// Basic getter for simple reference case:
		inline static ValueType Dereference(const ValueType* target) { return *target; }

		// Basic setter for simple reference case:
		inline static void Assign(ValueType* target, const ValueType& value) { (*target) = value; }
	};
}
