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
				private:
					inline Serializer() : ItemSerializer("SimpleStruct::Serializer", "Hint??? Nah...") {}

				public:
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
			const Function<nlohmann::json, const SerializedObject&, bool&> ignoreObject([](const SerializedObject&, bool&) { return nlohmann::json(); });
			{
				SimpleStruct object;
				bool error = false;
				nlohmann::json json = SerializeToJson(SimpleStruct::Serializer::Instance()->Serialize(object), nullptr, error, ignoreObject);
			}
			Jimara::Test::Memory::MemorySnapshot snapshot;
			{
				Jimara::Test::CountingLogger logger;
				auto testSingleValue = [&](auto value, const char* name) {
					decltype(value) initialValue = value;
					const Reference<const ItemSerializer::Of<decltype(value)>> serializer = ValueSerializer<decltype(value)>::Create(name);
					bool error = false;
					nlohmann::json json = SerializeToJson(serializer->Serialize(value), &logger, error, ignoreObject);
					logger.Info(name, ": ", json);
					return !error;
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

				EXPECT_TRUE(testSingleValue(std::string_view("text"), "std::string_view"));
				EXPECT_TRUE(testSingleValue(std::wstring_view(L"ტექსტი"), "std::wstring_view"));

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
					nlohmann::json json = SerializeToJson(SimpleStruct::Serializer::Instance()->Serialize(object), &logger, error, ignoreObject);
					logger.Info("SimpleStruct: ", json.dump(1, '\t'));
				}

				EXPECT_TRUE(logger.Numfailures() == 0);
			}
			EXPECT_TRUE(snapshot.Compare());
			ASSERT_TRUE(false);
		}
	}
}
