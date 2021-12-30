#include "ComponentFieldTypes.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../Data/Serialization/Attributes/EulerAnglesAttribute.h"
#include "../../Data/Serialization/Attributes/SliderAttribute.h"

namespace Jimara {
	namespace Samples {
		namespace {
			// For each attribute type, we add a handy struct with a static method that creates attributes of given type

			/// <summary>
			/// Creates no attributes
			/// </summary>
			struct NoAttributeFactory {
				template<typename ValueType>
				inline static std::vector<Reference<const Object>> CreateAttributes() {
					return std::vector<Reference<const Object>>();
				}
			};

			/// <summary>
			/// Creates Serialization::ColorAttribute for each field
			/// </summary>
			struct ColorAttributeFactory {
				template<typename ValueType>
				inline static std::vector<Reference<const Object>> CreateAttributes() {
					return std::vector<Reference<const Object>>({ Object::Instantiate<Serialization::ColorAttribute>() });
				}
			};

			/// <summary>
			/// Creates Serialization::EnumAttribute for each field (not bitmask)
			/// </summary>
			struct EnumAttributeFactory {
				template<typename ValueType>
				inline static std::enable_if_t<
					!(std::is_same_v<ValueType, std::string_view> || std::is_same_v<ValueType, std::wstring_view>)
					, std::vector<Reference<const Object>>> CreateAttributes() {
					return std::vector<Reference<const Object>>({ Object::Instantiate<Serialization::EnumAttribute<ValueType>>(
						std::vector<typename Serialization::EnumAttribute<ValueType>::Choice> {
							typename Serialization::EnumAttribute<ValueType>::Choice("ZERO", ValueType(0)),
							typename Serialization::EnumAttribute<ValueType>::Choice("ONE", ValueType(1)),
							typename Serialization::EnumAttribute<ValueType>::Choice("TWO", ValueType(2)),
							typename Serialization::EnumAttribute<ValueType>::Choice("THREE", ValueType(3))
						}, false)});
				}

				template<typename ValueType>
				inline static 
				std::enable_if_t<std::is_same_v<ValueType, std::string_view>, std::vector<Reference<const Object>>> CreateAttributes() {
					return std::vector<Reference<const Object>>({ Object::Instantiate<Serialization::EnumAttribute<std::string_view>>(
						std::vector<Serialization::EnumAttribute<std::string_view>::Choice> {
							Serialization::EnumAttribute<std::string_view>::Choice("ZERO", "0S"),
							Serialization::EnumAttribute<std::string_view>::Choice("ONE", "1S"),
							Serialization::EnumAttribute<std::string_view>::Choice("TWO", "2S"),
							Serialization::EnumAttribute<std::string_view>::Choice("THREE", "3S")
						}, false) });
				}

				template<typename ValueType>
				inline static 
				std::enable_if_t<std::is_same_v<ValueType, std::wstring_view>, std::vector<Reference<const Object>>> CreateAttributes() {
					return std::vector<Reference<const Object>>({ Object::Instantiate<Serialization::EnumAttribute<std::wstring_view>>(
						std::vector<Serialization::EnumAttribute<std::wstring_view>::Choice> {
							Serialization::EnumAttribute<std::wstring_view>::Choice("ZERO", L"0WS"),
							Serialization::EnumAttribute<std::wstring_view>::Choice("ONE", L"1WS"),
							Serialization::EnumAttribute<std::wstring_view>::Choice("TWO", L"2WS"),
							Serialization::EnumAttribute<std::wstring_view>::Choice("THREE", L"3WS")
						}, false) });
				}
			};

			/// <summary>
			/// Creates Serialization::EnumAttribute for each field (bitmask)
			/// </summary>
			struct BitmaskAttributeFactory {
				template<typename ValueType>
				inline static std::enable_if_t<
					!(std::is_same_v<ValueType, std::string_view> || std::is_same_v<ValueType, std::wstring_view>)
					, std::vector<Reference<const Object>>> CreateAttributes() {
					return std::vector<Reference<const Object>>({ Object::Instantiate<Serialization::EnumAttribute<ValueType>>(
						std::vector<typename Serialization::EnumAttribute<ValueType>::Choice> {
							typename Serialization::EnumAttribute<ValueType>::Choice("ZERO", ValueType(0)),
							typename Serialization::EnumAttribute<ValueType>::Choice("ONE", ValueType(1)),
							typename Serialization::EnumAttribute<ValueType>::Choice("TWO", ValueType(2)),
							typename Serialization::EnumAttribute<ValueType>::Choice("FOUR", ValueType(4)),
							typename Serialization::EnumAttribute<ValueType>::Choice("ALL", ValueType(7))
						}, true) });
				}

				template<typename ValueType>
				inline static 
				std::enable_if_t<std::is_same_v<ValueType, std::string_view>, std::vector<Reference<const Object>>> CreateAttributes() {
					return std::vector<Reference<const Object>>({ Object::Instantiate<Serialization::EnumAttribute<std::string_view>>(
						std::vector<Serialization::EnumAttribute<std::string_view>::Choice> {
							Serialization::EnumAttribute<std::string_view>::Choice("ZERO", "0S"),
							Serialization::EnumAttribute<std::string_view>::Choice("ONE", "1S"),
							Serialization::EnumAttribute<std::string_view>::Choice("TWO", "2S"),
							Serialization::EnumAttribute<std::string_view>::Choice("FOUR", "4S"),
							Serialization::EnumAttribute<std::string_view>::Choice("ALL", "7S")
						}, true) });
				}

				template<typename ValueType>
				inline static 
				std::enable_if_t<std::is_same_v<ValueType, std::wstring_view>, std::vector<Reference<const Object>>> CreateAttributes() {
					return std::vector<Reference<const Object>>({ Object::Instantiate<Serialization::EnumAttribute<std::wstring_view>>(
						std::vector<Serialization::EnumAttribute<std::wstring_view>::Choice> {
							Serialization::EnumAttribute<std::wstring_view>::Choice("ZERO", L"0WS"),
							Serialization::EnumAttribute<std::wstring_view>::Choice("ONE", L"1WS"),
							Serialization::EnumAttribute<std::wstring_view>::Choice("TWO", L"2WS"),
							Serialization::EnumAttribute<std::wstring_view>::Choice("FOUR", L"4WS"),
							Serialization::EnumAttribute<std::wstring_view>::Choice("ALL", L"7WS")
						}, true) });
				}
			};

			/// <summary>
			/// Creates Serialization::EulerAnglesAttribute for each field
			/// </summary>
			struct EulerAnglesAttributeFactory {
				template<typename ValueType>
				inline static std::vector<Reference<const Object>> CreateAttributes() {
					return std::vector<Reference<const Object>>({ Object::Instantiate<Serialization::EulerAnglesAttribute>() });
				}
			};

			/// <summary>
			/// Creates Serialization::SliderAttribute for each field
			/// </summary>
			struct SliderAttributeFactory {
				template<typename ValueType>
				inline static std::enable_if_t<
					!(
						std::is_same_v<ValueType, float> 
						|| std::is_same_v<ValueType, std::string_view> 
						|| std::is_same_v<ValueType, std::wstring_view>
					), std::vector<Reference<const Object>>> CreateAttributes() {
					return std::vector<Reference<const Object>>({
						Object::Instantiate<Serialization::SliderAttribute<ValueType>>(ValueType(0), ValueType(15), ValueType(2)) });
				}

				template<typename ValueType>
				inline static 
				std::enable_if_t<std::is_same_v<ValueType, float>, std::vector<Reference<const Object>>> CreateAttributes() {
					return std::vector<Reference<const Object>>({
						Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f) });
				}

				template<typename ValueType>
				inline static 
				std::enable_if_t<std::is_same_v<ValueType, std::string_view>, std::vector<Reference<const Object>>> CreateAttributes() {
					return std::vector<Reference<const Object>>({
						Object::Instantiate<Serialization::SliderAttribute<std::string_view>>("A", "B", "C") });
				}

				template<typename ValueType>
				inline static 
				std::enable_if_t<std::is_same_v<ValueType, std::wstring_view>, std::vector<Reference<const Object>>> CreateAttributes() {
					return std::vector<Reference<const Object>>({
						Object::Instantiate<Serialization::SliderAttribute<std::wstring_view>>(L"A", L"B", L"C") });
				}
			};
		}

		/// <summary>
		/// Main ComponentSerializer for ComponentFieldTypes
		/// </summary>
		class ComponentFieldTypes::Serializer : public virtual ComponentSerializer::Of<ComponentFieldTypes> {
		private:
			// Sub-serializer for AllTypes::CharacterTypes
			class CharacterTypesSerializer : public virtual Serialization::SerializerList::From<AllTypes::CharacterTypes> {
			private:
				const Reference<const ItemSerializer::Of<char>> m_charValueSerializer;
				const Reference<const ItemSerializer::Of<signed char>> m_signedCharValueSerializer;
				const Reference<const ItemSerializer::Of<unsigned char>> m_unsignedCharValueSerializer;
				const Reference<const ItemSerializer::Of<wchar_t>> m_wideCharValueSerializer;

			public:
				// Constructor
				inline CharacterTypesSerializer(
					Reference<const ItemSerializer::Of<char>> charValueSerializer,
					Reference<const ItemSerializer::Of<signed char>> signedCharValueSerializer,
					Reference<const ItemSerializer::Of<unsigned char>> unsignedCharValueSerializer,
					Reference<const ItemSerializer::Of<wchar_t>> wideCharValueSerializer) 
					: ItemSerializer("Character types", "<char>/<signed char>/<unsigned char>/<wchar_t> types")
					, m_charValueSerializer(charValueSerializer)
					, m_signedCharValueSerializer(signedCharValueSerializer)
					, m_unsignedCharValueSerializer(unsignedCharValueSerializer)
					, m_wideCharValueSerializer(wideCharValueSerializer) {}

				// Creates Sub-Serializer with given attribute factory
				template<typename AttributeFactory>
				inline static Reference<const CharacterTypesSerializer> Create() {
					return Object::Instantiate<CharacterTypesSerializer>(
						Serialization::ValueSerializer<char>::Create("char", "<char> value", 
							AttributeFactory::template CreateAttributes<char>()),
						Serialization::ValueSerializer<signed char>::Create("signed char", "<signed char> value", 
							AttributeFactory::template CreateAttributes<signed char>()),
						Serialization::ValueSerializer<unsigned char>::Create("unsigned char", "<unsigned char> value", 
							AttributeFactory::template CreateAttributes<unsigned char>()),
						Serialization::ValueSerializer<wchar_t>::Create("wchar_t", "<wchar_t> value", 
							AttributeFactory::template CreateAttributes<wchar_t>()));
				}

				// Exposes fields
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AllTypes::CharacterTypes* target)const final override {
					recordElement(m_charValueSerializer->Serialize(target->charValue));
					recordElement(m_signedCharValueSerializer->Serialize(target->signedCharValue));
					recordElement(m_unsignedCharValueSerializer->Serialize(target->unsignedCharValue));
					recordElement(m_wideCharValueSerializer->Serialize(target->wideCharValue));
				}
			};

			// Sub-serializer for AllTypes::IntegerTypes::Signed
			class SignedIntegerTypesSerializer : public virtual Serialization::SerializerList::From<AllTypes::IntegerTypes::Signed> {
			private:
				const Reference<const ItemSerializer::Of<short>> m_shortValueSerializer;
				const Reference<const ItemSerializer::Of<int>> m_intValueSerializer;
				const Reference<const ItemSerializer::Of<long>> m_longValueSerializer;
				const Reference<const ItemSerializer::Of<long long>> m_longLongValueSerializer;

			public:
				// Constructor
				inline SignedIntegerTypesSerializer(
					Reference<const ItemSerializer::Of<short>> shortValueSerializer,
					Reference<const ItemSerializer::Of<int>> intValueSerializer,
					Reference<const ItemSerializer::Of<long>> longValueSerializer,
					Reference<const ItemSerializer::Of<long long>> longLongValueSerializer) 
					: ItemSerializer("Signed integer types", "<short>/<int>/<long>/<long long> types")
					, m_shortValueSerializer(shortValueSerializer)
					, m_intValueSerializer(intValueSerializer)
					, m_longValueSerializer(longValueSerializer)
					, m_longLongValueSerializer(longLongValueSerializer) {}

				// Creates Sub-Serializer with given attribute factory
				template<typename AttributeFactory>
				inline static Reference<const SignedIntegerTypesSerializer> Create() {
					return Object::Instantiate<SignedIntegerTypesSerializer>(
						Serialization::ValueSerializer<short>::Create("short", "<short> value", 
							AttributeFactory::template CreateAttributes<short>()),
						Serialization::ValueSerializer<int>::Create("int", "<int> value", 
							AttributeFactory::template CreateAttributes<int>()),
						Serialization::ValueSerializer<long>::Create("long", "<long> value",
							AttributeFactory::template CreateAttributes<long>()),
						Serialization::ValueSerializer<long long>::Create("long long", "<long long> value",
							AttributeFactory::template CreateAttributes<long long>()));
				}

				// Exposes fields
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AllTypes::IntegerTypes::Signed* target)const final override {
					recordElement(m_shortValueSerializer->Serialize(target->shortValue));
					recordElement(m_intValueSerializer->Serialize(target->intValue));
					recordElement(m_longValueSerializer->Serialize(target->longValue));
					recordElement(m_longLongValueSerializer->Serialize(target->longLongValue));
				}
			};

			// Sub-serializer for AllTypes::IntegerTypes::Unsigned
			class UnsignedIntegerTypesSerializer : public virtual Serialization::SerializerList::From<AllTypes::IntegerTypes::Unsigned> {
			private:
				const Reference<const ItemSerializer::Of<unsigned short>> m_unsignedShortValueSerializer;
				const Reference<const ItemSerializer::Of<unsigned int>> m_unsignedIntValueSerializer;
				const Reference<const ItemSerializer::Of<unsigned long>> m_unsignedLongValueSerializer;
				const Reference<const ItemSerializer::Of<unsigned long long>> m_unsignedLongLongValueSerializer;

			public:
				// Constructor
				inline UnsignedIntegerTypesSerializer(
					Reference<const ItemSerializer::Of<unsigned short>> unsignedShortValueSerializer,
					Reference<const ItemSerializer::Of<unsigned int>> unsignedIntValueSerializer,
					Reference<const ItemSerializer::Of<unsigned long>> unsignedLongValueSerializer,
					Reference<const ItemSerializer::Of<unsigned long long>> unsignedLongLongValueSerializer)
					: ItemSerializer("Unsigned integer types", "<unsigned short>/<int>/<long>/<long long> types")
					, m_unsignedShortValueSerializer(unsignedShortValueSerializer)
					, m_unsignedIntValueSerializer(unsignedIntValueSerializer)
					, m_unsignedLongValueSerializer(unsignedLongValueSerializer)
					, m_unsignedLongLongValueSerializer(unsignedLongLongValueSerializer) {}

				// Creates Sub-Serializer with given attribute factory
				template<typename AttributeFactory>
				inline static Reference<const UnsignedIntegerTypesSerializer> Create() {
					return Object::Instantiate<UnsignedIntegerTypesSerializer>(
						Serialization::ValueSerializer<unsigned short>::Create("unsigned short", "<unsigned short> value"
							, AttributeFactory::template CreateAttributes<unsigned short>()),
						Serialization::ValueSerializer<unsigned int>::Create("unsigned int", "<unsigned int> value"
							, AttributeFactory::template CreateAttributes<unsigned int>()),
						Serialization::ValueSerializer<unsigned long>::Create("unsigned long", "<unsigned long> value"
							, AttributeFactory::template CreateAttributes<unsigned long>()),
						Serialization::ValueSerializer<unsigned long long>::Create("unsigned long long", "<unsigned long long> value"
							, AttributeFactory::template CreateAttributes<unsigned long long>()));
				}

				// Exposes fields
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AllTypes::IntegerTypes::Unsigned* target)const final override {
					recordElement(m_unsignedShortValueSerializer->Serialize(target->unsignedShortValue));
					recordElement(m_unsignedIntValueSerializer->Serialize(target->unsignedIntValue));
					recordElement(m_unsignedLongValueSerializer->Serialize(target->unsignedLongValue));
					recordElement(m_unsignedLongLongValueSerializer->Serialize(target->unsignedLongLongValue));
				}
			};

			// Sub-serializer for AllTypes::IntegerTypes
			class IntegerTypesSerializer : public virtual Serialization::SerializerList::From<AllTypes::IntegerTypes> {
			private:
				const Reference<const ItemSerializer::Of<AllTypes::IntegerTypes::Signed>> m_signedTypesSerializer;
				const Reference<const ItemSerializer::Of<AllTypes::IntegerTypes::Unsigned>> m_unsignedTypesSerializer;

			public:
				// Constructor
				inline IntegerTypesSerializer(
					Reference<const ItemSerializer::Of<AllTypes::IntegerTypes::Signed>> signedTypesSerializer,
					Reference<const ItemSerializer::Of<AllTypes::IntegerTypes::Unsigned>> unsignedTypesSerializer)
					: ItemSerializer("Integer types", "(signed/unsigned) <short>/<int>/<long>/<long long> Types")
					, m_signedTypesSerializer(signedTypesSerializer)
					, m_unsignedTypesSerializer(unsignedTypesSerializer) {}

				// Creates Sub-Serializer with given attribute factory
				template<typename AttributeFactory>
				inline static Reference<const IntegerTypesSerializer> Create() {
					return Object::Instantiate<IntegerTypesSerializer>(
						SignedIntegerTypesSerializer::Create<AttributeFactory>(),
						UnsignedIntegerTypesSerializer::Create<AttributeFactory>());
				}

				// Exposes fields
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AllTypes::IntegerTypes* target)const final override {
					recordElement(m_signedTypesSerializer->Serialize(target->signedTypes));
					recordElement(m_unsignedTypesSerializer->Serialize(target->unsignedTypes));
				}
			};

			// Sub-serializer for AllTypes::FloatingPointTypes
			class FloatingPointTypesSerializer : public virtual Serialization::SerializerList::From<AllTypes::FloatingPointTypes> {
			private:
				const Reference<const ItemSerializer::Of<float>> m_floatValueSerializer;
				const Reference<const ItemSerializer::Of<double>> m_doubleValueSerializer;

			public:
				// Constructor
				inline FloatingPointTypesSerializer(
					Reference<const ItemSerializer::Of<float>> floatValueSerializer,
					Reference<const ItemSerializer::Of<double>> doubleValueSerializer)
					: ItemSerializer("Floating point types", "<float>/<double> types")
					, m_floatValueSerializer(floatValueSerializer)
					, m_doubleValueSerializer(doubleValueSerializer) {}

				// Creates Sub-Serializer with given attribute factory
				template<typename AttributeFactory>
				inline static Reference<const FloatingPointTypesSerializer> Create() {
					return Object::Instantiate<FloatingPointTypesSerializer>(
						Serialization::ValueSerializer<float>::Create("float", "<float> value", 
							AttributeFactory::template CreateAttributes<float>()),
						Serialization::ValueSerializer<double>::Create("double", "<double> value", 
							AttributeFactory::template CreateAttributes<double>()));
				}

				// Exposes fields
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AllTypes::FloatingPointTypes* target)const final override {
					recordElement(m_floatValueSerializer->Serialize(target->floatValue));
					recordElement(m_doubleValueSerializer->Serialize(target->doubleValue));
				}
			};

			// Sub-serializer for AllTypes::VectorTypes
			class VectorTypesSerializer : public virtual Serialization::SerializerList::From<AllTypes::VectorTypes> {
			private:
				const Reference<const ItemSerializer::Of<Vector2>> m_vector2Serializer;
				const Reference<const ItemSerializer::Of<Vector3>> m_vector3Serializer;
				const Reference<const ItemSerializer::Of<Vector4>> m_vector4Serializer;

			public:
				// Constructor
				inline VectorTypesSerializer(
					Reference<const ItemSerializer::Of<Vector2>> vector2Serializer,
					Reference<const ItemSerializer::Of<Vector3>> vector3Serializer,
					Reference<const ItemSerializer::Of<Vector4>> vector4Serializer)
					: ItemSerializer("Vector types", "<Vector2>/<Vector3>/<Vector4> types")
					, m_vector2Serializer(vector2Serializer)
					, m_vector3Serializer(vector3Serializer)
					, m_vector4Serializer(vector4Serializer) {}

				// Creates Sub-Serializer with given attribute factory
				template<typename AttributeFactory>
				inline static Reference<const VectorTypesSerializer> Create() {
					return Object::Instantiate<VectorTypesSerializer>(
						Serialization::ValueSerializer<Vector2>::Create("Vector2", "<Vector2> value", 
							AttributeFactory::template CreateAttributes<Vector2>()),
						Serialization::ValueSerializer<Vector3>::Create("Vector3", "<Vector3> value", 
							AttributeFactory::template CreateAttributes<Vector3>()),
						Serialization::ValueSerializer<Vector4>::Create("Vector4", "<Vector4> value", 
							AttributeFactory::template CreateAttributes<Vector4>()));
				}

				// Exposes fields
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AllTypes::VectorTypes* target)const final override {
					recordElement(m_vector2Serializer->Serialize(target->vector2Value));
					recordElement(m_vector3Serializer->Serialize(target->vector3Value));
					recordElement(m_vector4Serializer->Serialize(target->vector4Value));
				}
			};

			// Sub-serializer for AllTypes::MatrixTypes
			class MatrixTypesSerializer : public virtual Serialization::SerializerList::From<AllTypes::MatrixTypes> {
			private:
				const Reference<const ItemSerializer::Of<Matrix2>> m_Matrix2Serializer;
				const Reference<const ItemSerializer::Of<Matrix3>> m_Matrix3Serializer;
				const Reference<const ItemSerializer::Of<Matrix4>> m_Matrix4Serializer;

			public:
				// Constructor
				inline MatrixTypesSerializer(
					Reference<const ItemSerializer::Of<Matrix2>> Matrix2Serializer,
					Reference<const ItemSerializer::Of<Matrix3>> Matrix3Serializer,
					Reference<const ItemSerializer::Of<Matrix4>> Matrix4Serializer)
					: ItemSerializer("Matrix types", "<Matrix2>/<Matrix3>/<Matrix4> types")
					, m_Matrix2Serializer(Matrix2Serializer)
					, m_Matrix3Serializer(Matrix3Serializer)
					, m_Matrix4Serializer(Matrix4Serializer) {}

				// Creates Sub-Serializer with given attribute factory
				template<typename AttributeFactory>
				inline static Reference<const MatrixTypesSerializer> Create() {
					return Object::Instantiate<MatrixTypesSerializer>(
						Serialization::ValueSerializer<Matrix2>::Create("Matrix2", "<Matrix2> value", 
							AttributeFactory::template CreateAttributes<Matrix2>()),
						Serialization::ValueSerializer<Matrix3>::Create("Matrix3", "<Matrix3> value", 
							AttributeFactory::template CreateAttributes<Matrix3>()),
						Serialization::ValueSerializer<Matrix4>::Create("Matrix4", "<Matrix4> value", 
							AttributeFactory::template CreateAttributes<Matrix4>()));
				}

				// Exposes fields
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AllTypes::MatrixTypes* target)const final override {
					recordElement(m_Matrix2Serializer->Serialize(target->matrix2Value));
					recordElement(m_Matrix3Serializer->Serialize(target->matrix3Value));
					recordElement(m_Matrix4Serializer->Serialize(target->matrix4Value));
				}
			};

			// Sub-serializer for AllTypes::MatrixTypes
			class StringTypesSerializer : public virtual Serialization::SerializerList::From<AllTypes::StringTypes> {
			private:
				const Reference<const ItemSerializer::Of<std::string>> m_stringValueSerializer;
				const Reference<const ItemSerializer::Of<std::wstring>> m_wstringValueSerializer;

			public:
				// Constructor
				inline StringTypesSerializer(
					Reference<const ItemSerializer::Of<std::string>> stringValueSerializer,
					Reference<const ItemSerializer::Of<std::wstring>> wstringValueSerializer)
					: ItemSerializer("String types", "<std::string>/<std::wstring> types")
					, m_stringValueSerializer(stringValueSerializer)
					, m_wstringValueSerializer(wstringValueSerializer) {}

				// Creates Sub-Serializer with given attribute factory
				template<typename AttributeFactory>
				inline static Reference<const StringTypesSerializer> Create() {
					return Object::Instantiate<StringTypesSerializer>(
						Serialization::ValueSerializer<std::string_view>::For<std::string>("std::string", "<std::string> value",
							[](std::string* text) -> std::string_view { return *text; },
							[](const std::string_view& value, std::string* text) { (*text) = value; },
							AttributeFactory::template CreateAttributes<std::string_view>()),
						Serialization::ValueSerializer<std::wstring_view>::For<std::wstring>("std::wstring", "<std::wstring> value",
							[](std::wstring* text) -> std::wstring_view { return *text; },
							[](const std::wstring_view& value, std::wstring* text) { (*text) = value; },
							AttributeFactory::template CreateAttributes<std::wstring_view>()));
				}

				// Exposes fields
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AllTypes::StringTypes* target)const final override {
					recordElement(m_stringValueSerializer->Serialize(target->stringValue));
					recordElement(m_wstringValueSerializer->Serialize(target->wideStringValue));
				}
			};

			// Sub-serializer for AllTypes
			class AllTypesSerializer : public virtual Serialization::SerializerList::From<AllTypes> {
			private:
				const Reference<const ItemSerializer::Of<bool>> m_boolValueSerializer;
				const Reference<const ItemSerializer::Of<AllTypes::CharacterTypes>> m_characterTypesSerializer;
				const Reference<const ItemSerializer::Of<AllTypes::IntegerTypes>> m_integerTypesSerializer;
				const Reference<const ItemSerializer::Of<AllTypes::FloatingPointTypes>> m_floatingPointTypesSerializer;
				const Reference<const ItemSerializer::Of<AllTypes::VectorTypes>> m_vectorTypesSerializer;
				const Reference<const ItemSerializer::Of<AllTypes::MatrixTypes>> m_matrixTypesSerializer;
				const Reference<const ItemSerializer::Of<AllTypes::StringTypes>> m_stringTypesSerializer;


			public:
				// Constructor
				inline AllTypesSerializer(
					const std::string_view& name, const std::string_view& hint,
					Reference<const ItemSerializer::Of<bool>> boolValueSerializer,
					Reference<const ItemSerializer::Of<AllTypes::CharacterTypes>> characterTypesSerializer,
					Reference<const ItemSerializer::Of<AllTypes::IntegerTypes>> integerTypesSerializer,
					Reference<const ItemSerializer::Of<AllTypes::FloatingPointTypes>> floatingPointTypesSerializer,
					Reference<const ItemSerializer::Of<AllTypes::VectorTypes>> vectorTypesSerializer,
					Reference<const ItemSerializer::Of<AllTypes::MatrixTypes>> matrixTypesSerializer,
					Reference<const ItemSerializer::Of<AllTypes::StringTypes>> stringTypesSerializer)
					: ItemSerializer(name, hint)
					, m_boolValueSerializer(boolValueSerializer)
					, m_characterTypesSerializer(characterTypesSerializer)
					, m_integerTypesSerializer(integerTypesSerializer)
					, m_floatingPointTypesSerializer(floatingPointTypesSerializer)
					, m_vectorTypesSerializer(vectorTypesSerializer)
					, m_matrixTypesSerializer(matrixTypesSerializer)
					, m_stringTypesSerializer(stringTypesSerializer) {}

				// Creates Sub-Serializer with given attribute factory
				template<typename AttributeFactory>
				inline static Reference<const AllTypesSerializer> Create(const std::string_view& name, const std::string_view& hint) {
					return Object::Instantiate<AllTypesSerializer>(name, hint,
						Serialization::ValueSerializer<bool>::Create("bool", "Boolean value",
							AttributeFactory::template CreateAttributes<bool>()),
						CharacterTypesSerializer::Create<AttributeFactory>(),
						IntegerTypesSerializer::Create<AttributeFactory>(),
						FloatingPointTypesSerializer::Create<AttributeFactory>(),
						VectorTypesSerializer::Create<AttributeFactory>(),
						MatrixTypesSerializer::Create<AttributeFactory>(),
						StringTypesSerializer::Create<AttributeFactory>());
				}

				// Exposes fields
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AllTypes* target)const final override {
					recordElement(m_boolValueSerializer->Serialize(target->boolValue));
					recordElement(m_characterTypesSerializer->Serialize(target->characterTypes));
					recordElement(m_integerTypesSerializer->Serialize(target->integerTypes));
					recordElement(m_floatingPointTypesSerializer->Serialize(target->floatingPointTypes));
					recordElement(m_vectorTypesSerializer->Serialize(target->vectorTypes));
					recordElement(m_matrixTypesSerializer->Serialize(target->matrixTypes));
					recordElement(m_stringTypesSerializer->Serialize(target->stringTypes));
				}
			};

			const Reference<const Serialization::SerializerList::From<AllTypes>> m_allTypesNoAttributesSerializer =
				AllTypesSerializer::Create<NoAttributeFactory>(
					"All Types No Attributes", "All value types, with no attributes");

			const Reference<const Serialization::SerializerList::From<AllTypes>> m_allTypesColorAttributeSerializer =
				AllTypesSerializer::Create<ColorAttributeFactory>(
					"All Types Color Attribute", "All value types, with Serialization::ColorAttribute (only 3d/4d vectors should be affected)");

			const Reference<const Serialization::SerializerList::From<AllTypes>> m_allTypesEnumAttributeSerializer =
				AllTypesSerializer::Create<EnumAttributeFactory>(
					"All Types Enum Attribute", "All value types, with Serialization::EnumAttribute (only value types should be affected)");

			const Reference<const Serialization::SerializerList::From<AllTypes>> m_allTypesBitmaskAttributeSerializer =
				AllTypesSerializer::Create<BitmaskAttributeFactory>(
					"All Types Enum(bitmask) Attribute", 
					"All value types, with Serialization::EnumAttribute with bitmask flag (only value types should be affected; integer types should act as bitmasks)");

			const Reference<const Serialization::SerializerList::From<AllTypes>> m_allTypesEulerAnglesAttributeSerializer =
				AllTypesSerializer::Create<EulerAnglesAttributeFactory>(
					"All Types EulerAngles Attribute",
					"All value types, with Serialization::EulerAnglesAttribute with bitmask flag (3d vectors should be affected)");

			const Reference<const Serialization::SerializerList::From<AllTypes>> m_allTypesSliderAttributeSerializer =
				AllTypesSerializer::Create<SliderAttributeFactory>(
					"All Types Slider Attribute",
					"All value types, with Serialization::SliderAttribute with bitmask flag (only scalar types should be affected)");

		public:
			// Constructor
			inline Serializer() 
				: ItemSerializer("Jimara/Samples/ComponentFieldTypes", 
					"Sample component for showcasing component field types (Completely unimportant behaviour-wise; serves only as a sample for testing/learning)") {}

			// Main serializer instance
			static const ComponentFieldTypes::Serializer* Instance() {
				static const ComponentFieldTypes::Serializer instance;
				return &instance;
			}

			// Exposes fields
			virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, ComponentFieldTypes* target)const final override {
				recordElement(m_allTypesNoAttributesSerializer->Serialize(target->m_allTypesNoAttributes));
				recordElement(m_allTypesColorAttributeSerializer->Serialize(target->m_allTypesColorAttribute));
				recordElement(m_allTypesEnumAttributeSerializer->Serialize(target->m_allTypesEnumAttribute));
				recordElement(m_allTypesBitmaskAttributeSerializer->Serialize(target->m_allTypesBitmaskEnumAttribute));
				recordElement(m_allTypesEulerAnglesAttributeSerializer->Serialize(target->m_allTypesEulerAnglesAttribute));
				recordElement(m_allTypesSliderAttributeSerializer->Serialize(target->m_allTypesSliderAttribute));
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Samples::ComponentFieldTypes>(const Callback<const Object*>& report) {
		report(Samples::ComponentFieldTypes::Serializer::Instance());
	}
}
