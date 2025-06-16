#pragma once
#include "../ItemSerializers.h"

namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Bitmask of ItemSerializer::Type
		/// </summary>
		class SerializerTypeMask {
		private:
			// Underlying bitmask
			uint32_t m_mask;

			// Internal constructor for boolean operations between SerializerTypeMask instances
			inline constexpr SerializerTypeMask(uint32_t mask) : m_mask(mask) { }

		public:
			/// <summary> Constructor </summary>
			inline constexpr SerializerTypeMask() : m_mask(0) {};

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="type"> Type to include in mask </param>
			inline constexpr SerializerTypeMask(ItemSerializer::Type type) : SerializerTypeMask() { (*this) |= type; }

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="firstType"> First type to include in mask </param>
			/// <param name="secondType"> Second type to include in mask </param>
			/// <param name="rest..."> Rest of the types to include in mask </param>
			template<typename... Types>
			inline constexpr SerializerTypeMask(ItemSerializer::Type firstType, ItemSerializer::Type secondType, Types... rest) 
				: SerializerTypeMask(secondType, rest...) {
				(*this) |= firstType;
			}

			/// <summary> Any character type (char/signed char/unsigned char/wchar_t) </summary>
			inline static constexpr SerializerTypeMask CharacterTypes() {
				return SerializerTypeMask(
					ItemSerializer::Type::CHAR_VALUE, ItemSerializer::Type::SCHAR_VALUE, ItemSerializer::Type::UCHAR_VALUE, ItemSerializer::Type::WCHAR_VALUE);
			}

			/// <summary> Any signed integer type (short/int/long/long long) </summary>
			inline static constexpr SerializerTypeMask SignedIntegerTypes() {
				return SerializerTypeMask(
					ItemSerializer::Type::SHORT_VALUE, ItemSerializer::Type::INT_VALUE, ItemSerializer::Type::LONG_VALUE, ItemSerializer::Type::LONG_LONG_VALUE);
			}

			/// <summary> Any unsigned integer type (unsigned short/unsigned int/unsigned long/unsigned long long) </summary>
			inline static constexpr SerializerTypeMask UnsignedIntegerTypes() {
				return SerializerTypeMask(
					ItemSerializer::Type::USHORT_VALUE, ItemSerializer::Type::UINT_VALUE, ItemSerializer::Type::ULONG_VALUE, ItemSerializer::Type::ULONG_LONG_VALUE);
			}

			/// <summary> Any integer type (SignedIntegerTypes() | UnsignedIntegerTypes()) </summary>
			inline static constexpr SerializerTypeMask IntegerTypes() {
				return SignedIntegerTypes() | UnsignedIntegerTypes();
			}

			/// <summary> Floating point types (float/double) </summary>
			inline static constexpr SerializerTypeMask FloatingPointTypes() {
				return SerializerTypeMask(ItemSerializer::Type::FLOAT_VALUE, ItemSerializer::Type::DOUBLE_VALUE);
			}

			/// <summary> Any vector type (Vector2/Vector3/Vector4) </summary>
			inline static constexpr SerializerTypeMask VectorTypes() {
				return SerializerTypeMask(ItemSerializer::Type::VECTOR2_VALUE, ItemSerializer::Type::VECTOR3_VALUE, ItemSerializer::Type::VECTOR4_VALUE);
			}

			/// <summary> Any matrix type (Matrix2/Matrix3/Matrix4) </summary>
			inline static constexpr SerializerTypeMask MatrixTypes() {
				return SerializerTypeMask(ItemSerializer::Type::MATRIX2_VALUE, ItemSerializer::Type::MATRIX3_VALUE, ItemSerializer::Type::MATRIX4_VALUE);
			}

			/// <summary> Any string view type (std::string_view/std::wstring_view) </summary>
			inline static constexpr SerializerTypeMask StringViewTypes() {
				return SerializerTypeMask(ItemSerializer::Type::STRING_VIEW_VALUE, ItemSerializer::Type::WSTRING_VIEW_VALUE);
			}

			/// <summary> All value types </summary>
			inline static constexpr SerializerTypeMask AllValueTypes() {
				return SerializerTypeMask(ItemSerializer::Type::BOOL_VALUE)
					| CharacterTypes() | IntegerTypes() | FloatingPointTypes() | VectorTypes() | MatrixTypes() | StringViewTypes();
			}

			/// <summary> All valid types </summary>
			inline static constexpr SerializerTypeMask AllTypes() {
				return AllValueTypes() | SerializerTypeMask(ItemSerializer::Type::OBJECT_REFERENCE_VALUE, ItemSerializer::Type::SERIALIZER_LIST);
			}

			/// <summary> Bit, corresponding to given type </summary>
			inline static constexpr uint32_t Bit(ItemSerializer::Type type) { return uint32_t(1) << static_cast<uint32_t>(type); }

			/// <summary>
			/// Includes given type
			/// </summary>
			/// <param name="type"> Type to include </param>
			/// <returns> self </returns>
			inline constexpr SerializerTypeMask& operator|=(ItemSerializer::Type type) {
				m_mask |= Bit(type);
				return *this;
			}

			/// <summary>
			/// SerializerTypeMask, containing all types from this, plus given type
			/// </summary>
			/// <param name="type"> Type to include </param>
			/// <returns> self | type </returns>
			inline constexpr SerializerTypeMask operator|(ItemSerializer::Type type)const {
				return (*this) | SerializerTypeMask(type);
			}

			/// <summary>
			/// Excludes given type
			/// </summary>
			/// <param name="type"> Type to exclude </param>
			/// <returns> self </returns>
			inline constexpr SerializerTypeMask& operator^=(ItemSerializer::Type type) {
				m_mask &= (~(uint32_t(0))) ^ Bit(type);
				return *this;
			}

			/// <summary>
			/// SerializerTypeMask, containing all types from this, minus given type
			/// </summary>
			/// <param name="type"> Type to exclude </param>
			/// <returns> self ^ type </returns>
			inline constexpr SerializerTypeMask operator^(ItemSerializer::Type type)const {
				return (*this) ^ SerializerTypeMask(type);
			}

			/// <summary>
			/// Checks if type is contained in the bitmask
			/// </summary>
			/// <param name="type"> Type to check </param>
			/// <returns> self & type </returns>
			inline constexpr bool operator&(ItemSerializer::Type type)const {
				return (m_mask & Bit(type)) != 0;
			}

			/// <summary>
			/// SerializerTypeMask containing all entries from this and the other
			/// </summary>
			/// <param name="other"> Type mask to logically 'or' with </param>
			/// <returns> self | other </returns>
			inline constexpr SerializerTypeMask operator|(const SerializerTypeMask& other)const {
				return SerializerTypeMask(m_mask | other.m_mask);
			}

			/// <summary>
			/// Adds entries from other
			/// </summary>
			/// <param name="other"> Type mask to logically 'or' with </param>
			/// <returns> self </returns>
			inline constexpr SerializerTypeMask& operator|=(const SerializerTypeMask& other) {
				m_mask |= other.m_mask;
				return (*this);
			}

			/// <summary>
			/// SerializerTypeMask containing common entries from this and the other
			/// </summary>
			/// <param name="other"> Type mask to logically 'and' with </param>
			/// <returns> self & other </returns>
			inline constexpr SerializerTypeMask operator&(const SerializerTypeMask& other)const {
				return SerializerTypeMask(m_mask & other.m_mask);
			}

			/// <summary>
			/// Leaves only the entries that are also contained inside other
			/// </summary>
			/// <param name="other"> Type mask to logically 'and' with </param>
			/// <returns> self </returns>
			inline constexpr SerializerTypeMask& operator&=(const SerializerTypeMask& other) {
				m_mask &= other.m_mask;
				return (*this);
			}

			/// <summary>
			/// SerializerTypeMask containing entries that are in only one of this and other
			/// </summary>
			/// <param name="other"> Type mask to logically 'xor' with </param>
			/// <returns> self ^ other </returns>
			inline constexpr SerializerTypeMask operator^(const SerializerTypeMask& other)const {
				return SerializerTypeMask(m_mask ^ other.m_mask);
			}

			/// <summary>
			/// Sets to entries that are in only one of this and other
			/// </summary>
			/// <param name="other"> Type mask to logically 'xor' with </param>
			/// <returns> self </returns>
			inline constexpr SerializerTypeMask& operator^=(const SerializerTypeMask& other) {
				m_mask ^= other.m_mask;
				return (*this);
			}
		};

		// Static asserts to make sure the whole thing functions as intended
		static_assert(SerializerTypeMask(ItemSerializer::Type::BOOL_VALUE)& ItemSerializer::Type::BOOL_VALUE);
		static_assert(!(SerializerTypeMask()& ItemSerializer::Type::BOOL_VALUE));

		static_assert(SerializerTypeMask(ItemSerializer::Type::LONG_VALUE)& ItemSerializer::Type::LONG_VALUE);
		static_assert(!(SerializerTypeMask(ItemSerializer::Type::BOOL_VALUE)& ItemSerializer::Type::LONG_VALUE));

		static_assert(SerializerTypeMask(ItemSerializer::Type::FLOAT_VALUE, ItemSerializer::Type::DOUBLE_VALUE)& ItemSerializer::Type::FLOAT_VALUE);
		static_assert(SerializerTypeMask(ItemSerializer::Type::FLOAT_VALUE, ItemSerializer::Type::DOUBLE_VALUE)& ItemSerializer::Type::DOUBLE_VALUE);
		static_assert(!(SerializerTypeMask(ItemSerializer::Type::FLOAT_VALUE, ItemSerializer::Type::DOUBLE_VALUE)& ItemSerializer::Type::LONG_VALUE));

		static_assert(SerializerTypeMask::CharacterTypes()& ItemSerializer::Type::CHAR_VALUE);
		static_assert(SerializerTypeMask::CharacterTypes()& ItemSerializer::Type::SCHAR_VALUE);
		static_assert(SerializerTypeMask::CharacterTypes()& ItemSerializer::Type::UCHAR_VALUE);
		static_assert(SerializerTypeMask::CharacterTypes()& ItemSerializer::Type::WCHAR_VALUE);

		static_assert(SerializerTypeMask::SignedIntegerTypes()& ItemSerializer::Type::SHORT_VALUE);
		static_assert(SerializerTypeMask::SignedIntegerTypes()& ItemSerializer::Type::INT_VALUE);
		static_assert(SerializerTypeMask::SignedIntegerTypes()& ItemSerializer::Type::LONG_VALUE);
		static_assert(SerializerTypeMask::SignedIntegerTypes()& ItemSerializer::Type::LONG_LONG_VALUE);

		static_assert(SerializerTypeMask::UnsignedIntegerTypes()& ItemSerializer::Type::USHORT_VALUE);
		static_assert(SerializerTypeMask::UnsignedIntegerTypes()& ItemSerializer::Type::UINT_VALUE);
		static_assert(SerializerTypeMask::UnsignedIntegerTypes()& ItemSerializer::Type::ULONG_VALUE);
		static_assert(SerializerTypeMask::UnsignedIntegerTypes()& ItemSerializer::Type::ULONG_LONG_VALUE);

		static_assert(SerializerTypeMask::FloatingPointTypes()& ItemSerializer::Type::FLOAT_VALUE);
		static_assert(SerializerTypeMask::FloatingPointTypes()& ItemSerializer::Type::DOUBLE_VALUE);

		static_assert(SerializerTypeMask::VectorTypes()& ItemSerializer::Type::VECTOR2_VALUE);
		static_assert(SerializerTypeMask::VectorTypes()& ItemSerializer::Type::VECTOR3_VALUE);
		static_assert(SerializerTypeMask::VectorTypes()& ItemSerializer::Type::VECTOR4_VALUE);

		static_assert(SerializerTypeMask::MatrixTypes()& ItemSerializer::Type::MATRIX2_VALUE);
		static_assert(SerializerTypeMask::MatrixTypes()& ItemSerializer::Type::MATRIX3_VALUE);
		static_assert(SerializerTypeMask::MatrixTypes()& ItemSerializer::Type::MATRIX4_VALUE);

		static_assert(SerializerTypeMask::StringViewTypes()& ItemSerializer::Type::STRING_VIEW_VALUE);
		static_assert(SerializerTypeMask::StringViewTypes()& ItemSerializer::Type::WSTRING_VIEW_VALUE);

		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::BOOL_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::CHAR_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::SCHAR_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::UCHAR_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::WCHAR_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::SHORT_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::USHORT_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::INT_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::UINT_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::LONG_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::ULONG_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::LONG_LONG_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::ULONG_LONG_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::FLOAT_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::DOUBLE_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::VECTOR2_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::VECTOR3_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::VECTOR4_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::MATRIX2_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::MATRIX3_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::MATRIX4_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::STRING_VIEW_VALUE);
		static_assert(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::WSTRING_VIEW_VALUE);
		static_assert(!(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::OBJECT_REFERENCE_VALUE));
		static_assert(!(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::SERIALIZER_LIST));
		static_assert(!(SerializerTypeMask::AllValueTypes()& ItemSerializer::Type::SERIALIZER_TYPE_COUNT));

		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::BOOL_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::CHAR_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::SCHAR_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::UCHAR_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::WCHAR_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::SHORT_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::USHORT_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::INT_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::UINT_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::LONG_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::ULONG_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::LONG_LONG_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::ULONG_LONG_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::FLOAT_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::DOUBLE_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::VECTOR2_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::VECTOR3_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::VECTOR4_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::MATRIX2_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::MATRIX3_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::MATRIX4_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::STRING_VIEW_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::WSTRING_VIEW_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::OBJECT_REFERENCE_VALUE);
		static_assert(SerializerTypeMask::AllTypes()& ItemSerializer::Type::SERIALIZER_LIST);
		static_assert(!(SerializerTypeMask::AllTypes()& ItemSerializer::Type::SERIALIZER_TYPE_COUNT));
	}
}

