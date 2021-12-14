#include "../GtestHeaders.h"
#include "../Memory.h"
#include "../CountingLogger.h"
#include "Data/Serialization/Helpers/SerializeToJson.h"


namespace Jimara {
	namespace Serialization {
		TEST(SerializeToJsonTest, BasicTypes) {
			Jimara::Test::Memory::MemorySnapshot snapshot;
			{
				Jimara::Test::CountingLogger logger;
				const Function<nlohmann::json, const SerializedObject&, bool&> ignoreObject([](const SerializedObject&, bool&) { return nlohmann::json(); });
				auto testSingleValue = [&](auto value, const char* name) {
					decltype(value) initialValue = value;
					const Reference<const ItemSerializer::Of<decltype(value)>> serializer = ValueSerializer<decltype(value)>::Create(name);
					bool error = false;
					nlohmann::json json = SerializeToJson(serializer->Serialize(value), &logger, error, ignoreObject);
					logger.Info(json);
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

				EXPECT_TRUE(testSingleValue(std::string_view("text"), "std::string_view"));
				EXPECT_TRUE(testSingleValue(std::wstring_view(L"ტექსტი"), "std::wstring_view"));
			}
			EXPECT_TRUE(snapshot.Compare());
			ASSERT_TRUE(false);
		}
	}
}
