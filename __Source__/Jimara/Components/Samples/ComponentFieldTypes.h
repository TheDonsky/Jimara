#pragma once
#include "../Component.h"


namespace Jimara {
	namespace Samples {
		/// <summary> Exposes ComponentFieldTypes through BuiltInTypeRegistrator </summary>
		JIMARA_REGISTER_TYPE(Jimara::Samples::ComponentFieldTypes);

		/// <summary>
		/// This is a sample component, only made to test editor display of serialized fields; therefore, it's rather meaningless to any game;
		/// However, one may find implementation useful to understand how serialization works.
		/// </summary>
		class ComponentFieldTypes final : public virtual Component {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent object </param>
			inline ComponentFieldTypes(Component* parent) : Component(parent, "SerializedFieldTypes") {}

		private:
			/// <summary>
			/// Simple structure, containing all value types our Serialization interface is aware of
			/// </summary>
			struct AllTypes {
				/// <summary> BOOL_VALUE </summary>
				bool boolValue = false;

				/// <summary>
				/// All character types (Serialized as SERIALIZER_LIST)
				/// </summary>
				struct CharacterTypes {
					/// <summary> CHAR_VALUE </summary>
					char charValue = 'A';

					/// <summary> SCHAR_VALUE </summary>
					signed char signedCharValue = 'B';

					/// <summary> UCHAR_VALUE </summary>
					unsigned char unsignedCharValue = 'C';

					/// <summary> WCHAR_VALUE </summary>
					wchar_t wideCharValue = L'ჭ';
				} characterTypes;

				/// <summary>
				/// All integer types (Serialized as SERIALIZER_LIST)
				/// </summary>
				struct IntegerTypes {
					/// <summary>
					/// All signed integer types (Serialized as SERIALIZER_LIST)
					/// </summary>
					struct Signed {
						/// <summary> SHORT_VALUE </summary>
						short shortValue = -1;

						/// <summary> INT_VALUE </summary>
						int intValue = -2;

						/// <summary> LONG_VALUE </summary>
						long longValue = -3;

						/// <summary> LONG_LONG_VALUE </summary>
						long long longLongValue = -4;
					} signedTypes;

					/// <summary>
					/// All unsigned integer types (Serialized as SERIALIZER_LIST)
					/// </summary>
					struct Unsigned {
						/// <summary> USHORT_VALUE </summary>
						unsigned short unsignedShortValue = 1;

						/// <summary> UINT_VALUE </summary>
						unsigned int unsignedIntValue = 2;

						/// <summary> ULONG_VALUE </summary>
						unsigned long unsignedLongValue = 3;

						/// <summary> ULONG_LONG_VALUE </summary>
						unsigned long long unsignedLongLongValue = 4;
					} unsignedTypes;
				} integerTypes;

				/// <summary>
				/// All floating point types (Serialized as SERIALIZER_LIST)
				/// </summary>
				struct FloatingPointTypes {
					/// <summary> FLOAT_VALUE </summary>
					float floatValue = 3.14f;

					/// <summary> DOUBLE_VALUE </summary>
					double doubleValue = 9.99f;
				} floatingPointTypes;

				/// <summary>
				/// All spatial vector types (Serialized as SERIALIZER_LIST)
				/// </summary>
				struct VectorTypes {
					/// <summary> VECTOR2_VALUE </summary>
					Vector2 vector2Value = Vector2(2.0f, 2.0f);

					/// <summary> VECTOR3_VALUE </summary>
					Vector3 vector3Value = Vector3(3.0f, 3.0f, 3.0f);

					/// <summary> VECTOR4_VALUE </summary>
					Vector4 vector4Value = Vector4(4.0f, 4.0f, 4.0f, 4.0f);
				} vectorTypes;

				/// <summary>
				/// All matrix types (Serialized as SERIALIZER_LIST)
				/// </summary>
				struct MatrixTypes {
					/// <summary> MATRIX2_VALUE </summary>
					Matrix2 matrix2Value = Matrix2(Vector2(0.0f, 0.1f), Vector2(1.0f, 1.1f));

					/// <summary> MATRIX3_VALUE </summary>
					Matrix3 matrix3Value = Matrix3(Vector3(0.0f, 0.1f, 0.2f), Vector3(1.0f, 1.1f, 1.2f), Vector3(2.0f, 2.1f, 2.2f));

					/// <summary> MATRIX4_VALUE </summary>
					Matrix4 matrix4Value = Matrix4(Vector4(0.0f, 0.1f, 0.2f, 0.3f), Vector4(1.0f, 1.1f, 1.2f, 1.3f), Vector4(2.0f, 2.1f, 2.2f, 2.3f), Vector4(3.0f, 3.1f, 3.2f, 3.3f));
				} matrixTypes;

				/// <summary>
				/// All string types (Serialized as SERIALIZER_LIST)
				/// </summary>
				struct StringTypes {
					/// <summary> STRING_VIEW_VALUE </summary>
					std::string stringValue = "Text";

					/// <summary> WSTRING_VIEW_VALUE </summary>
					std::wstring wideStringValue = L"ტექსტი";
				} stringTypes;

				/// <summary>
				/// Two different kinds of OBJECT_PTR_VALUE-s
				/// </summary>
				struct ObjectPointers {
					/// <summary> OBJECT_PTR_VALUE </summary>
					Reference<Component> component;

					/// <summary> OBJECT_PTR_VALUE </summary>
					Reference<Resource> resource;

					/// <summary> OBJECT_PTR_VALUE </summary>
					Reference<Asset> asset;
				} objectPointers;
			};

			// All types and no attributes
			AllTypes m_allTypesNoAttributes;

			// All types with Color attribute
			AllTypes m_allTypesColorAttribute;

			// All types with enum attribute
			AllTypes m_allTypesEnumAttribute;

			// All types with enum attribute
			AllTypes m_allTypesBitmaskEnumAttribute;

			// All types with euler angles attribute
			AllTypes m_allTypesEulerAnglesAttribute;

			// All types with hide in editor attribute
			AllTypes m_allTypesHideInEditorAttribute;

			// All types with slider attribute
			AllTypes m_allTypesSliderAttribute;

		public:
			// Serializer
			class Serializer;

			// TypeIdDetails has to access the Serializer
			friend class TypeIdDetails;
		};
	}

	// ypeIdDetails::GetTypeAttributesOf exposes the serializer
	template<> void TypeIdDetails::GetTypeAttributesOf<Samples::ComponentFieldTypes>(const Callback<const Object*>& report);
}
