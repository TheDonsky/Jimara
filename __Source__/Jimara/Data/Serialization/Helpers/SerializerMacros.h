#pragma once
#include "../DefaultSerializer.h"
#include <type_traits>

static_assert([]() -> bool {
	int val = 0;
	int& ref = val;
	int* ptr = &val;
	std::remove_pointer_t<decltype(&ref)> valFromRef = val;
	std::remove_pointer_t<decltype(ptr)> valFromPtr = val;
	const constexpr bool refTypeOK = std::is_same_v<decltype(val), decltype(valFromRef)>;
	const constexpr bool ptrTypeOK = std::is_same_v<decltype(val), decltype(valFromPtr)>;
	return refTypeOK && ptrTypeOK;
	}());

/// <summary>
/// When serializing an object with multiple fields, this macro can greatly simplify the process by providing a context in which 
/// you will not need to manually create any field serializers.
/// <para/> Use case would be something along this line:
/// <para/>		JIMARA_SERIALIZE_FIELDS(targetObject, recordElement, {
///	<para/>			JIMARA_SERIALIZE_FIELD(targetObject->m_someValue0, "Value0 name", "Hint to what the value means", attr0, attr1, ...);
/// <para/>			JIMARA_SERIALIZE_FIELD_GET_SET(GetVal1, SetVal1, "Value1 name", "Hint to what the second value", attr0, attr1, ...);
/// <para/>			JIMARA_SERIALIZE_FIELD_CUSTOM(targetObject->m_someStruct, SomeStriuctSerializerType, serializer constructor args...);
/// <para/>			...
/// <para/>		});
/// <para/> Note: JSM_Body can have any code in it, as long as no variable or function name starts with JSM_ 
///		(that means Jimara Serialization macro and is kind of reserved for internal usage)
/// </summary>
/// <param name="JSM_Target"> Target object pointer </param>
/// <param name="JSM_Report"> Callback(SerializedObject) from SerializerList or an equivalent </param>
/// <param name="JSM_Body"> List of JIMARA_SERIALIZE_FIELD commands, alongside any arbitarry code in between </param>
#define JIMARA_SERIALIZE_FIELDS(JSM_Target, JSM_Report, JSM_Body) { \
	typedef std::remove_pointer_t<decltype(JSM_Target)> JSM_Target_T; \
	JSM_Target_T& JSM_Target_Ref = *JSM_Target; \
	const Callback<Serialization::SerializedObject>& JSM_Report_Callback(JSM_Report); \
	JSM_Body; \
	}


/// <summary>
/// Creates a static constant ValueSerializer and serializes given reference with it
/// <para/> Notes: 
/// <para/>		0. This will only work if used inside JSM_Body of JIMARA_SERIALIZE_FIELDS macro;
/// <para/>		1. JSM_ValueReference should be a direct reference to any type, whose DefaultSerializer is provided or overriden;
/// <para/>		2. Example usage would be: JIMARA_SERIALIZE_FIELD(JSM_Target_Ref.m_val, "Value", "Value description", SerializerAttributes...).
/// </summary>
/// <param name="JSM_ValueReference"> Reference to the value to be serialized </param>
/// <param name="JSM_ValueName"> Name of the serialized field </param>
/// <param name="JSM_ValueHint"> Serialized field description </param>
/// <param name="__VA_ARGS__"> List of serializer attribute instances (references can be created inline, as well as statically) </param>
#define JIMARA_SERIALIZE_FIELD(JSM_ValueReference, JSM_ValueName, JSM_ValueHint, ...) JSM_Report_Callback([&]() -> Jimara::Serialization::SerializedObject { \
	auto& JSM_ValueRef = JSM_ValueReference; \
	typedef std::remove_pointer_t<decltype(&JSM_ValueRef)> JSM_RawValue_T; \
	typedef std::conditional_t<std::is_enum_v<JSM_RawValue_T>, std::underlying_type<JSM_RawValue_T>, std::enable_if<true, JSM_RawValue_T>>::type JSM_Value_T; \
	static const auto JSM_Serializer = Jimara::Serialization::DefaultSerializer<JSM_Value_T>::Create(\
		JSM_ValueName, JSM_ValueHint, std::vector<Jimara::Reference<const Jimara::Object>> { __VA_ARGS__ }); \
	return JSM_Serializer->Serialize((JSM_Value_T*)(&JSM_ValueRef)); \
	}())


/// <summary>
/// Creates a static constant ValueSerializer with the getter and setter function pointers and exposes the field
/// <para/> Notes: 
/// <para/>		0. This will only work if used inside JSM_Body of JIMARA_SERIALIZE_FIELDS macro;
/// <para/>		1. Return value of JSM_Target->JSM_GetMethod() should be a primitive or vector type, compatible with ItemSerializer template 
///					(ei something like JSM_Target_Ref.m_val where m_val is a vector/float/integer/character member of the target object);
/// <para/>		2. JSM_Target->JSM_SetMethod(value) should receive the same type or const reference to the same type that JSM_GetMethod returns.
/// <para/>		3. Example usage would be: JIMARA_SERIALIZE_FIELD_GET_SET(GetValue, SetValue, "Value", "Value description", SerializerAttributes...).
/// </summary>
/// <param name="JSM_GetMethod"> Getter member function (for example, for 'ValueT JSM_Target_T::GetValue()' this would be just 'GetValue') </param>
/// <param name="JSM_SetMethod"> Setter member function (for example, for 'void JSM_Target_T::SetValue(ValueT (or const reference to value))' this would be 'SetValue') </param>
/// <param name="JSM_ValueName"> Name of the serialized field </param>
/// <param name="JSM_ValueHint"> Serialized field description </param>
/// <param name="__VA_ARGS__"> List of serializer attribute instances (references can be created inline, as well as statically) </param>
#define JIMARA_SERIALIZE_FIELD_GET_SET(JSM_GetMethod, JSM_SetMethod, JSM_ValueName, JSM_ValueHint, ...) JSM_Report_Callback([&]() -> Jimara::Serialization::SerializedObject { \
	auto JSM_GetValue = [&]() { auto rv = JSM_Target_Ref.JSM_GetMethod(); return rv; }; \
	typedef std::invoke_result_t<decltype(JSM_GetValue)> JSM_RawValue_T; \
	typedef std::conditional_t<std::is_enum_v<JSM_RawValue_T>, std::underlying_type<JSM_RawValue_T>, std::enable_if<true, JSM_RawValue_T>>::type JSM_Value_T; \
	typedef JSM_Value_T(*JSM_GetFn)(JSM_Target_T*); \
	typedef void(*JSM_SetFn)(JSM_Value_T const&, JSM_Target_T*); \
	static const Jimara::Reference<const Jimara::Serialization::ItemSerializer::Of<JSM_Target_T>> JSM_Serializer = \
		Jimara::Serialization::ValueSerializer<JSM_Value_T>::Create<JSM_Target_T>( \
			JSM_ValueName, JSM_ValueHint, \
			(JSM_GetFn)[](JSM_Target_T* JSM_Target) { return (JSM_Value_T)JSM_Target->JSM_GetMethod(); }, \
			(JSM_SetFn)[](JSM_Value_T const& JSM_Value, JSM_Target_T* JSM_Target) { JSM_Target->JSM_SetMethod((JSM_RawValue_T)JSM_Value); }, \
			std::vector<Jimara::Reference<const Jimara::Object>> { __VA_ARGS__ }); \
	return JSM_Serializer->Serialize(JSM_Target_Ref); \
	}())


/// <summary>
/// Creates a static immutable instance of a custom item serializer type and exposes given field reference with it
/// <para/> Notes: 
/// <para/>		0. This will only work if used inside JSM_Body of JIMARA_SERIALIZE_FIELDS macro;
/// <para/>		1. JSM_ValueReference should be a reference to any struct or a class, as long as JSM_SerializerType implements ItemSerializer::Of that particular class;
/// <para/>		2. Example usage would be: JIMARA_SERIALIZE_FIELD_CUSTOM(JSM_Target_Ref.m_struct, StructSerializer, static constructor arguments for StructSerializer instance).
/// </summary>
/// <param name="JSM_ValueReference"> Reference to the structure/class/object to be serialized </param>
/// <param name="JSM_SerializerType"> Custom serializer type </param>
/// <param name="__VA_ARGS__"> static constructor arguments for StructSerializer instance </param>
#define JIMARA_SERIALIZE_FIELD_CUSTOM(JSM_ValueReference, JSM_SerializerType, ...) JSM_Report_Callback([&]() -> Jimara::Serialization::SerializedObject { \
	auto& JSM_ValueRef = JSM_ValueReference; \
	typedef std::remove_pointer_t<decltype(&JSM_ValueRef)> JSM_Value_T; \
	static const Jimara::Reference<const Jimara::Serialization::ItemSerializer::Of<JSM_Value_T>> JSM_Serializer = \
		Jimara::Object::Instantiate<JSM_SerializerType>(__VA_ARGS__); \
	return JSM_Serializer->Serialize(JSM_ValueRef); \
	}())
