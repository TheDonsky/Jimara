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
		size_t Texture::TexelSize(PixelFormat format) {
			static const size_t* SIZES = [&]() -> const size_t* {
				static size_t sizes[static_cast<size_t>(PixelFormat::FORMAT_COUNT)];
				for (size_t i = 0u; i < static_cast<size_t>(PixelFormat::FORMAT_COUNT); i++)
					sizes[i] = 0u;
				sizes[static_cast<size_t>(PixelFormat::R8_SRGB)] = sizeof(uint8_t);
				sizes[static_cast<size_t>(PixelFormat::R8_UNORM)] = sizeof(uint8_t);

				sizes[static_cast<size_t>(PixelFormat::R8G8_SRGB)] = sizeof(uint8_t) * 2u;
				sizes[static_cast<size_t>(PixelFormat::R8G8_UNORM)] = sizeof(uint8_t) * 2u;
				
				sizes[static_cast<size_t>(PixelFormat::R8G8B8_SRGB)] = sizeof(uint8_t) * 3u;
				sizes[static_cast<size_t>(PixelFormat::R8G8B8_UNORM)] = sizeof(uint8_t) * 3u;
				sizes[static_cast<size_t>(PixelFormat::B8G8R8_SRGB)] = sizeof(uint8_t) * 3u;
				sizes[static_cast<size_t>(PixelFormat::B8G8R8_UNORM)] = sizeof(uint8_t) * 3u;

				sizes[static_cast<size_t>(PixelFormat::R8G8B8A8_SRGB)] = sizeof(uint8_t) * 4u;
				sizes[static_cast<size_t>(PixelFormat::R8G8B8A8_UNORM)] = sizeof(uint8_t) * 4u;
				sizes[static_cast<size_t>(PixelFormat::B8G8R8A8_SRGB)] = sizeof(uint8_t) * 4u;
				sizes[static_cast<size_t>(PixelFormat::B8G8R8A8_UNORM)] = sizeof(uint8_t) * 4u;

				sizes[static_cast<size_t>(PixelFormat::R16_UINT)] = sizeof(uint16_t);
				sizes[static_cast<size_t>(PixelFormat::R16_SINT)] = sizeof(int16_t);
				sizes[static_cast<size_t>(PixelFormat::R16_UNORM)] = sizeof(uint16_t);
				sizes[static_cast<size_t>(PixelFormat::R16_SFLOAT)] = sizeof(uint16_t);

				sizes[static_cast<size_t>(PixelFormat::R16G16_UINT)] = sizeof(uint16_t) * 2u;
				sizes[static_cast<size_t>(PixelFormat::R16G16_SINT)] = sizeof(int16_t) * 2u;
				sizes[static_cast<size_t>(PixelFormat::R16G16_UNORM)] = sizeof(uint16_t) * 2u;
				sizes[static_cast<size_t>(PixelFormat::R16G16_SFLOAT)] = sizeof(uint16_t) * 2u;

				sizes[static_cast<size_t>(PixelFormat::R16G16B16_UINT)] = sizeof(uint16_t) * 3u;
				sizes[static_cast<size_t>(PixelFormat::R16G16B16_SINT)] = sizeof(int16_t) * 3u;
				sizes[static_cast<size_t>(PixelFormat::R16G16B16_UNORM)] = sizeof(uint16_t) * 3u;
				sizes[static_cast<size_t>(PixelFormat::R16G16B16_SFLOAT)] = sizeof(uint16_t) * 3u;

				sizes[static_cast<size_t>(PixelFormat::R16G16B16A16_UINT)] = sizeof(uint16_t) * 4u;
				sizes[static_cast<size_t>(PixelFormat::R16G16B16A16_SINT)] = sizeof(int16_t) * 4u;
				sizes[static_cast<size_t>(PixelFormat::R16G16B16A16_UNORM)] = sizeof(uint16_t) * 4u;
				sizes[static_cast<size_t>(PixelFormat::R16G16B16A16_SFLOAT)] = sizeof(uint16_t) * 4u;

				sizes[static_cast<size_t>(PixelFormat::R32_UINT)] = sizeof(uint32_t);
				sizes[static_cast<size_t>(PixelFormat::R32_SINT)] = sizeof(int32_t);
				sizes[static_cast<size_t>(PixelFormat::R32_SFLOAT)] = sizeof(float); static_assert(sizeof(float) == 4u);

				sizes[static_cast<size_t>(PixelFormat::R32G32_UINT)] = sizeof(uint32_t) * 2u;
				sizes[static_cast<size_t>(PixelFormat::R32G32_SINT)] = sizeof(int32_t) * 2u;
				sizes[static_cast<size_t>(PixelFormat::R32G32_SFLOAT)] = sizeof(float) * 2u;

				sizes[static_cast<size_t>(PixelFormat::R32G32B32_UINT)] = sizeof(uint32_t) * 3u;
				sizes[static_cast<size_t>(PixelFormat::R32G32B32_SINT)] = sizeof(int32_t) * 3u;
				sizes[static_cast<size_t>(PixelFormat::R32G32B32_SFLOAT)] = sizeof(float) * 3u;

				sizes[static_cast<size_t>(PixelFormat::R32G32B32A32_UINT)] = sizeof(uint32_t) * 4u;
				sizes[static_cast<size_t>(PixelFormat::R32G32B32A32_SINT)] = sizeof(int32_t) * 4u;
				sizes[static_cast<size_t>(PixelFormat::R32G32B32A32_SFLOAT)] = sizeof(float) * 4u;

				sizes[static_cast<size_t>(PixelFormat::D32_SFLOAT)] = sizeof(float);
				sizes[static_cast<size_t>(PixelFormat::D32_SFLOAT_S8_UINT)] = sizeof(float) + sizeof(uint8_t);
				sizes[static_cast<size_t>(PixelFormat::D24_UNORM_S8_UINT)] = sizeof(uint32_t);
				return sizes;
			}();
			return (format < PixelFormat::FORMAT_COUNT) ? SIZES[static_cast<size_t>(format)] : size_t(0u);
		}

		Texture::ColorSpace Texture::FormatColorSpace(PixelFormat format) {
			static const ColorSpace* SPACES = [&]() -> const ColorSpace* {
				static ColorSpace spaces[static_cast<size_t>(PixelFormat::FORMAT_COUNT)];
				for (size_t i = 0u; i < static_cast<size_t>(PixelFormat::FORMAT_COUNT); i++)
					spaces[i] = ColorSpace::OTHER;
				spaces[static_cast<size_t>(PixelFormat::R8_SRGB)] = ColorSpace::SRGB;
				spaces[static_cast<size_t>(PixelFormat::R8_UNORM)] = ColorSpace::LINEAR;

				spaces[static_cast<size_t>(PixelFormat::R8G8_SRGB)] = ColorSpace::SRGB;
				spaces[static_cast<size_t>(PixelFormat::R8G8_UNORM)] = ColorSpace::LINEAR;

				spaces[static_cast<size_t>(PixelFormat::R8G8B8_SRGB)] = ColorSpace::SRGB;
				spaces[static_cast<size_t>(PixelFormat::R8G8B8_UNORM)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::B8G8R8_SRGB)] = ColorSpace::SRGB;
				spaces[static_cast<size_t>(PixelFormat::B8G8R8_UNORM)] = ColorSpace::LINEAR;

				spaces[static_cast<size_t>(PixelFormat::R8G8B8A8_SRGB)] = ColorSpace::SRGB;
				spaces[static_cast<size_t>(PixelFormat::R8G8B8A8_UNORM)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::B8G8R8A8_SRGB)] = ColorSpace::SRGB;
				spaces[static_cast<size_t>(PixelFormat::B8G8R8A8_UNORM)] = ColorSpace::LINEAR;

				spaces[static_cast<size_t>(PixelFormat::R16_UINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R16_SINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R16_UNORM)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R16_SFLOAT)] = ColorSpace::LINEAR;

				spaces[static_cast<size_t>(PixelFormat::R16G16_UINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R16G16_SINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R16G16_UNORM)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R16G16_SFLOAT)] = ColorSpace::LINEAR;

				spaces[static_cast<size_t>(PixelFormat::R16G16B16_UINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R16G16B16_SINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R16G16B16_UNORM)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R16G16B16_SFLOAT)] = ColorSpace::LINEAR;

				spaces[static_cast<size_t>(PixelFormat::R16G16B16A16_UINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R16G16B16A16_SINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R16G16B16A16_UNORM)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R16G16B16A16_SFLOAT)] = ColorSpace::LINEAR;

				spaces[static_cast<size_t>(PixelFormat::R32_UINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R32_SINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R32_SFLOAT)] = ColorSpace::LINEAR;

				spaces[static_cast<size_t>(PixelFormat::R32G32_UINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R32G32_SINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R32G32_SFLOAT)] = ColorSpace::LINEAR;

				spaces[static_cast<size_t>(PixelFormat::R32G32B32_UINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R32G32B32_SINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R32G32B32_SFLOAT)] = ColorSpace::LINEAR;

				spaces[static_cast<size_t>(PixelFormat::R32G32B32A32_UINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R32G32B32A32_SINT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::R32G32B32A32_SFLOAT)] = ColorSpace::LINEAR;

				spaces[static_cast<size_t>(PixelFormat::D32_SFLOAT)] = ColorSpace::LINEAR;
				spaces[static_cast<size_t>(PixelFormat::D32_SFLOAT_S8_UINT)] = ColorSpace::OTHER;
				spaces[static_cast<size_t>(PixelFormat::D24_UNORM_S8_UINT)] = ColorSpace::OTHER;
				return spaces;
				}();
				return (format < PixelFormat::FORMAT_COUNT) ? SPACES[static_cast<size_t>(format)] : ColorSpace::OTHER;
		}

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
					, Size3(static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1), 1, createMipmaps, Graphics::ImageTexture::AccessFlags::NONE);

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
					, Size3(static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1), 1, createMipmaps, Graphics::ImageTexture::AccessFlags::NONE);

				uint32_t* dataPtr = static_cast<uint32_t*>(texture->Map());
				uint32_t* const dataEnd = dataPtr + static_cast<size_t>(texWidth) * static_cast<size_t>(texHeight) * 2u;
				float* valuePtr = pixels;
				while (dataPtr < dataEnd) {
					auto clamped = [](float value) {
						const constexpr float fp16_max = 65504.0f;
						const constexpr float fp16_min = -fp16_max;
						return Math::Min(Math::Max(fp16_min, value), fp16_max);
					};
					(*dataPtr) = glm::packHalf2x16(Vector2(clamped(*valuePtr), clamped(valuePtr[1])));
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
