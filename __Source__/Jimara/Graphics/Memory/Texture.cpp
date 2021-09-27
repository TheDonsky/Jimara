#include "Texture.h"
#include "../GraphicsDevice.h"

#pragma warning(disable: 26451)
#pragma warning(disable: 26819)
#pragma warning(disable: 6011)
#pragma warning(disable: 6262)
#pragma warning(disable: 28182)
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#pragma warning(default: 26451)
#pragma warning(default: 26819)
#pragma warning(default: 6011)
#pragma warning(default: 6262)
#pragma warning(default: 28182)

namespace Jimara {
	namespace Graphics {
		Reference<ImageTexture> ImageTexture::LoadFromFile(GraphicsDevice* device, const std::string_view& filename, bool createMipmaps) {
			int texWidth, texHeight, texChannels;
			stbi_uc* pixels = stbi_load(filename.data(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
			if (pixels == nullptr) {
				device->Log()->Error("ImageTexture::LoadFromFile - Could not load image from file: '", filename, "'");
				return nullptr;
			}
			assert(sizeof(uint32_t) == 4); // Well... we may be on a strange system if this check fails...
			
			Reference<ImageTexture> texture = device->CreateTexture(
				TextureType::TEXTURE_2D, PixelFormat::R8G8B8A8_SRGB
				, Size3(static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1), 1, createMipmaps);

			memcpy(texture->Map(), pixels, 4u * static_cast<size_t>(texWidth) * static_cast<size_t>(texHeight));
			texture->Unmap(true);

			stbi_image_free(pixels);
			return texture;
		}
	}
}
