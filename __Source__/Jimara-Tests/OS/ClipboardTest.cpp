#include "../GtestHeaders.h"
#include "../CountingLogger.h"
#include "OS/IO/Clipboard.h"
#include <cstring>


namespace Jimara {
	namespace OS {
		// Tests clipboard texts
		TEST(ClipboardTest, Texts) {
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			EXPECT_TRUE(Clipboard::Clear(logger));
			{
				const std::string_view value = "Jimara::Tests::ClipboardTest::Texts_TextA";
				EXPECT_TRUE(Clipboard::SetText(value, logger));
				std::optional<std::string> result = Clipboard::GetText(logger);
				EXPECT_TRUE(result.has_value());
				if (result.has_value())
					EXPECT_EQ(result.value(), value);
			}
			{
				const std::string_view value = "Jimara::Tests::ClipboardTest::Texts_TextB";
				EXPECT_TRUE(Clipboard::SetText(value, logger));
				for (size_t i = 0; i < 4; i++) {
					std::optional<std::string> result = Clipboard::GetText(logger);
					EXPECT_TRUE(result.has_value());
					if (result.has_value())
						EXPECT_EQ(result.value(), value);
				}
			}
			{
				EXPECT_TRUE(Clipboard::Clear(logger));
				std::optional<std::string> result = Clipboard::GetText(logger);
				EXPECT_FALSE(result.has_value());
			}
			EXPECT_TRUE(logger->NumUnsafe() == 0); 
		}

		TEST(ClipboardTest, Data) {
			Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			EXPECT_TRUE(Clipboard::Clear(logger));
			auto comapre = [](const MemoryBlock& block, const auto& value) {
				EXPECT_NE(block.Data(), nullptr);
				EXPECT_EQ(block.Size(), sizeof(value));
				if (block.Size() >= sizeof(value))
					EXPECT_EQ(std::memcmp(block.Data(), &value, sizeof(value)), 0);
			};
			{
				logger->Info("Testing integer..");
				const std::string_view format = "com.JimaraTest.Integer";
				{
					MemoryBlock data = Clipboard::GetData(format, logger);
					EXPECT_EQ(data.Data(), nullptr);
					EXPECT_EQ(data.Size(), 0);
				}
				int value = 77773;
				EXPECT_TRUE(Clipboard::SetData(format, MemoryBlock(&value, sizeof(value), nullptr), logger));
				for (size_t i = 0; i < 4; i++)
					comapre(Clipboard::GetData(format, logger), value);
			}
			{
				logger->Info("Testing double..");
				const std::string_view format = "com.JimaraTest.Double";
				{
					MemoryBlock data = Clipboard::GetData(format, logger);
					EXPECT_EQ(data.Data(), nullptr);
					EXPECT_EQ(data.Size(), 0);
				}
				double valueA = 14.0411235;
				EXPECT_TRUE(Clipboard::SetData(format, MemoryBlock(&valueA, sizeof(valueA), nullptr), logger));
				for (size_t i = 0; i < 4; i++)
					comapre(Clipboard::GetData(format, logger), valueA);

				double valueB = 64.021223;
				EXPECT_TRUE(Clipboard::SetData(format, MemoryBlock(&valueB, sizeof(valueB), nullptr), logger));
				for (size_t i = 0; i < 4; i++)
					comapre(Clipboard::GetData(format, logger), valueB);
			}
			{
				logger->Info("Testing integer array..");
				const std::string_view format = "com.JimaraTest.IntegerArray";
				{
					MemoryBlock data = Clipboard::GetData(format, logger);
					EXPECT_EQ(data.Data(), nullptr);
					EXPECT_EQ(data.Size(), 0);
				}
				const int values[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
				EXPECT_TRUE(Clipboard::SetData(format, MemoryBlock(values, sizeof(values), nullptr), logger));
				for (size_t i = 0; i < 4; i++) {
					MemoryBlock result = Clipboard::GetData(format, logger);
					EXPECT_NE(result.Data(), nullptr);
					EXPECT_EQ(result.Size(), sizeof(values));
					if (result.Size() >= sizeof(values))
						EXPECT_EQ(std::memcmp(result.Data(), values, sizeof(values)), 0);
				}
			}
			{
				logger->Info("Testing float char and string (simultanously)..");
				const std::string_view formatA = "com.JimaraTest.Float";
				const std::string_view formatB = "com.JimaraTest.Char";
				float valueA = 92.02131f;
				const std::string_view valueS = "Some string value set between float and char...";
				char valueB = 'B';
				EXPECT_TRUE(Clipboard::SetData(formatA, MemoryBlock(&valueA, sizeof(valueA), nullptr), logger));
				EXPECT_TRUE(Clipboard::SetText(valueS, logger));
				EXPECT_TRUE(Clipboard::SetData(formatB, MemoryBlock(&valueB, sizeof(valueB), nullptr), logger));
				comapre(Clipboard::GetData(formatA, logger), valueA);
				comapre(Clipboard::GetData(formatB, logger), valueB);
				{
					std::optional<std::string> result = Clipboard::GetText(logger);
					EXPECT_TRUE(result.has_value());
					if (result.has_value())
						EXPECT_EQ(result.value(), valueS);
				}

				EXPECT_TRUE(Clipboard::Clear());
				EXPECT_EQ(Clipboard::GetData(formatA, logger).Data(), nullptr);
				EXPECT_EQ(Clipboard::GetData(formatB, logger).Data(), nullptr);
				EXPECT_FALSE(Clipboard::GetText().has_value());

				EXPECT_TRUE(Clipboard::SetData(formatA, MemoryBlock(&valueA, sizeof(valueA), nullptr), logger));
				comapre(Clipboard::GetData(formatA, logger), valueA);
			}
			EXPECT_TRUE(logger->NumUnsafe() == 0);
		}
	}
}
