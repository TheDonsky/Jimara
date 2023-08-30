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
		ShaderClass::ShaderClass(const OS::Path& shaderPath, GraphicsPipeline::BlendMode blendMode) 
			: m_shaderPath(shaderPath), m_pathStr(shaderPath), m_blendMode(blendMode) {}

		const OS::Path& ShaderClass::ShaderPath()const { return m_shaderPath; }

		GraphicsPipeline::BlendMode ShaderClass::BlendMode()const { return m_blendMode; }

		namespace {
#pragma warning (disable: 4250)
			class ShaderClass_SharedConstBufferBinding 
				: public virtual ShaderClass::ConstantBufferBinding
				, public virtual ObjectCache<ShaderClass_BufferDataIndex>::StoredObject {
			public:
				inline ShaderClass_SharedConstBufferBinding(Buffer* buffer) 
					: ShaderClass::ConstantBufferBinding(buffer) {}

				class VectorBuffer : public virtual Stacktor<uint8_t, 64>, public virtual Object {};

				class Cache : public virtual ObjectCache<ShaderClass_BufferDataIndex> {
				public:
					inline static Reference<ShaderClass::ConstantBufferBinding> For(ShaderClass_BufferDataIndex index) {
						static Cache cache;
						return cache.GetCachedOrCreate(index, false, [&]()->Reference<ShaderClass::ConstantBufferBinding> {
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
				: public virtual ShaderClass::TextureSamplerBinding, 
				public virtual ObjectCache<ShaderClass_TextureIndex>::StoredObject {
			public:
				inline ShaderClass_SharedTextureSamplerBinding(TextureSampler* sampler) 
					: ShaderClass::TextureSamplerBinding(sampler) {}

				class Cache : public virtual ObjectCache<ShaderClass_TextureIndex> {
				public:
					inline static Reference<ShaderClass::TextureSamplerBinding> For(const ShaderClass_TextureIndex& index) {
						static Cache cache;
						return cache.GetCachedOrCreate(index, false, [&]()->Reference<ShaderClass::TextureSamplerBinding> {
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

		Reference<const ShaderClass::ConstantBufferBinding> ShaderClass::SharedConstantBufferBinding(const void* bufferData, size_t bufferSize, GraphicsDevice* device) {
			if (device == nullptr) return nullptr;
			if (bufferData == nullptr) bufferSize = 0;
			return ShaderClass_SharedConstBufferBinding::Cache::For(ShaderClass_BufferDataIndex{ device, MemoryBlock(bufferData, bufferSize, nullptr) });
		}

		Reference<const ShaderClass::TextureSamplerBinding> ShaderClass::SharedTextureSamplerBinding(const Vector4& color, GraphicsDevice* device) {
			if (device == nullptr) return nullptr;
			else return ShaderClass_SharedTextureSamplerBinding::Cache::For(ShaderClass_TextureIndex{ device, color });
		}


		Reference<ShaderClass::Set> ShaderClass::Set::All() {
			static SpinLock allLock;
			static Reference<Set> all;
			static Reference<RegisteredTypeSet> registeredTypes;

			std::unique_lock<SpinLock> lock(allLock);
			{
				const Reference<RegisteredTypeSet> currentTypes = RegisteredTypeSet::Current();
				if (currentTypes == registeredTypes) return all;
				else registeredTypes = currentTypes;
			}
			std::set<Reference<const ShaderClass>> shaders;
			for (size_t i = 0; i < registeredTypes->Size(); i++) {
				void(*checkAttribute)(decltype(shaders)*, const Object*) = [](decltype(shaders)* shaderSet, const Object* attribute) {
					const ShaderClass* shaderClass = dynamic_cast<const ShaderClass*>(attribute);
					if (shaderClass != nullptr)
						shaderSet->insert(shaderClass);
				};
				registeredTypes->At(i).GetAttributes(Callback<const Object*>(checkAttribute, &shaders));
			}
			all = new Set(shaders);
			all->ReleaseRef();
			return all;
		}

		size_t ShaderClass::Set::Size()const { return m_shaders.size(); }

		const ShaderClass* ShaderClass::Set::At(size_t index)const { return m_shaders[index]; }

		const ShaderClass* ShaderClass::Set::operator[](size_t index)const { return At(index); }

		const ShaderClass* ShaderClass::Set::FindByPath(const OS::Path& shaderPath)const {
			const decltype(m_shadersByPath)::const_iterator it = m_shadersByPath.find(shaderPath);
			if (it == m_shadersByPath.end()) return nullptr;
			else return it->second;
		}

		size_t ShaderClass::Set::IndexOf(const ShaderClass* shaderClass)const {
			IndexPerShader::const_iterator it = m_indexPerShader.find(shaderClass);
			if (it == m_indexPerShader.end()) return NoIndex();
			else return it->second;
		}

		ShaderClass::Set::Set(const std::set<Reference<const ShaderClass>>& shaders)
			: m_shaders(shaders.begin(), shaders.end())
			, m_indexPerShader([&]() {
				IndexPerShader index;
				for (std::set<Reference<const ShaderClass>>::const_iterator it = shaders.begin(); it != shaders.end(); ++it)
					index.insert(std::make_pair(it->operator->(), index.size()));
				return index;
				}())
			, m_shadersByPath([&]() {
				ShadersByPath shadersByPath;
				for (std::set<Reference<const ShaderClass>>::const_iterator it = shaders.begin(); it != shaders.end(); ++it) {
					const std::string path = it->operator->()->ShaderPath();
					if (path.size() > 0)
						shadersByPath[path] = *it;
				}
				return shadersByPath;
				}())
			, m_classSelector([&]() {
				typedef Serialization::EnumAttribute<std::string_view> EnumAttribute;
				std::vector<EnumAttribute::Choice> choices;
				choices.push_back(EnumAttribute::Choice("<None>", ""));
				for (std::set<Reference<const ShaderClass>>::const_iterator it = shaders.begin(); it != shaders.end(); ++it) {
					const std::string& path = it->operator->()->m_pathStr;
					choices.push_back(EnumAttribute::Choice(path, path));
				}
				typedef std::string_view(*GetFn)(const ShaderClass**);
				typedef void(*SetFn)(const std::string_view&, const ShaderClass**);
				return Serialization::ValueSerializer<std::string_view>::For<const ShaderClass*>("Shader", "Shader class", 
					(GetFn)[](const ShaderClass** shaderClass) -> std::string_view {
						if (shaderClass == nullptr || (*shaderClass) == nullptr) return "";
						else return (*shaderClass)->m_pathStr;
					},
					(SetFn)[](const std::string_view& value, const ShaderClass** shaderClass) {
						if (shaderClass == nullptr) return;
						Reference<Set> allShaders = Set::All();
						(*shaderClass) = allShaders->FindByPath(value);
					}, { Object::Instantiate<EnumAttribute>(choices, false) });
				}()){
#ifndef NDEBUG
			for (size_t i = 0; i < m_shaders.size(); i++)
				assert(IndexOf(m_shaders[i]) == i);
#endif // !NDEBUG
		}


		ShaderClass::TextureSamplerSerializer::TextureSamplerSerializer(
			const std::string_view& bindingName,
			const std::string_view& serializerName,
			const std::string_view& serializerTooltip,
			const std::vector<Reference<const Object>> serializerAttributes)
			: m_bindingName(bindingName) {
			
			typedef TextureSampler* (*GetFn)(const TextureSamplerSerializer*, Bindings*);
			static const GetFn getSampler = [](const TextureSamplerSerializer* self, Bindings* target) { 
				return target->GetTextureSampler(self->m_bindingName); 
			};

			typedef void (*SetFn)(const TextureSamplerSerializer*, TextureSampler* const&, Bindings*);
			static const SetFn setSampler = [](const TextureSamplerSerializer* self, TextureSampler* const& sampler, Bindings* target) {
				return target->SetTextureSampler(self->m_bindingName, sampler);
			};

			m_serializer = Serialization::ValueSerializer<TextureSampler*>::Create<Bindings>(
				serializerName, serializerTooltip,
				Function<TextureSampler*, Bindings*>(getSampler, this),
				Callback<TextureSampler* const&, Bindings*>(setSampler, this),
				serializerAttributes);
		}
	}
}
