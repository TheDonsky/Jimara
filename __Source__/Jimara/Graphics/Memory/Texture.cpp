#include "Texture.h"
#include "../GraphicsDevice.h"
#include "../../OS/IO/MMappedFile.h"

#pragma warning(disable: 26451)
#pragma warning(disable: 26819)
#pragma warning(disable: 6011)
#pragma warning(disable: 6262)
#pragma warning(disable: 6308)
#pragma warning(disable: 28182)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma warning(default: 26451)
#pragma warning(default: 26819)
#pragma warning(default: 6011)
#pragma warning(default: 6262)
#pragma warning(default: 6308)
#pragma warning(default: 28182)

namespace Jimara {
	namespace Graphics {
		Reference<ImageTexture> ImageTexture::LoadFromFile(GraphicsDevice* device, const OS::Path& filename, bool createMipmaps, bool highPrecision) {
			Reference<OS::MMappedFile> memoryMapping = OS::MMappedFile::Create(filename, device->Log());
			if (memoryMapping == nullptr) {
				device->Log()->Error("ImageTexture::LoadFromFile - Failed to open file '", filename, "'!");
				return nullptr;
			}
			Jimara::MemoryBlock memoryBlock(*memoryMapping);
			if (memoryBlock.Size() != static_cast<int>(memoryBlock.Size())) {
				device->Log()->Error("ImageTexture::LoadFromFile - File too large for stbi_load_from_memory ('", filename, "')!");
				return nullptr;
			}

			int texWidth, texHeight, texChannels;
			Reference<ImageTexture> texture;
			if (!highPrecision) {
				stbi_uc* pixels = stbi_load_from_memory(
					reinterpret_cast<const stbi_uc*>(memoryBlock.Data()), static_cast<int>(memoryBlock.Size()),
					&texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
				if (pixels == nullptr) {
					device->Log()->Error("ImageTexture::LoadFromFile - Could not load image from file: '", filename, "'");
					return nullptr;
				}
				assert(sizeof(uint32_t) == 4); // Well... we may be on a strange system if this check fails...

				texture = device->CreateTexture(
					TextureType::TEXTURE_2D, PixelFormat::R8G8B8A8_UNORM
					, Size3(static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1), 1, createMipmaps);

				memcpy(texture->Map(), pixels, 4u * static_cast<size_t>(texWidth) * static_cast<size_t>(texHeight));
				texture->Unmap(true);

				stbi_image_free(pixels);
			}
			else {
				float* pixels = stbi_loadf_from_memory(
					reinterpret_cast<const stbi_uc*>(memoryBlock.Data()), static_cast<int>(memoryBlock.Size()),
					&texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
				if (pixels == nullptr) {
					device->Log()->Error("ImageTexture::LoadFromFile - Could not load image from file: '", filename, "'");
					return nullptr;
				}
				assert(sizeof(float) == 4); // Well... we may be on a strange system if this check fails...

				texture = device->CreateTexture(
					TextureType::TEXTURE_2D, PixelFormat::R16G16B16A16_SFLOAT
					, Size3(static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1), 1, createMipmaps);

				uint32_t* dataPtr = static_cast<uint32_t*>(texture->Map());
				uint32_t* const dataEnd = dataPtr + static_cast<size_t>(texWidth) * static_cast<size_t>(texHeight) * 2u;
				float* valuePtr = pixels;
				while (dataPtr < dataEnd) {
					(*dataPtr) = glm::packHalf2x16(Vector2(*valuePtr, valuePtr[1]));
					valuePtr += 2u;
					dataPtr++;
				}
				texture->Unmap(true);

				stbi_image_free(pixels);
			}
			return texture;
		}
	}
}
