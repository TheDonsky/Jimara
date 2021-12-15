#include "../GtestHeaders.h"
#include "../Memory.h"
#include "../CountingLogger.h"
#include "Data/Serialization/Helpers/SerializeToJson.h"
#include <tuple>


namespace Jimara {
	namespace Serialization {
		namespace {
			struct SimpleStruct {
				int integer = 0;
				char symbol = '\0';
				std::string text = "";
				Vector3 vector0 = Vector3(0.0f);
				Vector3 vector1 = Vector3(0.0f);
				Matrix3 matrix0 = Matrix3(0.0f);
				Matrix4 matrix1 = Matrix4(0.0f);

				inline SimpleStruct() {}

				inline SimpleStruct(
					int i, char c, const std::string_view& s,
					const Vector3& v0, const Vector3& v1,
					const Matrix3& m0, const Matrix4& m1)
					: integer(i), symbol(c), text(s), vector0(v0), vector1(v1), matrix0(m0), matrix1(m1) {}

				inline bool operator==(const SimpleStruct& other)const {
					return
						(integer == other.integer) &&
						(symbol == other.symbol) &&
						(text == other.text) &&
						(vector0 == other.vector0) &&
						(vector1 == other.vector1) &&
						(matrix0 == other.matrix0) &&
						(matrix1 == other.matrix1);
				}

				class Serializer : public virtual SerializerList::From<SimpleStruct> {
				public:
					inline Serializer(const std::string_view& name = "SimpleStruct::Serializer", const std::string_view& hint = "")
						: ItemSerializer(name, hint) {}

					inline virtual void GetFields(const Callback<SerializedObject>& report, SimpleStruct* target)const final override {
						static const Reference<const ItemSerializer::Of<int>> integerSerializer = IntSerializer::Create("integer");
						report(integerSerializer->Serialize(target->integer));

						static const Reference<const ItemSerializer::Of<char>> symbolSerializer = CharSerializer::Create("symbol");
						report(symbolSerializer->Serialize(target->symbol));

						static const Reference<const ItemSerializer::Of<SimpleStruct>> textSerializer = StringViewSerializer::For<SimpleStruct>(
							"text", "Text hint",
							[](SimpleStruct* tg) -> std::string_view { return tg->text; },
							[](const std::string_view& text, SimpleStruct* tg) { tg->text = text; });
						report(textSerializer->Serialize(target));

						static const Reference<const ItemSerializer::Of<Vector3>> vectorSerializer = Vector3Serializer::Create("vector");
						report(vectorSerializer->Serialize(target->vector0));
						report(vectorSerializer->Serialize(target->vector1));

						static const Reference<const ItemSerializer::Of<Matrix3>> matrix0Serializer = Matrix3Serializer::Create("matrix");
						report(matrix0Serializer->Serialize(target->matrix0));

						static const Reference<const ItemSerializer::Of<Matrix4>> matrix1Serializer = Matrix4Serializer::Create("matrix");
						report(matrix1Serializer->Serialize(target->matrix1));
					}

					inline static Serializer* Instance() {
						static Serializer instance;
						return &instance;
					}
				};
			};
		}

		TEST(SerializeToJsonTest, BasicTypes) {
			const Function<nlohmann::json, const SerializedObject&, bool&> ignoreObjectSerialization([](const SerializedObject&, bool&) { return nlohmann::json(); });
			const Function<bool, const SerializedObject&, const nlohmann::json&> ignoreObjectDeserialization([](const SerializedObject&, const nlohmann::json&) { return true; });
			{
				SimpleStruct object;
				bool error = false;
				nlohmann::json json = SerializeToJson(SimpleStruct::Serializer::Instance()->Serialize(object), nullptr, error, ignoreObjectSerialization);
			}
			Jimara::Test::Memory::MemorySnapshot snapshot;
			{
				Jimara::Test::CountingLogger logger;
				auto testSingleValue = [&](auto value, const char* name) {
					const Reference<const ItemSerializer::Of<decltype(value)>> serializer = ValueSerializer<decltype(value)>::Create(name);
					bool error = false;
					nlohmann::json json = SerializeToJson(serializer->Serialize(value), &logger, error, ignoreObjectSerialization);
					logger.Info(name, ": ", json);
					if (error) return false;

					decltype(value) deserialized = {};
					if (!DeserializeFromJson(serializer->Serialize(deserialized), json, &logger, ignoreObjectDeserialization))
						logger.Error("Failed to desererialize from json!");
					else if (value != deserialized)
						logger.Error("Value mismatch!");
					else return true;
					return false;
				};
				EXPECT_TRUE(testSingleValue((bool)true, "Boolean"));
				EXPECT_TRUE(testSingleValue((bool)false, "Boolean"));
				
				EXPECT_TRUE(testSingleValue((char)'a', "Char"));
				EXPECT_TRUE(testSingleValue((signed char)'b', "Signed Char"));
				EXPECT_TRUE(testSingleValue((unsigned char)'c', "Unsigned Char"));
				EXPECT_TRUE(testSingleValue((wchar_t)L'ჭ', "Wide Char"));

				EXPECT_TRUE(testSingleValue((short)-1223, "Short"));
				EXPECT_TRUE(testSingleValue((short)321, "Short"));
				EXPECT_TRUE(testSingleValue((unsigned short)3245, "Unsigned Short"));

				EXPECT_TRUE(testSingleValue((int)3232342, "Int"));
				EXPECT_TRUE(testSingleValue((int)-32334, "Int"));
				EXPECT_TRUE(testSingleValue((unsigned int)973421, "Unsigned Int"));

				EXPECT_TRUE(testSingleValue((long)24678, "Long"));
				EXPECT_TRUE(testSingleValue((long)-78564, "Long"));
				EXPECT_TRUE(testSingleValue((unsigned long)9492, "Unsigned Long"));

				EXPECT_TRUE(testSingleValue((long long)675543, "Long Long"));
				EXPECT_TRUE(testSingleValue((long long)-8752213, "Long Long"));
				EXPECT_TRUE(testSingleValue((unsigned long long)76863121, "Unsigned Long Long"));

				EXPECT_TRUE(testSingleValue((float)94343.342543f, "Float"));
				EXPECT_TRUE(testSingleValue((double)-4535675632.99324236, "Double"));

				EXPECT_TRUE(testSingleValue(Vector2(2.0f, 5.2f), "Vector2"));
				EXPECT_TRUE(testSingleValue(Vector3(1.0f, -3.2f, 8.2), "Vector3"));
				EXPECT_TRUE(testSingleValue(Vector4(-2.2f, 1.2f, 9.8, -89.12), "Vector4"));

				EXPECT_TRUE(testSingleValue(Matrix2(
					Vector2(0.0f, 0.1f), 
					Vector2(1.0f, 1.1f)), "Matrix2"));
				EXPECT_TRUE(testSingleValue(Matrix3(
					Vector3(0.0f, 0.1f, 0.2f), 
					Vector3(1.0f, 1.1f, 1.2f), 
					Vector3(2.0f, 2.1f, 2.2f)), "Matrix3"));
				EXPECT_TRUE(testSingleValue(Matrix4(
					Vector4(0.0f, 0.1f, 0.2f, 0.3f), 
					Vector4(1.0f, 1.1f, 1.2f, 1.3f), 
					Vector4(2.0f, 2.1f, 2.2f, 2.3f), 
					Vector4(3.0f, 3.1f, 3.2f, 3.3f)), "Matrix4"));

				{
					std::string text("text");
					bool error = false;
					const Reference<const ItemSerializer::Of<std::string>> serializer = ValueSerializer<std::string_view>::For<std::string>(
						"Text", "Hint",
						[](std::string* text) -> std::string_view { return *text; },
						[](const std::string_view& view, std::string* text) { *text = view; });
					nlohmann::json json = SerializeToJson(serializer->Serialize(text), &logger, error, ignoreObjectSerialization);
					logger.Info("std::string: ", json);
					EXPECT_FALSE(error);
					std::string copy;
					EXPECT_TRUE(DeserializeFromJson(serializer->Serialize(copy), json, &logger, ignoreObjectDeserialization));
					EXPECT_EQ(text, copy);
				}
				{
					std::wstring text(L"ტექსტი");
					bool error = false;
					const Reference<const ItemSerializer::Of<std::wstring>> serializer = ValueSerializer<std::wstring_view>::For<std::wstring>(
						"Text", "Hint",
						[](std::wstring* text) -> std::wstring_view { return *text; },
						[](const std::wstring_view& view, std::wstring* text) { *text = view; });
					nlohmann::json json = SerializeToJson(serializer->Serialize(text), &logger, error, ignoreObjectSerialization);
					logger.Info("std::wstring: ", json); 
					EXPECT_FALSE(error);
					std::wstring copy;
					EXPECT_TRUE(DeserializeFromJson(serializer->Serialize(copy), json, &logger, ignoreObjectDeserialization));
					EXPECT_TRUE(text == copy);
				}

				{
					SimpleStruct object(8, 'w', "Bla",
						Vector3(0.0f, 0.4f, 0.8f), Vector3(1.0f, 1.4f, 1.8f),
						Matrix3(
							Vector3(0.0f, 0.1f, 0.2f),
							Vector3(1.0f, 1.1f, 1.2f),
							Vector3(2.0f, 2.1f, 2.2f)),
						Matrix4(
							Vector4(0.0f, 0.1f, 0.2f, 0.3f),
							Vector4(1.0f, 1.1f, 1.2f, 1.3f),
							Vector4(2.0f, 2.1f, 2.2f, 2.3f),
							Vector4(3.0f, 3.1f, 3.2f, 3.3f)));
					bool error = false;
					nlohmann::json json = SerializeToJson(SimpleStruct::Serializer::Instance()->Serialize(object), &logger, error, ignoreObjectSerialization);
					logger.Info("SimpleStruct: ", json.dump(1, '\t'));
					EXPECT_FALSE(error);
					SimpleStruct copy;
					EXPECT_TRUE(DeserializeFromJson(SimpleStruct::Serializer::Instance()->Serialize(copy), json, &logger, ignoreObjectDeserialization));
					EXPECT_TRUE(object == copy);
				}

				EXPECT_TRUE(logger.Numfailures() == 0);
			}
			EXPECT_TRUE(snapshot.Compare());
		}



		namespace {
			struct CompoundStruct {
				SimpleStruct simpleA;
				SimpleStruct simpleB;
				int num = 0;
				OS::Logger* logger = nullptr;

				inline bool operator==(const CompoundStruct& other)const {
					return
						(simpleA == other.simpleA) &&
						(simpleB == other.simpleB) &&
						(num == other.num) &&
						(logger == other.logger);
				}

				class Serializer : public virtual SerializerList::From<CompoundStruct> {
				private:
					inline Serializer() : ItemSerializer("CompoundStruct::Serializer", "Hint??? Nah...") {}

				public:
					inline virtual void GetFields(const Callback<SerializedObject>& report, CompoundStruct* target)const final override {
						static const Reference<const ItemSerializer::Of<SimpleStruct>> simpleASerializer = Object::Instantiate<SimpleStruct::Serializer>("simpleA");
						report(simpleASerializer->Serialize(&target->simpleA));
						
						static const Reference<const ItemSerializer::Of<SimpleStruct>> simpleBSerializer = Object::Instantiate<SimpleStruct::Serializer>("simpleB");
						report(simpleBSerializer->Serialize(&target->simpleB));

						static const Reference<const ItemSerializer::Of<int>> integerSerializer = IntSerializer::Create("num");
						report(integerSerializer->Serialize(target->num));

						static const Reference<const ItemSerializer::Of<OS::Logger*>> loggerReferenceSerializer = ValueSerializer<OS::Logger*>::Create("logger");
						report(loggerReferenceSerializer->Serialize(&target->logger));
					}

					inline static Serializer* Instance() {
						static Serializer instance;
						return &instance;
					}
				};
			};
		}

		TEST(SerializeToJsonTest, CompundType) {
			size_t numObjectSerializeRequests = 0;
			nlohmann::json(*countObjectSerializeRequests)(size_t*, const SerializedObject&, bool&) = [](size_t* count, const SerializedObject&, bool&) {
				(*count)++;
				return nlohmann::json("<SOME OBJECT VALUE>");
			};
			bool(*countObjectDeserializeRequests)(size_t*, const SerializedObject&, const nlohmann::json&) = [](size_t* count, const SerializedObject&, const nlohmann::json&) {
				(*count)++;
				return true;
			};
			const Function<nlohmann::json, const SerializedObject&, bool&> countObjects(countObjectSerializeRequests, &numObjectSerializeRequests);
			const Function<bool, const SerializedObject&, const nlohmann::json&> countDeserializedReferences(countObjectDeserializeRequests, &numObjectSerializeRequests);
			{
				CompoundStruct object;
				bool error = false;
				nlohmann::json json = SerializeToJson(CompoundStruct::Serializer::Instance()->Serialize(object), nullptr, error, countObjects);
				EXPECT_FALSE(error);
				EXPECT_EQ(numObjectSerializeRequests, 1);
			}
			Jimara::Test::Memory::MemorySnapshot snapshot;
			{
				Jimara::Test::CountingLogger logger;
				CompoundStruct object;
				object.simpleA = SimpleStruct(8, 'w', "Bla",
					Vector3(0.0f, 0.4f, 0.8f), Vector3(1.0f, 1.4f, 1.8f),
					Matrix3(
						Vector3(0.0f, 0.1f, 0.2f),
						Vector3(1.0f, 1.1f, 1.2f),
						Vector3(2.0f, 2.1f, 2.2f)),
					Matrix4(
						Vector4(0.0f, 0.1f, 0.2f, 0.3f),
						Vector4(1.0f, 1.1f, 1.2f, 1.3f),
						Vector4(2.0f, 2.1f, 2.2f, 2.3f),
						Vector4(3.0f, 3.1f, 3.2f, 3.3f)));
				object.num = 9;
				bool error = false;
				nlohmann::json json = SerializeToJson(CompoundStruct::Serializer::Instance()->Serialize(object), &logger, error, countObjects);
				logger.Info("SimpleStruct: ", json.dump(1, '\t'));
				EXPECT_FALSE(error);
				EXPECT_EQ(numObjectSerializeRequests, 2);
				CompoundStruct copy;
				EXPECT_TRUE(DeserializeFromJson(CompoundStruct::Serializer::Instance()->Serialize(copy), json, &logger, countDeserializedReferences));
				EXPECT_EQ(numObjectSerializeRequests, 3);
				EXPECT_TRUE(copy == object);
				EXPECT_TRUE(logger.Numfailures() == 0);
			}
			EXPECT_TRUE(snapshot.Compare());
		}
	}
}
