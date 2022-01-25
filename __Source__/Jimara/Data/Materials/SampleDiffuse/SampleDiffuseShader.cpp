#include "SampleDiffuseShader.h"


namespace Jimara {
	SampleDiffuseShader* SampleDiffuseShader::Instance() {
		static SampleDiffuseShader instance;
		return &instance;
	}

	Material::Instance* SampleDiffuseShader::MaterialInstance() {
		static const Reference<Material::Instance> instance = []() -> Reference<Material::Instance> {
			Reference<Material> material = Object::Instantiate<Material>();
			{
				Material::Writer writer(material);
				writer.SetShader(Instance());
			}
			{
				Material::Reader reader(material);
				return Object::Instantiate<Material::Instance>(&reader);
			}
		}();
		return instance;
	}

	namespace {
		static const constexpr std::string_view TexSamplerName() { return "texSampler"; }
	}

	Reference<Material> SampleDiffuseShader::CreateMaterial(Graphics::Texture* texture) {
		Reference<Material> material = Object::Instantiate<Material>();
		Material::Writer writer(material);
		writer.SetShader(Instance());
		writer.SetTextureSampler(TexSamplerName().data(), texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D)->CreateSampler());
		return material;
	}

	namespace {
#pragma warning (disable: 4250)
		class TexSamplerBinding : public virtual Graphics::ShaderClass::TextureSamplerBinding, public virtual ObjectCache<Reference<Object>>::StoredObject {
		public:
			inline TexSamplerBinding(Graphics::TextureSampler* sampler) : Graphics::ShaderClass::TextureSamplerBinding(sampler) {}

			class Cache : public virtual ObjectCache<Reference<Object>> {
			public:
				inline static Reference<Graphics::ShaderClass::TextureSamplerBinding> For(Graphics::GraphicsDevice* device) {
					static Cache cache;
					return cache.GetCachedOrCreate(device, false, [&]()->Reference<Graphics::ShaderClass::TextureSamplerBinding> {
						const Reference<Graphics::ImageTexture> texture = device->CreateTexture(
							Graphics::Texture::TextureType::TEXTURE_2D,
							Graphics::Texture::PixelFormat::B8G8R8A8_SRGB, 
							Size3(1, 1, 1), 1, false);
						if (texture == nullptr) {
							device->Log()->Error("SampleDiffuseShader::DefaultTextureSamplerBinding - Failed to create default texture for '", TexSamplerName(), "'!");
							return nullptr;
						}
						{
							uint32_t* mapped = reinterpret_cast<uint32_t*>(texture->Map());
							if (mapped == nullptr) {
								device->Log()->Error("SampleDiffuseShader::DefaultTextureSamplerBinding - Failed to map default texture memory for '", TexSamplerName(), "'!");
								return nullptr;
							}
							*mapped = 0xFFFFFFFF;
							texture->Unmap(true);
						}
						const Reference<Graphics::TextureView> view = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
						if (view == nullptr) {
							device->Log()->Error("SampleDiffuseShader::DefaultTextureSamplerBinding - Failed to create default texture view for '", TexSamplerName(), "'!");
							return nullptr;
						}
						const Reference<Graphics::TextureSampler> sampler = view->CreateSampler();
						if (sampler == nullptr) {
							device->Log()->Error("SampleDiffuseShader::DefaultTextureSamplerBinding - Failed to create default texture sampler for '", TexSamplerName(), "'!");
							return nullptr;
						}
						return Object::Instantiate<TexSamplerBinding>(sampler);
						});
				}
			};
		};
#pragma warning (default: 4250)
	}

	Reference<const Graphics::ShaderClass::TextureSamplerBinding> SampleDiffuseShader::DefaultTextureSamplerBinding(const std::string& name, Graphics::GraphicsDevice* device)const {
		if (device == nullptr) return nullptr;
		else if (name == TexSamplerName()) return TexSamplerBinding::Cache::For(device);
		else return nullptr;
	}

	SampleDiffuseShader::SampleDiffuseShader() : Graphics::ShaderClass("Jimara/Data/Materials/SampleDiffuse/Jimara_SampleDiffuseShader") {}
}
