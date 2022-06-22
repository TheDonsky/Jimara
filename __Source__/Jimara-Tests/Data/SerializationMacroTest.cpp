#include "../GtestHeaders.h"
#include "../Memory.h"
#include <Data/Serialization/Helpers/SerializerMacros.h>
#include <Data/Serialization/Helpers/SerializeToJson.h>


namespace Jimara {
	namespace {
		static Object DEFAULT_POINTER_ADDRESS[1];
		static Object OTHER_POINTER_ADDRESS[1];

		struct SerializableValues {
			// Value types:
			bool booleanValue = true;
			char charValue = 'a';
			signed char scharValue = 'B';
			unsigned char ucharValue = 'C';
			wchar_t wcharValue = L'0';
			short shortValue = -64;
			unsigned short ushortValue = 2301u;
			int intValue = -129;
			unsigned int uintValue = 2091u;
			long longValue = -20191;
			unsigned long ulongValue = 1224345u;
			long long longLongValue = -19203;
			unsigned long long ulongLongValue = 9291u;
			float floatValue = -23.4112f;
			double doubleValue = 2324211.12441;
			Vector2 vector2Value = Vector2(0.124f, 992.12f);
			Vector3 vector3Value = Vector3(1.01f, 187.1f, 765.18f);
			Vector4 vector4Value = Vector4(0.09878f, 2.786f, 49.2345f, 1.287f);
			Matrix2 matrix2Value = Matrix2(Vector2(1.0f, 2.0f), Vector2(3.0f, 4.0f));
			Matrix3 matrix3Value = Matrix3(Vector3(0.0f, 2.0f, 4.0f), Vector3(8.0f, 16.0f, 32.0f), Vector3(64.0f, 128.0f, 256.0f));
			Matrix4 matrix4Value = Matrix4(Vector4(2.0f, -4.0f, 3.0f, -1.0f), Vector4(209.0f), Vector4(900.0f), Vector4(-0.23f));
			std::string stringValue = "StringValue";
			std::wstring wstringValue = L"Wide string value";
			Object* objectPointerValue = DEFAULT_POINTER_ADDRESS;

			// Serializer for direct values:
			struct ValueSerializer : public virtual Serialization::SerializerList::From<SerializableValues> {
				inline ValueSerializer() : Serialization::ItemSerializer("ValueSerializer") {}
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, SerializableValues* target)const final override {
					JIMARA_SERIALIZE_FIELDS(target, recordElement, {
						JIMARA_SERIALIZE_FIELD(target->booleanValue, "bool", "booleanValue");
						JIMARA_SERIALIZE_FIELD(target->charValue, "char", "charValue");
						JIMARA_SERIALIZE_FIELD(target->scharValue, "signed char", "scharValue");
						JIMARA_SERIALIZE_FIELD(target->ucharValue, "unsigned char", "ucharValue");
						JIMARA_SERIALIZE_FIELD(target->wcharValue, "wide char", "wcharValue");
						JIMARA_SERIALIZE_FIELD(target->shortValue, "short", "shortValue");
						JIMARA_SERIALIZE_FIELD(target->ushortValue, "unsigned Short", "ushortValue");
						JIMARA_SERIALIZE_FIELD(target->intValue, "int", "intValue");
						JIMARA_SERIALIZE_FIELD(target->uintValue, "unsigned int", "uintValue");
						JIMARA_SERIALIZE_FIELD(target->longValue, "long", "longValue");
						JIMARA_SERIALIZE_FIELD(target->ulongValue, "unsigned long", "ulongValue");
						JIMARA_SERIALIZE_FIELD(target->longLongValue, "long long", "longLongValue");
						JIMARA_SERIALIZE_FIELD(target->ulongLongValue, "unsigned long long", "ulongLongValue");
						JIMARA_SERIALIZE_FIELD(target->floatValue, "float", "floatValue");
						JIMARA_SERIALIZE_FIELD(target->doubleValue, "double", "doubleValue");
						JIMARA_SERIALIZE_FIELD(target->vector2Value, "Vector2", "vector2Value");
						JIMARA_SERIALIZE_FIELD(target->vector3Value, "Vector3", "vector3Value");
						JIMARA_SERIALIZE_FIELD(target->vector4Value, "Vector4", "vector4Value");
						JIMARA_SERIALIZE_FIELD(target->matrix2Value, "Matrix2", "matrix2Value");
						JIMARA_SERIALIZE_FIELD(target->matrix3Value, "Matrix3", "matrix3Value");
						JIMARA_SERIALIZE_FIELD(target->matrix4Value, "Matrix4", "matrix4Value");
						JIMARA_SERIALIZE_FIELD(target->stringValue, "std::string", "stringValue");
						JIMARA_SERIALIZE_FIELD(target->wstringValue, "std::wstring", "wstringValue");
						JIMARA_SERIALIZE_FIELD(target->objectPointerValue, "Object*", "objectPointerValue");
						});
				}
			};

			// References:
			bool& BoolReference() { return booleanValue; }
			inline char& CharReference() { return charValue; }
			signed char& ScharReference() { return scharValue; }
			unsigned char& UcharReference() { return ucharValue; }
			wchar_t& WcharReference() { return wcharValue; }
			inline short& ShortReference() { return shortValue; }
			unsigned short& UshortReference() { return ushortValue; }
			int& IntReference() { return intValue; }
			unsigned int& UintReference() { return uintValue; }
			long& LongReference() { return longValue; }
			unsigned long& UlongReference() { return ulongValue; }
			long long& LongLongReference() { return longLongValue; }
			inline unsigned long long& UlongLongReference() { return ulongLongValue; }
			float& FloatReference() { return floatValue; }
			double& DoubleReference() { return doubleValue; }
			Vector2& Vector2Reference() { return vector2Value; }
			Vector3& Vector3Reference() { return vector3Value; }
			inline Vector4& Vector4Reference() { return vector4Value; }
			Matrix2& Matrix2Reference() { return matrix2Value; }
			Matrix3& Matrix3Reference() { return matrix3Value; }
			inline Matrix4& Matrix4Reference() { return matrix4Value; }
			std::string& StringReference() { return stringValue; }
			inline std::wstring& WstringReference() { return wstringValue; }
			Object*& ObjectPointerReference() { return objectPointerValue; }

			// Serializer for reference functions:
			struct ReferenceSerializer : public virtual Serialization::SerializerList::From<SerializableValues> {
				inline ReferenceSerializer() : Serialization::ItemSerializer("ReferenceSerializer") {}
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, SerializableValues* target)const final override {
					JIMARA_SERIALIZE_FIELDS(target, recordElement, {
						JIMARA_SERIALIZE_FIELD(target->BoolReference(), "bool", "BoolReference()");
						JIMARA_SERIALIZE_FIELD(target->CharReference(), "char", "CharReference()");
						JIMARA_SERIALIZE_FIELD(target->ScharReference(), "signed char", "ScharReference()");
						JIMARA_SERIALIZE_FIELD(target->UcharReference(), "unsigned char", "UcharReference()");
						JIMARA_SERIALIZE_FIELD(target->WcharReference(), "wide char", "WcharReference()");
						JIMARA_SERIALIZE_FIELD(target->ShortReference(), "short", "ShortReference()");
						JIMARA_SERIALIZE_FIELD(target->UshortReference(), "unsigned Short", "UshortReference()");
						JIMARA_SERIALIZE_FIELD(target->IntReference(), "int", "intReference()");
						JIMARA_SERIALIZE_FIELD(target->UintReference(), "unsigned int", "UintReference()");
						JIMARA_SERIALIZE_FIELD(target->LongReference(), "long", "LongReference()");
						JIMARA_SERIALIZE_FIELD(target->UlongReference(), "unsigned long", "UlongReference()");
						JIMARA_SERIALIZE_FIELD(target->LongLongReference(), "long long", "LongLongReference()");
						JIMARA_SERIALIZE_FIELD(target->UlongLongReference(), "unsigned long long", "UlongLongReference()");
						JIMARA_SERIALIZE_FIELD(target->FloatReference(), "float", "FloatReference()");
						JIMARA_SERIALIZE_FIELD(target->DoubleReference(), "double", "DoubleReference()");
						JIMARA_SERIALIZE_FIELD(target->Vector2Reference(), "Vector2", "Vector2Reference()");
						JIMARA_SERIALIZE_FIELD(target->Vector3Reference(), "Vector3", "Vector3Reference()");
						JIMARA_SERIALIZE_FIELD(target->Vector4Reference(), "Vector4", "Vector4Reference()");
						JIMARA_SERIALIZE_FIELD(target->Matrix2Reference(), "Matrix2", "Matrix2Reference()");
						JIMARA_SERIALIZE_FIELD(target->Matrix3Reference(), "Matrix3", "Matrix3Reference()");
						JIMARA_SERIALIZE_FIELD(target->Matrix4Reference(), "Matrix4", "Matrix4Reference()");
						JIMARA_SERIALIZE_FIELD(target->StringReference(), "std::string", "StringReference()");
						JIMARA_SERIALIZE_FIELD(target->WstringReference(), "std::wstring", "WstringReference()");
						JIMARA_SERIALIZE_FIELD(target->ObjectPointerReference(), "Object*", "ObjectPointerReference()");
						});
				}
			};

			// Getters/Setters:
			bool GetBoolValue() { return booleanValue; }
			void SetBoolValue(const bool& value) { booleanValue = value; }
			char GetCharValue()const { return charValue; }
			char& SetCharValue(char value) { return charValue = value; }
			inline signed char GetScharValue()const { return scharValue; }
			SerializableValues& SetScharValue(const signed char& value) { scharValue = value; return *this; }
			unsigned char GetUcharValue() { return ucharValue; }
			void SetUcharValue(unsigned char value) { ucharValue = value; }
			inline wchar_t GetWcharValue()const { return wcharValue; }
			void SetWcharValue(const wchar_t value) { wcharValue = value; }
			short GetShortValue()const { return shortValue; }
			void SetShortValue(short value) { shortValue = value; }
			unsigned short GetUshortValue() { return ushortValue; }
			void SetUshortValue(unsigned short value) { ushortValue = value; }
			inline int GetIntValue()const { return intValue; }
			inline const SerializableValues* SetIntValue(const int& value) { intValue = value; return this; }
			inline unsigned int GetUintValue() { return uintValue; }
			virtual void SetUintValue(const unsigned int value) { uintValue = value; }
			virtual long GetLongValue()const { return longValue; }
			virtual long SetLongValue(long value) { return longValue = value; }
			inline unsigned long GetUlongValue()const { return ulongValue; }
			inline virtual void SetUlongValue(unsigned long value) { ulongValue = value; }
			long long GetLongLongValue() { return longLongValue; }
			void SetLongLongValue(long long value) { longLongValue = value; }
			unsigned long long GetUlongLongValue() { return ulongLongValue; }
			void SetUlongLongValue(unsigned long long value) { ulongLongValue = value; }
			virtual float GetFloatValue()const { return floatValue; }
			void SetFloatValue(const float& value) { floatValue = value; }
			inline double GetDoubleValue() { return doubleValue; }
			void SetDoubleValue(double value) { doubleValue = value; }
			Vector2 GetVector2Value() { return vector2Value; }
			void SetVector2Value(const Vector2& value) { vector2Value = value; }
			Vector3 GetVector3Value()const { return vector3Value; }
			Vector3& SetVector3Value(const Vector3& value) { return vector3Value = value; }
			virtual Vector4 GetVector4Value() { return vector4Value; }
			virtual const Vector4& SetVector4Value(const Vector4& value) { return vector4Value = value; }
			Matrix2 GetMatrix2Value() { return matrix2Value; }
			void SetMatrix2Value(const Matrix2& value) { matrix2Value = value; }
			Matrix3 GetMatrix3Value()const { return matrix3Value; }
			Matrix3& SetMatrix3Value(const Matrix3& value) { return matrix3Value = value; }
			virtual Matrix4 GetMatrix4Value() { return matrix4Value; }
			virtual const Matrix4& SetMatrix4Value(const Matrix4& value) { return matrix4Value = value; }
			virtual std::string_view GetStringValue()const { return stringValue; }
			virtual void SetStringValue(const std::string_view& value) { stringValue = value; }
			inline std::wstring_view GetWstringValue() { return wstringValue; }
			std::wstring& SetWstringValue(const std::wstring_view& value) { return wstringValue = value; }
			inline virtual Object* GetObjectPointerValue()const { return objectPointerValue; }
			void SetObjectPointerValue(Object* value) { objectPointerValue = value; }

			// Serializer for get/set methods:
			struct GetSetSerializer : public virtual Serialization::SerializerList::From<SerializableValues> {
				inline GetSetSerializer() : Serialization::ItemSerializer("GetSetSerializer") {}
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, SerializableValues* target)const final override {
					JIMARA_SERIALIZE_FIELDS(target, recordElement, {
						JIMARA_SERIALIZE_FIELD_GET_SET(GetBoolValue, SetBoolValue, "bool", "(Get/Set)BoolValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetCharValue, SetCharValue, "char", "(Get/Set)CharValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetScharValue, SetScharValue, "signed char", "(Get/Set)ScharValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetUcharValue, SetUcharValue, "unsigned char", "(Get/Set)UcharValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetWcharValue, SetWcharValue, "wide char", "(Get/Set)WcharValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetShortValue, SetShortValue, "short", "(Get/Set)ShortValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetUshortValue, SetUshortValue, "unsigned Short", "(Get/Set)UshortValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetIntValue, SetIntValue, "int", "(Get/Set)intValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetUintValue, SetUintValue, "unsigned int", "(Get/Set)UintValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetLongValue, SetLongValue, "long", "(Get/Set)LongValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetUlongValue, SetUlongValue, "unsigned long", "(Get/Set)UlongValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetLongLongValue, SetLongLongValue, "long long", "(Get/Set)LongLongValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetUlongLongValue, SetUlongLongValue, "unsigned long long", "(Get/Set)UlongLongValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetFloatValue, SetFloatValue, "float", "(Get/Set)FloatValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetDoubleValue, SetDoubleValue, "double", "(Get/Set)DoubleValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetVector2Value, SetVector2Value, "Vector2", "(Get/Set)Vector2Value()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetVector3Value, SetVector3Value, "Vector3", "(Get/Set)Vector3Value()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetVector4Value, SetVector4Value, "Vector4", "(Get/Set)Vector4Value()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetMatrix2Value, SetMatrix2Value, "Matrix2", "(Get/Set)Matrix2Value()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetMatrix3Value, SetMatrix3Value, "Matrix3", "(Get/Set)Matrix3Value()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetMatrix4Value, SetMatrix4Value, "Matrix4", "(Get/Set)Matrix4Value()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetStringValue, SetStringValue, "std::string", "(Get/Set)StringValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetWstringValue, SetWstringValue, "std::wstring", "(Get/Set)WstringValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(GetObjectPointerValue, SetObjectPointerValue, "Object*", "(Get/Set)ObjectPointerValue()");
						});
				}
			};

			struct GetRefSetValueSerializer : public virtual Serialization::SerializerList::From<SerializableValues> {
				inline GetRefSetValueSerializer() : Serialization::ItemSerializer("GetRefSetValueSerializer") {}
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, SerializableValues* target)const final override {
					JIMARA_SERIALIZE_FIELDS(target, recordElement, {
						JIMARA_SERIALIZE_FIELD_GET_SET(BoolReference, SetBoolValue, "bool", "(Get/Set)BoolValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(CharReference, SetCharValue, "char", "(Get/Set)CharValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(ScharReference, SetScharValue, "signed char", "(Get/Set)ScharValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(UcharReference, SetUcharValue, "unsigned char", "(Get/Set)UcharValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(WcharReference, SetWcharValue, "wide char", "(Get/Set)WcharValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(ShortReference, SetShortValue, "short", "(Get/Set)ShortValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(UshortReference, SetUshortValue, "unsigned Short", "(Get/Set)UshortValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(IntReference, SetIntValue, "int", "(Get/Set)intValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(UintReference, SetUintValue, "unsigned int", "(Get/Set)UintValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(LongReference, SetLongValue, "long", "(Get/Set)LongValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(UlongReference, SetUlongValue, "unsigned long", "(Get/Set)UlongValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(LongLongReference, SetLongLongValue, "long long", "(Get/Set)LongLongValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(UlongLongReference, SetUlongLongValue, "unsigned long long", "(Get/Set)UlongLongValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(FloatReference, SetFloatValue, "float", "(Get/Set)FloatValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(DoubleReference, SetDoubleValue, "double", "(Get/Set)DoubleValue()");
						JIMARA_SERIALIZE_FIELD_GET_SET(Vector2Reference, SetVector2Value, "Vector2", "(Get/Set)Vector2Value()");
						JIMARA_SERIALIZE_FIELD_GET_SET(Vector3Reference, SetVector3Value, "Vector3", "(Get/Set)Vector3Value()");
						JIMARA_SERIALIZE_FIELD_GET_SET(Vector4Reference, SetVector4Value, "Vector4", "(Get/Set)Vector4Value()");
						JIMARA_SERIALIZE_FIELD_GET_SET(Matrix2Reference, SetMatrix2Value, "Matrix2", "(Get/Set)Matrix2Value()");
						JIMARA_SERIALIZE_FIELD_GET_SET(Matrix3Reference, SetMatrix3Value, "Matrix3", "(Get/Set)Matrix3Value()");
						JIMARA_SERIALIZE_FIELD_GET_SET(Matrix4Reference, SetMatrix4Value, "Matrix4", "(Get/Set)Matrix4Value()");
						JIMARA_SERIALIZE_FIELD(target->StringReference(), "std::string", "StringReference()");
						JIMARA_SERIALIZE_FIELD(target->wstringValue, "std::wstring", "wstringValue");
						JIMARA_SERIALIZE_FIELD_GET_SET(ObjectPointerReference, SetObjectPointerValue, "Object*", "(Get/Set)ObjectPointerValue()");
						});
				}
			};

			// Comparators:
			inline bool operator==(const SerializableValues& other)const {
				return
					booleanValue == other.booleanValue &&
					charValue == other.charValue && scharValue == other.scharValue && ucharValue == other.ucharValue && wcharValue == other.wcharValue &&
					shortValue == other.shortValue && ushortValue == other.ushortValue &&
					intValue == other.intValue && uintValue == other.uintValue &&
					longValue == other.longValue && ulongValue == other.ulongValue &&
					longLongValue == other.longLongValue && ulongLongValue == other.ulongLongValue &&
					floatValue == other.floatValue && doubleValue == other.doubleValue &&
					vector2Value == other.vector2Value && vector3Value == other.vector3Value && vector4Value == other.vector4Value &&
					matrix2Value == other.matrix2Value && matrix3Value == other.matrix3Value && matrix4Value == other.matrix4Value &&
					stringValue == other.stringValue && wstringValue == other.wstringValue &&
					objectPointerValue == other.objectPointerValue;
			}
			inline bool operator!=(const SerializableValues& other)const { return !((*this) == other); }

			// Scramble:
			inline void Scramble() {
				booleanValue = false;
				charValue = 'k';
				scharValue = 'x';
				ucharValue = 'y';
				wcharValue = L'Q';
				shortValue = 7340;
				ushortValue = 1676;
				intValue = -203;
				uintValue = 80123;
				longValue = -34435;
				ulongValue = 89001;
				longLongValue = -245987;
				ulongLongValue = 901234;
				floatValue = 9.02112f;
				doubleValue = 10001234.01245;
				vector2Value = Vector2(-1234.0f);
				vector3Value = Vector3(99234234.1223f);
				vector4Value = Vector4(12344.01221f);
				matrix2Value = Matrix2(778123.0122f);
				matrix3Value = Matrix3(12111.f);
				matrix4Value = Matrix4(267884.0354561f);
				stringValue = "SOME RANDOM NEW STRING, DIFFERENT FROM THE ONE AT START";
				wstringValue = L"ANOTHER WIDE STRING";
				objectPointerValue = OTHER_POINTER_ADDRESS;
			}

			// Serialize/Deserialize:
			inline nlohmann::json Serialize(const Serialization::ItemSerializer::Of<SerializableValues>& serializer, bool& failed) {
				return Serialization::SerializeToJson(serializer.Serialize(this), nullptr, failed,
					[&](const Serialization::SerializedObject& object, bool&) -> nlohmann::json {
						const uint64_t value = (uint64_t)object.GetObjectValue();
						return value;
					});
			}
			inline bool Deserialize(const nlohmann::json& json, const Serialization::ItemSerializer::Of<SerializableValues>& serializer) {
				return Serialization::DeserializeFromJson(serializer.Serialize(this), json, nullptr, 
					[&](const Serialization::SerializedObject& object, const nlohmann::json& json) -> bool {
						if (!json.is_number_integer()) return false;
						object.SetObjectValue((Object*)((uint64_t)json));
						return true;
					});
			}
		};
	}

	TEST(SerializationMacroTest, ValueSerializer) {
		SerializableValues::ValueSerializer serializer;
		const SerializableValues initialValues;
		SerializableValues values = initialValues;
		EXPECT_EQ(values, initialValues);
		nlohmann::json json;
		{
			bool failed = false;
			json = values.Serialize(serializer, failed);
			EXPECT_FALSE(failed);
		}
		EXPECT_EQ(values, initialValues);
		values.Scramble();
		EXPECT_NE(values, initialValues);
		EXPECT_TRUE(values.Deserialize(json, serializer));
		EXPECT_EQ(values, initialValues);
	}

	TEST(SerializationMacroTest, ReferenceSerializer) {
		const SerializableValues::ReferenceSerializer serializer;
		const SerializableValues initialValues;
		SerializableValues values = initialValues;
		EXPECT_EQ(values, initialValues);
		nlohmann::json json;
		{
			bool failed = false;
			json = values.Serialize(serializer, failed);
			EXPECT_FALSE(failed);
		}
		EXPECT_EQ(values, initialValues);
		values.Scramble();
		EXPECT_NE(values, initialValues);
		EXPECT_TRUE(values.Deserialize(json, serializer));
		EXPECT_EQ(values, initialValues);
	}

	TEST(SerializationMacroTest, GetSetSerializer) {
		const SerializableValues::GetSetSerializer serializer;
		const SerializableValues initialValues;
		SerializableValues values = initialValues;
		EXPECT_EQ(values, initialValues);
		nlohmann::json json;
		{
			bool failed = false;
			json = values.Serialize(serializer, failed);
			EXPECT_FALSE(failed);
		}
		EXPECT_EQ(values, initialValues);
		values.Scramble();
		EXPECT_NE(values, initialValues);
		EXPECT_TRUE(values.Deserialize(json, serializer));
		EXPECT_EQ(values, initialValues);
	}

	TEST(SerializationMacroTest, GetRefSetValueSerializer) {
		const SerializableValues::GetRefSetValueSerializer serializer;
		const SerializableValues initialValues;
		SerializableValues values = initialValues;
		EXPECT_EQ(values, initialValues);
		nlohmann::json json;
		{
			bool failed = false;
			json = values.Serialize(serializer, failed);
			EXPECT_FALSE(failed);
		}
		EXPECT_EQ(values, initialValues);
		values.Scramble();
		EXPECT_NE(values, initialValues);
		EXPECT_TRUE(values.Deserialize(json, serializer));
		EXPECT_EQ(values, initialValues);
	}
}
