#include "TransientImage.h"
#include "../../Math/Helpers.h"


namespace Jimara {
	namespace {
		struct TransientImage_Descriptor {
			Reference<Graphics::GraphicsDevice> device;
			Graphics::Texture::TextureType type = Graphics::Texture::TextureType::TEXTURE_2D;
			Graphics::Texture::PixelFormat format = Graphics::Texture::PixelFormat::R8G8B8A8_SRGB;
			Size3 size = Size3(1);
			uint32_t arraySize = 1;
			Graphics::Texture::Multisampling sampleCount = Graphics::Texture::Multisampling::SAMPLE_COUNT_1;

			inline bool operator==(const TransientImage_Descriptor& other)const {
				return
					device == other.device &&
					type == other.type &&
					format == other.format &&
					size == other.size &&
					arraySize == other.arraySize &&
					sampleCount == other.sampleCount;
			}

			inline bool operator<(const TransientImage_Descriptor& other)const {
				if (device < other.device) return true;
				else if (device > other.device) return false;

				if (type < other.type) return true;
				else if (type > other.type) return false;

				if (format < other.format) return true;
				else if (format > other.format) return false;

				if (size.x < other.size.x) return true;
				else if (size.x > other.size.x) return false;

				if (size.y < other.size.y) return true;
				else if (size.y > other.size.y) return false;

				if (size.z < other.size.z) return true;
				else if (size.z > other.size.z) return false;

				if (arraySize < other.arraySize) return true;
				else if (arraySize > other.arraySize) return false;

				if (sampleCount < other.sampleCount) return true;
				else if (sampleCount > other.sampleCount) return false;

				return false;
			}
		};
	}
}

namespace std {
	template<>
	struct hash<Jimara::TransientImage_Descriptor> {
		inline size_t operator()(const Jimara::TransientImage_Descriptor& desc)const {
			return Jimara::MergeHashes(
				Jimara::MergeHashes(
					std::hash<Jimara::Reference<Jimara::Graphics::GraphicsDevice>>()(desc.device),
					Jimara::MergeHashes(
						std::hash<Jimara::Graphics::Texture::TextureType>()(desc.type),
						std::hash<Jimara::Graphics::Texture::PixelFormat>()(desc.format))),
				Jimara::MergeHashes(
					Jimara::MergeHashes(
						Jimara::MergeHashes(
							std::hash<uint32_t>()(desc.size.x),
							std::hash<uint32_t>()(desc.size.y)),
						Jimara::MergeHashes(
							std::hash<uint32_t>()(desc.size.z),
							std::hash<uint32_t>()(desc.arraySize))),
					std::hash<Jimara::Graphics::Texture::Multisampling>()(desc.sampleCount)
				));
		}
	};
}

namespace Jimara {
	struct TransientImage::Helpers {
#pragma warning(disable: 4250)
		class CachedInstance : public virtual TransientImage, public virtual ObjectCache<TransientImage_Descriptor>::StoredObject {
		public:
			inline CachedInstance(Graphics::Texture* texture) : TransientImage(texture) {}
		};
#pragma warning(default: 4250)

		class InstanceCache : public virtual ObjectCache<TransientImage_Descriptor> {
		public:
			inline static Reference<TransientImage> GetInstance(const TransientImage_Descriptor& desc) {
				static InstanceCache cache;
				return cache.GetCachedOrCreate(desc, false, [&]() -> Reference<CachedInstance> {
					Reference<Graphics::Texture> texture = desc.device->CreateMultisampledTexture(desc.type, desc.format, desc.size, desc.arraySize, desc.sampleCount);
					if (texture == nullptr) {
						desc.device->Log()->Error("TransientImage::Helpers::InstanceCache::GetInstance - Failed to create shared texture! [File: ", __FILE__, "; Line: ", __LINE__, "]");
						return nullptr;
					}
					else return Object::Instantiate<CachedInstance>(texture);
					});
			}
		};
	};

	Reference<TransientImage> TransientImage::Get(Graphics::GraphicsDevice* device,
		Graphics::Texture::TextureType type, Graphics::Texture::PixelFormat format,
		Size3 size, uint32_t arraySize, Graphics::Texture::Multisampling sampleCount) {
		if (device == nullptr) return nullptr;
		TransientImage_Descriptor desc = {};
		{
			desc.device = device;
			desc.type = type;
			desc.format = format;
			desc.size = size;
			desc.arraySize = arraySize;
			desc.sampleCount = sampleCount;
		}
		return Helpers::InstanceCache::GetInstance(desc);
	}
}
