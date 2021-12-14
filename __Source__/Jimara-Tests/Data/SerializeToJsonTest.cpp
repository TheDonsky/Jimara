#include "../GtestHeaders.h"
#include "../CountingLogger.h"
#include "Data/Serialization/Helpers/SerializeToJson.h"


namespace Jimara {
	namespace Serialization {
		TEST(SerializeToJsonTest, BasicTypes) {
			{
				Jimara::Test::CountingLogger logger;
				const Function<nlohmann::json, const SerializedObject&, bool&> ignoreObject([](const SerializedObject&, bool&) { return nlohmann::json(); });
				auto testSingleValue = [&](auto value, const char* name) {
					const Reference<const ItemSerializer::Of<decltype(value)>> serializer = ValueSerializer<decltype(value)>::Create("Value");
					bool error = false;
					nlohmann::json json = SerializeToJson(serializer->Serialize(value), &logger, error, ignoreObject);
					logger.Info(name, ": ", json);
					return !error;
				};
				EXPECT_TRUE(testSingleValue((bool)true, "True Boolean"));
				EXPECT_TRUE(testSingleValue((bool)false, "False Boolean"));
				EXPECT_TRUE(testSingleValue((char)'a', "Char 'a'"));
				EXPECT_TRUE(testSingleValue((signed char)'b', "Signed Char 'b'"));
				EXPECT_TRUE(testSingleValue((unsigned char)'c', "Unsigned Char 'c'"));
				EXPECT_TRUE(testSingleValue((wchar_t)L'ჭ', "Wide Char"));
				EXPECT_TRUE(testSingleValue((short)-1223, "Short -1223"));
				EXPECT_TRUE(testSingleValue((short)321, "Short 321"));
				EXPECT_TRUE(testSingleValue((unsigned short)3245, "Unsigned hort 3245"));
			}
			ASSERT_TRUE(false);
		}
	}
}
