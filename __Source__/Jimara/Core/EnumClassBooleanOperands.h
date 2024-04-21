#pragma once
#include <type_traits>

/// <summary>
/// Defines basic bitwise operands for enumeration type
/// </summary>
/// <typeparam name="Type"> Any enum class </typeparam>
#define JIMARA_DEFINE_ENUMERATION_BOOLEAN_OPERATIONS(Type) \
	/* <summary> Logical negation of an enumeration value </summary> */ \
	/* <param name="v"> Enumeration value </param> */ \
	/* <returns> ~v </returns> */ \
	inline static constexpr Type operator~(Type v) { return static_cast<Type>(~static_cast<std::underlying_type_t<Type>>(v)); } \
	\
	/* <summary> 'Or' operation between enumeration values </summary> */ \
	/* <typeparam name="Type"> Enumeration type </typeparam> */ \
	/* <param name="a"> First bitmask </param> */ \
	/* <param name="b"> Second bitmask </param> */ \
	/* <returns> a | b </returns> */ \
	inline static constexpr Type operator|(Type a, Type b) { \
		return static_cast<Type>(static_cast<std::underlying_type_t<Type>>(a) | static_cast<std::underlying_type_t<Type>>(b)); \
	} \
	/* <summary> Logically 'or'-s one enumeration value with another </summary> */ \
	/* <typeparam name="Type"> Enumeration type </typeparam> */ \
	/* <param name="a"> First bitmask </param> */ \
	/* <param name="b"> Second bitmask </param> */ \
	/* <returns> a = (a | b) </returns> */ \
	inline static Type& operator|(Type& a, Type b) { return a = (a | b); } \
	\
	/* <summary> 'And' operation between enumeration values </summary> */ \
	/* <typeparam name="Type"> Enumeration type </typeparam> */ \
	/* <param name="a"> First bitmask </param> */ \
	/* <param name="b"> Second bitmask </param> */ \
	/* <returns> a & b </returns> */ \
	inline static constexpr Type operator&(Type a, Type b) { \
		return static_cast<Type>(static_cast<std::underlying_type_t<Type>>(a) & static_cast<std::underlying_type_t<Type>>(b)); \
	} \
	/* <summary> Logically 'and'-s one enumeration value with another </summary> */ \
	/* <typeparam name="Type"> Enumeration type </typeparam> */ \
	/* <param name="a"> First bitmask </param> */ \
	/* <param name="b"> Second bitmask </param> */ \
	/* <returns> a = (a & b) </returns> */ \
	inline static Type& operator&(Type& a, Type b) { return a = (a & b); } \
	\
	/* <summary> 'Xor' operation between enumeration values </summary> */ \
	/* <typeparam name="Type"> Enumeration type </typeparam> */ \
	/* <param name="a"> First bitmask </param> */ \
	/* <param name="b"> Second bitmask </param> */ \
	/* <returns> a ^ b </returns> */ \
	inline static constexpr Type operator^(Type a, Type b) { \
		return static_cast<Type>(static_cast<std::underlying_type_t<Type>>(a) ^ static_cast<std::underlying_type_t<Type>>(b)); \
	} \
	/* <summary> Logically 'xor'-s one enumeration value with another </summary> */ \
	/* <typeparam name="Type"> Enumeration type </typeparam> */ \
	/* <param name="a"> First bitmask </param> */ \
	/* <param name="b"> Second bitmask </param> */ \
	/* <returns> a = (a ^ b) </returns> */ \
	inline static Type& operator^(Type& a, Type b) { return a = (a ^ b); } \
	\
	/* A bunch of static assertions to make sure everything works */ \
	static_assert((static_cast<Type>(0) | static_cast<Type>(0)) == static_cast<Type>(0 | 0)); \
	static_assert((static_cast<Type>(0) | static_cast<Type>(1)) == static_cast<Type>(0 | 1)); \
	static_assert((static_cast<Type>(0) | static_cast<Type>(2)) == static_cast<Type>(0 | 2)); \
	static_assert((static_cast<Type>(0) | static_cast<Type>(3)) == static_cast<Type>(0 | 3)); \
	static_assert((static_cast<Type>(1) | static_cast<Type>(0)) == static_cast<Type>(1 | 0)); \
	static_assert((static_cast<Type>(1) | static_cast<Type>(1)) == static_cast<Type>(1 | 1)); \
	static_assert((static_cast<Type>(1) | static_cast<Type>(2)) == static_cast<Type>(1 | 2)); \
	static_assert((static_cast<Type>(1) | static_cast<Type>(3)) == static_cast<Type>(1 | 3)); \
	static_assert((static_cast<Type>(2) | static_cast<Type>(0)) == static_cast<Type>(2 | 0)); \
	static_assert((static_cast<Type>(2) | static_cast<Type>(1)) == static_cast<Type>(2 | 1)); \
	static_assert((static_cast<Type>(2) | static_cast<Type>(2)) == static_cast<Type>(2 | 2)); \
	static_assert((static_cast<Type>(2) | static_cast<Type>(3)) == static_cast<Type>(2 | 3)); \
	static_assert((static_cast<Type>(3) | static_cast<Type>(0)) == static_cast<Type>(3 | 0)); \
	static_assert((static_cast<Type>(3) | static_cast<Type>(1)) == static_cast<Type>(3 | 1)); \
	static_assert((static_cast<Type>(3) | static_cast<Type>(2)) == static_cast<Type>(3 | 2)); \
	static_assert((static_cast<Type>(3) | static_cast<Type>(3)) == static_cast<Type>(3 | 3)); \
	\
	static_assert((static_cast<Type>(0) & static_cast<Type>(0)) == static_cast<Type>(0 & 0)); \
	static_assert((static_cast<Type>(0) & static_cast<Type>(1)) == static_cast<Type>(0 & 1)); \
	static_assert((static_cast<Type>(0) & static_cast<Type>(2)) == static_cast<Type>(0 & 2)); \
	static_assert((static_cast<Type>(0) & static_cast<Type>(3)) == static_cast<Type>(0 & 3)); \
	static_assert((static_cast<Type>(1) & static_cast<Type>(0)) == static_cast<Type>(1 & 0)); \
	static_assert((static_cast<Type>(1) & static_cast<Type>(1)) == static_cast<Type>(1 & 1)); \
	static_assert((static_cast<Type>(1) & static_cast<Type>(2)) == static_cast<Type>(1 & 2)); \
	static_assert((static_cast<Type>(1) & static_cast<Type>(3)) == static_cast<Type>(1 & 3)); \
	static_assert((static_cast<Type>(2) & static_cast<Type>(0)) == static_cast<Type>(2 & 0)); \
	static_assert((static_cast<Type>(2) & static_cast<Type>(1)) == static_cast<Type>(2 & 1)); \
	static_assert((static_cast<Type>(2) & static_cast<Type>(2)) == static_cast<Type>(2 & 2)); \
	static_assert((static_cast<Type>(2) & static_cast<Type>(3)) == static_cast<Type>(2 & 3)); \
	static_assert((static_cast<Type>(3) & static_cast<Type>(0)) == static_cast<Type>(3 & 0)); \
	static_assert((static_cast<Type>(3) & static_cast<Type>(1)) == static_cast<Type>(3 & 1)); \
	static_assert((static_cast<Type>(3) & static_cast<Type>(2)) == static_cast<Type>(3 & 2)); \
	static_assert((static_cast<Type>(3) & static_cast<Type>(3)) == static_cast<Type>(3 & 3)); \
	\
	static_assert((static_cast<Type>(0) ^ static_cast<Type>(0)) == static_cast<Type>(0 ^ 0)); \
	static_assert((static_cast<Type>(0) ^ static_cast<Type>(1)) == static_cast<Type>(0 ^ 1)); \
	static_assert((static_cast<Type>(0) ^ static_cast<Type>(2)) == static_cast<Type>(0 ^ 2)); \
	static_assert((static_cast<Type>(0) ^ static_cast<Type>(3)) == static_cast<Type>(0 ^ 3)); \
	static_assert((static_cast<Type>(1) ^ static_cast<Type>(0)) == static_cast<Type>(1 ^ 0)); \
	static_assert((static_cast<Type>(1) ^ static_cast<Type>(1)) == static_cast<Type>(1 ^ 1)); \
	static_assert((static_cast<Type>(1) ^ static_cast<Type>(2)) == static_cast<Type>(1 ^ 2)); \
	static_assert((static_cast<Type>(1) ^ static_cast<Type>(3)) == static_cast<Type>(1 ^ 3)); \
	static_assert((static_cast<Type>(2) ^ static_cast<Type>(0)) == static_cast<Type>(2 ^ 0)); \
	static_assert((static_cast<Type>(2) ^ static_cast<Type>(1)) == static_cast<Type>(2 ^ 1)); \
	static_assert((static_cast<Type>(2) ^ static_cast<Type>(2)) == static_cast<Type>(2 ^ 2)); \
	static_assert((static_cast<Type>(2) ^ static_cast<Type>(3)) == static_cast<Type>(2 ^ 3)); \
	static_assert((static_cast<Type>(3) ^ static_cast<Type>(0)) == static_cast<Type>(3 ^ 0)); \
	static_assert((static_cast<Type>(3) ^ static_cast<Type>(1)) == static_cast<Type>(3 ^ 1)); \
	static_assert((static_cast<Type>(3) ^ static_cast<Type>(2)) == static_cast<Type>(3 ^ 2)); \
	static_assert((static_cast<Type>(3) ^ static_cast<Type>(3)) == static_cast<Type>(3 ^ 3))
