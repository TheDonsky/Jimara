#include "../GtestHeaders.h"
#include "OS/IO/MMappedFile.h"
#include "OS/Logging/StreamLogger.h"
#include "Data/Formats/FBX/FBXContent.h"
#include <sstream>

namespace Jimara {
	namespace {
		const char FBX_BINARY_HEADER[] = "Kaydara FBX Binary  ";
		constexpr inline static size_t FbxBinaryHeaderSize() { return sizeof(FBX_BINARY_HEADER) / sizeof(char); }

		const Endian FBX_BINARY_ENDIAN = Endian::LITTLE;
	}

	TEST(FBXTest, Playground) {
		Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
		//Reference<OS::MMappedFile> fileMapping = OS::MMappedFile::Create("C:/Users/Donsky/Desktop/DefaultGuy_0.fbx", logger);
		Reference<OS::MMappedFile> fileMapping = OS::MMappedFile::Create("Assets/Meshes/FBX/Cube.fbx", logger);
		ASSERT_NE(fileMapping, nullptr);
		MemoryBlock block = *fileMapping;

		size_t ptr = FbxBinaryHeaderSize();

		// File-magic:
		ASSERT_GE(block.Size(), ptr);
		ASSERT_EQ(memcmp(FBX_BINARY_HEADER, block.Data(), ptr), 0);
		
		// Unknown ([0x1A, 0x00] expected):
		ASSERT_GE(block.Size(), ptr + (sizeof(uint8_t) * 2));
		const uint8_t header21_22[2] = {
			block.Get<uint8_t>(ptr, NativeEndian()),
			block.Get<uint8_t>(ptr, NativeEndian())
		};
		EXPECT_EQ(header21_22[0], 0x1A);
		EXPECT_EQ(header21_22[1], 0x00);
		
		// Version number:
		ASSERT_GE(block.Size(), ptr + sizeof(uint32_t));
		const uint32_t version = block.Get<uint32_t>(ptr, FBX_BINARY_ENDIAN);



		const char* blockText = (const char*)block.Data();

		Reference<FBXContent> content = FBXContent::Decode(block, logger);
		ASSERT_NE(content, nullptr);
		logger->Info(*content);
	}
}