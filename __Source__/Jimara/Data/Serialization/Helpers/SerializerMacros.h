#pragma once
#include "../ItemSerializers.h"
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
/// <para/>		1. JSM_ValueReference should be a direct reference to a primitive or vector type, compatible with ItemSerializer template 
///					(ei something like JSM_Target_Ref.m_val where m_val is a vector/float/integer/character member of the target object);
/// <para/>		2. Example usage would be: JIMARA_SERIALIZE_FIELD(JSM_Target_Ref.m_val, "Value", "Value description", SerializerAttributes...).
/// </summary>
/// <param name="JSM_ValueReference"> Reference to the value to be serialized </param>
/// <param name="JSM_ValueName"> Name of the serialized field </param>
/// <param name="JSM_ValueHint"> Serialized field description </param>
/// <param name="__VA_ARGS__"> List of serializer attribute instances (references can be created inline, as well as statically) </param>
#define JIMARA_SERIALIZE_FIELD(JSM_ValueReference, JSM_ValueName, JSM_ValueHint, ...) JSM_Report_Callback([&]() -> Jimara::Serialization::SerializedObject { \
	auto& JSM_ValueRef = JSM_ValueReference; \
	typedef std::remove_pointer_t<decltype(&JSM_ValueRef)> JSM_Value_T; \
	static const Jimara::Reference<const Jimara::Serialization::ItemSerializer::Of<JSM_Value_T>> JSM_Serializer = \
		Jimara::Serialization::ValueSerializer<JSM_Value_T>::Create(JSM_ValueName, JSM_ValueHint, std::vector<Jimara::Reference<const Jimara::Object>> { __VA_ARGS__ }); \
	return JSM_Serializer->Serialize(JSM_ValueRef); \
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
	typedef std::invoke_result_t<decltype(JSM_GetValue)> JSM_Value_T; \
	typedef JSM_Value_T(*JSM_GetFn)(JSM_Target_T*); \
	typedef void(*JSM_SetFn)(JSM_Value_T const&, JSM_Target_T*); \
	static const Jimara::Reference<const Jimara::Serialization::ItemSerializer::Of<JSM_Target_T>> JSM_Serializer = \
		Jimara::Serialization::ValueSerializer<JSM_Value_T>::Create<JSM_Target_T>( \
			JSM_ValueName, JSM_ValueHint, \
			(JSM_GetFn)[](JSM_Target_T* JSM_Target) { return JSM_Target->JSM_GetMethod(); }, \
			(JSM_SetFn)[](JSM_Value_T const& JSM_Value, JSM_Target_T* JSM_Target) { JSM_Target->JSM_SetMethod(JSM_Value); }, \
			std::vector<Jimara::Reference<const Jimara::Object>> { __VA_ARGS__ }); \
	return JSM_Serializer->Serialize(JSM_Target_Ref); \
	}())


/// <summary>
/// Creates a static constant string serializer and serializes given string reference with it
/// <para/> Notes: 
/// <para/>		0. This will only work if used inside JSM_Body of JIMARA_SERIALIZE_FIELDS macro;
/// <para/>		1. JSM_StringRef has to be a reference to a valid string
/// <para/>		2. Example usage would be: JIMARA_SERIALIZE_STRING(JSM_Target_Ref.m_someString, "Text", "Text description", SerializerAttributes...).
/// </summary>
/// <param name="JSM_StringRef"> Reference to the string value to be serialized </param>
/// <param name="JSM_ValueName"> Name of the serialized field </param>
/// <param name="JSM_ValueHint"> Serialized field description </param>
/// <param name="__VA_ARGS__"> List of serializer attribute instances (references can be created inline, as well as statically) </param>
#define JIMARA_SERIALIZE_STRING(JSM_StringRef, JSM_ValueName, JSM_ValueHint, ...) JSM_Report_Callback([&]() -> Jimara::Serialization::SerializedObject { \
	typedef std::string_view(*JSM_GetFn)(std::string*); \
	typedef void(*JSM_SetFn)(std::string_view const&, std::string*); \
	static const Jimara::Reference<const Jimara::Serialization::ItemSerializer::Of<std::string>> JSM_Serializer = \
		Jimara::Serialization::ValueSerializer<std::string_view>::Create<std::string>( \
			JSM_ValueName, JSM_ValueHint, \
			(JSM_GetFn)[](std::string* JSM_StringPtr) -> std::string_view { return *JSM_StringPtr; }, \
			(JSM_SetFn)[](const std::string_view& JSM_Value, std::string* JSM_StringPtr) { (*JSM_StringPtr) = JSM_Value; }, \
			std::vector<Jimara::Reference<const Jimara::Object>> { __VA_ARGS__ }); \
	return JSM_Serializer->Serialize(JSM_StringRef); \
	}())


/// <summary>
/// Creates a static constant wide string serializer and serializes given wide string reference with it
/// <para/> Notes: 
/// <para/>		0. This will only work if used inside JSM_Body of JIMARA_SERIALIZE_FIELDS macro;
/// <para/>		1. JSM_WstringRef has to be a reference to a valid wstring
/// <para/>		2. Example usage would be: JIMARA_SERIALIZE_WSTRING(JSM_Target_Ref.m_someWstring, "Text", "Text description", SerializerAttributes...).
/// </summary>
/// <param name="JSM_WstringRef"> Reference to the std::wstring value to be serialized </param>
/// <param name="JSM_ValueName"> Name of the serialized field </param>
/// <param name="JSM_ValueHint"> Serialized field description </param>
/// <param name="__VA_ARGS__"> List of serializer attribute instances (references can be created inline, as well as statically) </param>
#define JIMARA_SERIALIZE_WSTRING(JSM_WstringRef, JSM_ValueName, JSM_ValueHint, ...) JSM_Report_Callback([&]() -> Jimara::Serialization::SerializedObject { \
	typedef std::wstring_view(*JSM_GetFn)(std::wstring*); \
	typedef void(*JSM_SetFn)(std::wstring_view const&, std::wstring*); \
	static const Jimara::Reference<const Jimara::Serialization::ItemSerializer::Of<std::wstring>> JSM_Serializer = \
		Jimara::Serialization::ValueSerializer<std::wstring_view>::Create<std::wstring>( \
			JSM_ValueName, JSM_ValueHint, \
			(JSM_GetFn)[](std::wstring* JSM_StringPtr) -> std::wstring_view { return *JSM_StringPtr; }, \
			(JSM_SetFn)[](const std::wstring_view& JSM_Value, std::wstring* JSM_StringPtr) { (*JSM_StringPtr) = JSM_Value; }, \
			std::vector<Jimara::Reference<const Jimara::Object>> { __VA_ARGS__ }); \
	return JSM_Serializer->Serialize(JSM_WstringRef); \
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
