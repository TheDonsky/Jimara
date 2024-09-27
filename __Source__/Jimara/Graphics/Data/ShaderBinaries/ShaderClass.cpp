#include "ShaderClass.h"
#include "../../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../../Math/Helpers.h"

namespace Jimara {
	namespace Graphics {
		namespace {
			struct ShaderClass_BufferDataIndex {
				Reference<GraphicsDevice> device;
				MemoryBlock data = MemoryBlock(nullptr, 0, nullptr);

				inline bool operator==(const ShaderClass_BufferDataIndex& other)const {
					return device == other.device && data.Size() == other.data.Size() && (std::memcmp(data.Data(), other.data.Data(), data.Size()) == 0);
				}
			};

			struct ShaderClass_TextureIndex {
				Reference<GraphicsDevice> device;
				Vector4 color = Vector4(0.0f);

				inline bool operator==(const ShaderClass_TextureIndex& other)const {
					return device == other.device && color == other.color;
				}
			};
		}
	}
}

namespace std {
	template<>
	struct hash<Jimara::Graphics::ShaderClass_BufferDataIndex> {
		inline size_t operator()(const Jimara::Graphics::ShaderClass_BufferDataIndex& index)const {
			size_t hash = std::hash<Jimara::Graphics::GraphicsDevice*>()(index.device);
			const size_t wordCount = (index.data.Size() / sizeof(size_t));
			{
				const size_t* data = reinterpret_cast<const size_t*>(index.data.Data());
				for (size_t i = 0; i < wordCount; i++)
					hash = Jimara::MergeHashes(hash, std::hash<size_t>()(data[i]));
			}
			{
				static_assert(sizeof(uint8_t) == 1);
				const uint8_t* data = reinterpret_cast<const uint8_t*>(index.data.Data());
				for (size_t i = (wordCount * sizeof(size_t)); i < index.data.Size(); i++)
					hash = Jimara::MergeHashes(hash, std::hash<uint8_t>()(data[i]));
			}
			return hash;
		}
	};

	template<>
	struct hash<Jimara::Graphics::ShaderClass_TextureIndex> {
		inline size_t operator()(const Jimara::Graphics::ShaderClass_TextureIndex& index)const {
			return Jimara::MergeHashes(
				std::hash<Jimara::Graphics::GraphicsDevice*>()(index.device), Jimara::MergeHashes(
					Jimara::MergeHashes(std::hash<float>()(index.color.r), std::hash<float>()(index.color.g)),
					Jimara::MergeHashes(std::hash<float>()(index.color.b), std::hash<float>()(index.color.a))));
		}
	};
}

namespace Jimara {
	namespace Graphics {
		namespace {
#pragma warning (disable: 4250)
			class ShaderClass_SharedConstBufferBinding 
				: public virtual ResourceBinding<Buffer>
				, public virtual ObjectCache<ShaderClass_BufferDataIndex>::StoredObject {
			public:
				inline ShaderClass_SharedConstBufferBinding(Buffer* buffer) 
					: ResourceBinding<Buffer>(buffer) {}

				class VectorBuffer : public virtual Stacktor<uint8_t, 64>, public virtual Object {};

				class Cache : public virtual ObjectCache<ShaderClass_BufferDataIndex> {
				public:
					inline static Reference<ResourceBinding<Buffer>> For(ShaderClass_BufferDataIndex index) {
						static Cache cache;
						return cache.GetCachedOrCreate(index, [&]() -> Reference<ResourceBinding<Buffer>> {
							if (index.data.Size() > 0)
								index.data = [&]() {
								static_assert(sizeof(uint8_t) == 1);
								Reference<VectorBuffer> dataBuffer = Object::Instantiate<VectorBuffer>();
								dataBuffer->Resize(index.data.Size());
								std::memcpy(dataBuffer->Data(), index.data.Data(), index.data.Size());
								return MemoryBlock(dataBuffer->Data(), dataBuffer->Size(), dataBuffer);
							}();
							const Reference<Buffer> buffer = index.device->CreateConstantBuffer(index.data.Size());
							if (index.data.Size() > 0) {
								void* data = buffer->Map();
								memcpy(data, index.data.Data(), index.data.Size());
								buffer->Unmap(true);
							}
							return Object::Instantiate<ShaderClass_SharedConstBufferBinding>(buffer);
							});
					}
				};
			};

			class ShaderClass_SharedTextureSamplerBinding 
				: public virtual ResourceBinding<TextureSampler>,
				public virtual ObjectCache<ShaderClass_TextureIndex>::StoredObject {
			public:
				inline ShaderClass_SharedTextureSamplerBinding(TextureSampler* sampler) 
					: ResourceBinding<TextureSampler>(sampler) {}

				class Cache : public virtual ObjectCache<ShaderClass_TextureIndex> {
				public:
					inline static Reference<ResourceBinding<TextureSampler>> For(const ShaderClass_TextureIndex& index) {
						static Cache cache;
						return cache.GetCachedOrCreate(index, [&]() -> Reference<ResourceBinding<TextureSampler>> {
							const Reference<ImageTexture> texture = index.device->CreateTexture(
								Texture::TextureType::TEXTURE_2D,
								Texture::PixelFormat::R32G32B32A32_SFLOAT,
								Size3(1u, 1u, 1u), 1u, false, Graphics::ImageTexture::AccessFlags::NONE);
							if (texture == nullptr) {
								index.device->Log()->Error("ShaderClass::SharedTextureSamplerBinding - Failed to create default texture!");
								return nullptr;
							}
							{
								Vector4* mapped = reinterpret_cast<Vector4*>(texture->Map());
								if (mapped == nullptr) {
									index.device->Log()->Error("ShaderClass::SharedTextureSamplerBinding - Failed to map default texture memory!");
									return nullptr;
								}
								(*mapped) = index.color;
								texture->Unmap(true);
							}
							const Reference<TextureView> view = texture->CreateView(TextureView::ViewType::VIEW_2D);
							if (view == nullptr) {
								index.device->Log()->Error("SampleDiffuseShader::DefaultTextureSamplerBinding - Failed to create default texture view for!");
								return nullptr;
							}
							const Reference<TextureSampler> sampler = view->CreateSampler();
							if (sampler == nullptr) {
								index.device->Log()->Error("ShaderClass::SharedTextureSamplerBinding - Failed to create default texture sampler for!");
								return nullptr;
							}
							return Object::Instantiate<ShaderClass_SharedTextureSamplerBinding>(sampler);
							});
					}
				};
			};

#pragma warning (default: 4250)
		}

		JIMARA_API Reference<const ResourceBinding<Buffer>> SharedConstantBufferBinding(const void* bufferData, size_t bufferSize, GraphicsDevice* device) {
			if (device == nullptr) return nullptr;
			if (bufferData == nullptr) bufferSize = 0;
			return ShaderClass_SharedConstBufferBinding::Cache::For(ShaderClass_BufferDataIndex{ device, MemoryBlock(bufferData, bufferSize, nullptr) });
		}

		JIMARA_API Reference<const ResourceBinding<TextureSampler>> SharedTextureSamplerBinding(const Vector4& color, GraphicsDevice* device) {
			if (device == nullptr) return nullptr;
			else return ShaderClass_SharedTextureSamplerBinding::Cache::For(ShaderClass_TextureIndex{ device, color });
		}
	}
}
