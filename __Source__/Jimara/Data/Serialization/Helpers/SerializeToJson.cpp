#include "SerializeToJson.h"
#include <sstream>


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
				nlohmann::json json;
				object.GetFields([&](const SerializedObject& field) {
					if (field.Serializer() == nullptr) {
						if (logger != nullptr)
							logger->Warning("SerializeToJson - Got a field with null-serializer!");
						return;
					}
					auto name = [&]() -> std::string {
						const std::string& baseName = field.Serializer()->TargetName();
						for (size_t i = 0; i <= json.size(); i++) {
							const std::string nameCandidate = [&]() -> std::string {
								std::stringstream stream;
								stream << baseName << "[" << i << "]";
								return stream.str();
							}();
							if (json.find(nameCandidate) == json.end())
								return nameCandidate;
						}
						if (logger != nullptr)
							logger->Error("SerializeToJson - Internal error: could not generate name key for a field!");
						error = true;
						return "";
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
	}
}
