#include "SerializeToJson.h"
#include "../../../Core/Helpers.h"
#include <sstream>
#include <unordered_map>


namespace Jimara {
	namespace Serialization {
		namespace {
			typedef nlohmann::json(*SerializeToJsonFn)(const SerializedObject&, OS::Logger*, bool&);
			template<typename ValueType>
			inline static nlohmann::json SerializeTo(const SerializedObject& object, OS::Logger*, bool&) {
				return  nlohmann::json(object.operator ValueType());
			}
			template<typename MatrixType, size_t MatrixDimm>
			inline static nlohmann::json SerializeAsMatrix(const SerializedObject& object, OS::Logger*, bool&) {
				MatrixType m = object.operator MatrixType();
				nlohmann::json list;
				for (typename MatrixType::length_type i = 0; i < MatrixDimm; i++)
					for (typename MatrixType::length_type j = 0; j < MatrixDimm; j++)
						list.push_back(m[i][j]);
				return list;
			}
		}

		nlohmann::json SerializeToJson(const SerializedObject& object, OS::Logger* logger, bool& error,
			const Function<nlohmann::json, const SerializedObject&, bool&>& serializerObjectPtr) {
			static const SerializeToJsonFn* SERIALIZERS = []() -> const SerializeToJsonFn* {
				const constexpr size_t TYPE_COUNT = static_cast<size_t>(ItemSerializer::Type::SERIALIZER_TYPE_COUNT);
				static SerializeToJsonFn serializers[TYPE_COUNT];

				static const SerializeToJsonFn defaultSerializer = [](const SerializedObject& object, OS::Logger* logger, bool& error) -> nlohmann::json {
					if (logger != nullptr)
						logger->Error("SerializeToJson - Unsupported ItemSerializer type: ", static_cast<size_t>(object.Serializer()->GetType()), "!");
					error = true;
					return nlohmann::json();
				};
				for (size_t i = 0; i < TYPE_COUNT; i++)
					serializers[i] = defaultSerializer;

				serializers[static_cast<size_t>(ItemSerializer::Type::BOOL_VALUE)] = SerializeTo<bool>;
				serializers[static_cast<size_t>(ItemSerializer::Type::CHAR_VALUE)] = SerializeTo<char>;
				serializers[static_cast<size_t>(ItemSerializer::Type::SCHAR_VALUE)] = SerializeTo<signed char>;
				serializers[static_cast<size_t>(ItemSerializer::Type::UCHAR_VALUE)] = SerializeTo<unsigned char>;
				serializers[static_cast<size_t>(ItemSerializer::Type::WCHAR_VALUE)] = SerializeTo<wchar_t>;
				serializers[static_cast<size_t>(ItemSerializer::Type::SHORT_VALUE)] = SerializeTo<short>;
				serializers[static_cast<size_t>(ItemSerializer::Type::USHORT_VALUE)] = SerializeTo<unsigned short>;
				serializers[static_cast<size_t>(ItemSerializer::Type::INT_VALUE)] = SerializeTo<int>;
				serializers[static_cast<size_t>(ItemSerializer::Type::UINT_VALUE)] = SerializeTo<unsigned int>;
				serializers[static_cast<size_t>(ItemSerializer::Type::LONG_VALUE)] = SerializeTo<long>;
				serializers[static_cast<size_t>(ItemSerializer::Type::ULONG_VALUE)] = SerializeTo<unsigned long>;
				serializers[static_cast<size_t>(ItemSerializer::Type::LONG_LONG_VALUE)] = SerializeTo<long long>;
				serializers[static_cast<size_t>(ItemSerializer::Type::ULONG_LONG_VALUE)] = SerializeTo<unsigned long long>;
				serializers[static_cast<size_t>(ItemSerializer::Type::FLOAT_VALUE)] = SerializeTo<float>;
				serializers[static_cast<size_t>(ItemSerializer::Type::DOUBLE_VALUE)] = SerializeTo<double>;

				serializers[static_cast<size_t>(ItemSerializer::Type::VECTOR2_VALUE)] = [](const SerializedObject& object, OS::Logger*, bool&) {
					Vector2 v = object.operator Vector2();
					return nlohmann::json({ v.x, v.y });
				};
				serializers[static_cast<size_t>(ItemSerializer::Type::VECTOR3_VALUE)] = [](const SerializedObject& object, OS::Logger*, bool&) {
					Vector3 v = object.operator Vector3();
					return nlohmann::json({ v.x, v.y, v.z });
				};
				serializers[static_cast<size_t>(ItemSerializer::Type::VECTOR4_VALUE)] = [](const SerializedObject& object, OS::Logger*, bool&) {
					Vector4 v = object.operator Vector4();
					return nlohmann::json({ v.x, v.y, v.z, v.w });
				};

				serializers[static_cast<size_t>(ItemSerializer::Type::MATRIX2_VALUE)] = SerializeAsMatrix<Matrix2, 2>;
				serializers[static_cast<size_t>(ItemSerializer::Type::MATRIX3_VALUE)] = SerializeAsMatrix<Matrix3, 3>;
				serializers[static_cast<size_t>(ItemSerializer::Type::MATRIX4_VALUE)] = SerializeAsMatrix<Matrix4, 4>;

				serializers[static_cast<size_t>(ItemSerializer::Type::STRING_VIEW_VALUE)] = SerializeTo<std::string_view>;
				serializers[static_cast<size_t>(ItemSerializer::Type::WSTRING_VIEW_VALUE)] = SerializeTo<std::wstring_view>;

				return serializers;
			}();
			const ItemSerializer* serializer = object.Serializer();
			if (serializer == nullptr) {
				if (logger != nullptr)
					logger->Error("SerializeToJson - Null serializer provided!");
				error = true;
				return nlohmann::json();
			}
			ItemSerializer::Type type = serializer->GetType();
			if (type < ItemSerializer::Type::OBJECT_PTR_VALUE)
				return SERIALIZERS[static_cast<size_t>(type)](object, logger, error);
			else if (type == ItemSerializer::Type::OBJECT_PTR_VALUE)
				return serializerObjectPtr(object, error);
			else if (type == ItemSerializer::Type::SERIALIZER_LIST) {
				nlohmann::json json({});
				std::unordered_map<std::string, size_t> fieldNameCounts;
				object.GetFields([&](const SerializedObject& field) {
					if (field.Serializer() == nullptr) {
						if (logger != nullptr)
							logger->Warning("SerializeToJson - Got a field with null-serializer!");
						return;
					}
					auto name = [&]() -> std::string {
						const std::string& baseName = field.Serializer()->TargetName();
						size_t index;
						std::unordered_map<std::string, size_t>::iterator it = fieldNameCounts.find(baseName);
						if (it == fieldNameCounts.end()) {
							index = 0;
							fieldNameCounts[baseName] = index;
						}
						else {
							it->second++;
							index = it->second;
						}
						std::stringstream stream;
						stream << baseName << "[" << index << "]";
						return stream.str();
					};
					json[name()] = SerializeToJson(field, logger, error, serializerObjectPtr);
					});
				return json;
			}
			else {
				if (logger != nullptr)
					logger->Error("SerializeToJson - Serializer type out of bounds!", static_cast<size_t>(object.Serializer()->GetType()), "!");
				error = true;
				return nlohmann::json();
			}
		}


		namespace {
			typedef bool(*DeserializeToJsonFn)(const SerializedObject&, const nlohmann::json&, OS::Logger*);
			template<typename ValueType>
			inline static bool DeserializeNumber(const nlohmann::json& json, ValueType& value) {
				if (json.is_boolean()) value = static_cast<ValueType>(json.get<bool>());
				else if (json.is_number_float()) value = static_cast<ValueType>(json.get<double>());
				else if (json.is_number_unsigned()) value = static_cast<ValueType>(json.get<unsigned long long>());
				else if (json.is_number_integer()) value = static_cast<ValueType>(json.get<long long>());
				else return false;
				return true;
			}
			template<typename ValueType>
			inline static bool DeserializeNumber(const SerializedObject& object, const nlohmann::json& json, OS::Logger*) {
				ValueType value;
				if (DeserializeNumber(json, value))
					object = value;
				return true;
			}
			template<typename MatrixType, size_t MatrixDimm>
			inline static bool DeserializeMatrix(const SerializedObject& object, const nlohmann::json& json, OS::Logger*) {
				if (json.is_array()) {
					MatrixType value(0.0f);
					size_t idx = 0;
					for (typename MatrixType::length_type i = 0; i < MatrixDimm; i++)
						for (typename MatrixType::length_type j = 0; j < MatrixDimm; j++) {
							if (json.size() <= idx) break;
							DeserializeNumber(json[idx], value[i][j]);
							idx++;
						}
					object = value;
				}
				else {
					float value;
					if (DeserializeNumber(json, value))
						object = MatrixType(value);
				}
				return true;
			}
		}

		bool DeserializeFromJson(const SerializedObject& object, const nlohmann::json& json, OS::Logger* logger,
			const Function<bool, const SerializedObject&, const nlohmann::json&>& deserializerObjectPtr) {
			static const DeserializeToJsonFn* DESERIALIZERS = []() -> const DeserializeToJsonFn* {
				const constexpr size_t TYPE_COUNT = static_cast<size_t>(ItemSerializer::Type::SERIALIZER_TYPE_COUNT);
				static DeserializeToJsonFn deserializers[TYPE_COUNT];

				static const DeserializeToJsonFn defaultDeserializer = [](const SerializedObject& object, const nlohmann::json&, OS::Logger* logger) -> bool {
					if (logger != nullptr)
						logger->Error("DeserializeFromJson - Unsupported ItemSerializer type: ", static_cast<size_t>(object.Serializer()->GetType()), "!");
					return false;
				};
				for (size_t i = 0; i < TYPE_COUNT; i++)
					deserializers[i] = defaultDeserializer;

				deserializers[static_cast<size_t>(ItemSerializer::Type::BOOL_VALUE)] = [](const SerializedObject& object, const nlohmann::json& json, OS::Logger*) -> bool {
					if (json.is_boolean()) object = json.get<bool>();
					else if (json.is_number()) object = json.get<double>() != 0.0;
					return true;
				};

				deserializers[static_cast<size_t>(ItemSerializer::Type::CHAR_VALUE)] = DeserializeNumber<char>;
				deserializers[static_cast<size_t>(ItemSerializer::Type::SCHAR_VALUE)] = DeserializeNumber<signed char>;
				deserializers[static_cast<size_t>(ItemSerializer::Type::UCHAR_VALUE)] = DeserializeNumber<unsigned char>;
				deserializers[static_cast<size_t>(ItemSerializer::Type::WCHAR_VALUE)] = DeserializeNumber<wchar_t>;
				deserializers[static_cast<size_t>(ItemSerializer::Type::SHORT_VALUE)] = DeserializeNumber<short>;
				deserializers[static_cast<size_t>(ItemSerializer::Type::USHORT_VALUE)] = DeserializeNumber<unsigned short>;
				deserializers[static_cast<size_t>(ItemSerializer::Type::INT_VALUE)] = DeserializeNumber<int>;
				deserializers[static_cast<size_t>(ItemSerializer::Type::UINT_VALUE)] = DeserializeNumber<unsigned int>;
				deserializers[static_cast<size_t>(ItemSerializer::Type::LONG_VALUE)] = DeserializeNumber<long>;
				deserializers[static_cast<size_t>(ItemSerializer::Type::ULONG_VALUE)] = DeserializeNumber<unsigned long>;
				deserializers[static_cast<size_t>(ItemSerializer::Type::LONG_LONG_VALUE)] = DeserializeNumber<long long>;
				deserializers[static_cast<size_t>(ItemSerializer::Type::ULONG_LONG_VALUE)] = DeserializeNumber<unsigned long long>;
				deserializers[static_cast<size_t>(ItemSerializer::Type::FLOAT_VALUE)] = DeserializeNumber<float>;
				deserializers[static_cast<size_t>(ItemSerializer::Type::DOUBLE_VALUE)] = DeserializeNumber<double>;

				deserializers[static_cast<size_t>(ItemSerializer::Type::VECTOR2_VALUE)] = [](const SerializedObject& object, const nlohmann::json& json, OS::Logger*) -> bool {
					if (json.is_array()) {
						Vector2 v(0.0f);
						if (json.size() >= 1) {
							DeserializeNumber(json[0], v.x);
							if (json.size() >= 2) DeserializeNumber(json[1], v.y);
						}
						object = v;
					}
					else {
						float f;
						if (DeserializeNumber(json, f))
							object = Vector2(f);
					}
					return true;
				};
				deserializers[static_cast<size_t>(ItemSerializer::Type::VECTOR3_VALUE)] = [](const SerializedObject& object, const nlohmann::json& json, OS::Logger*) -> bool {
					if (json.is_array()) {
						Vector3 v(0.0f);
						if (json.size() >= 1) {
							DeserializeNumber(json[0], v.x);
							if (json.size() >= 2) {
								DeserializeNumber(json[1], v.y);
								if (json.size() >= 3) DeserializeNumber(json[2], v.z);
							}
						}
						object = v;
					}
					else {
						float f;
						if (DeserializeNumber(json, f))
							object = Vector3(f);
					}
					return true;
				};
				deserializers[static_cast<size_t>(ItemSerializer::Type::VECTOR4_VALUE)] = [](const SerializedObject& object, const nlohmann::json& json, OS::Logger*) -> bool {
					if (json.is_array()) {
						Vector4 v(0.0f);
						if (json.size() >= 1) {
							DeserializeNumber(json[0], v.x);
							if (json.size() >= 2) {
								DeserializeNumber(json[1], v.y);
								if (json.size() >= 3) {
									DeserializeNumber(json[2], v.z);
									if (json.size() >= 4) DeserializeNumber(json[3], v.w);
								}
							}
						}
						object = v;
					}
					else {
						float f;
						if (DeserializeNumber(json, f))
							object = Vector4(f);
					}
					return true;
				};
				

				deserializers[static_cast<size_t>(ItemSerializer::Type::MATRIX2_VALUE)] = DeserializeMatrix<Matrix2, 2>;
				deserializers[static_cast<size_t>(ItemSerializer::Type::MATRIX3_VALUE)] = DeserializeMatrix<Matrix3, 3>;
				deserializers[static_cast<size_t>(ItemSerializer::Type::MATRIX4_VALUE)] = DeserializeMatrix<Matrix4, 4>;

				deserializers[static_cast<size_t>(ItemSerializer::Type::STRING_VIEW_VALUE)] = [](const SerializedObject& object, const nlohmann::json& json, OS::Logger*) -> bool {
					if (json.is_string()) {
						std::string text = json.get<std::string>();
						object = std::string_view(text);
					}
					return true;
				};
				deserializers[static_cast<size_t>(ItemSerializer::Type::WSTRING_VIEW_VALUE)] = [](const SerializedObject& object, const nlohmann::json& json, OS::Logger*) -> bool {
					if (json.is_array()) {
						std::wstring text;
						for (size_t i = 0; i < json.size(); i++) {
							wchar_t symbol;
							if (DeserializeNumber(json[i], symbol))
								text += symbol;
						}
						object = std::wstring_view(text);
					}
					else if (json.is_string()) {
						std::wstring text = Convert<std::wstring>(json.get<std::string>());
						object = std::wstring_view(text);
					}
					else {
						wchar_t symbols[2] = { 0, 0 };
						if (DeserializeNumber(json, symbols[0]))
							object = std::wstring_view(symbols, 1);
					}
					return true;
				};

				return deserializers;
			}();
			const ItemSerializer* serializer = object.Serializer();
			if (serializer == nullptr) {
				if (logger != nullptr)
					logger->Error("DeserializeFromJson - Null serializer provided!");
				return false;
			}
			ItemSerializer::Type type = serializer->GetType();
			if (type < ItemSerializer::Type::OBJECT_PTR_VALUE)
				return DESERIALIZERS[static_cast<size_t>(type)](object, json, logger);
			else if (type == ItemSerializer::Type::OBJECT_PTR_VALUE)
				return deserializerObjectPtr(object, json);
			else if (type == ItemSerializer::Type::SERIALIZER_LIST) {
				if (!json.is_object()) {
					// Leave default values; no warnings required...
					return true;
				}
				bool success = true;
				std::unordered_map<std::string, size_t> fieldNameCounts;
				object.GetFields([&](const SerializedObject& field) {
					if (field.Serializer() == nullptr) {
						if (logger != nullptr)
							logger->Warning("DeserializeFromJson - Got a field with null-serializer!");
						return;
					}
					const std::string& baseName = field.Serializer()->TargetName();
					const std::string name = [&]() -> std::string {
						size_t index;
						std::unordered_map<std::string, size_t>::iterator it = fieldNameCounts.find(baseName);
						if (it == fieldNameCounts.end()) {
							index = 0;
							fieldNameCounts[baseName] = index;
						}
						else {
							it->second++;
							index = it->second;
						}
						std::stringstream stream;
						stream << baseName << "[" << index << "]";
						return stream.str();
					}();
					nlohmann::json::const_iterator it = json.find(name);
					if (it == json.end())
						it = json.find(baseName);
					if (it == json.end()) {
						// Leave default values; no warnings required...
						return;
					}
					if (!DeserializeFromJson(field, *it, logger, deserializerObjectPtr))
						success = false;
					});
				return success;
			}
			else {
				if (logger != nullptr)
					logger->Error("DeserializeFromJson - Serializer type out of bounds!", static_cast<size_t>(object.Serializer()->GetType()), "!");
				return false;
			}
		}
	}
}
