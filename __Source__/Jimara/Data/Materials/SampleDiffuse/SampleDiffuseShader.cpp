#include "SampleDiffuseShader.h"


namespace Jimara {
	SampleDiffuseShader* SampleDiffuseShader::Instance() {
		static SampleDiffuseShader instance;
		return &instance;
	}

	namespace {
#pragma warning(disable: 4250)
		class SampleDiffuseShaderMaterialInstance : public virtual Material::Instance, public virtual ObjectCache<Reference<Graphics::GraphicsDevice>>::StoredObject {
		public:
			inline SampleDiffuseShaderMaterialInstance(Material::Reader& reader) : Material::Instance(&reader) {}

			class Cache : public virtual ObjectCache<Reference<Graphics::GraphicsDevice>> {
			public:
				inline static Reference<SampleDiffuseShaderMaterialInstance> GetFor(Graphics::GraphicsDevice* device) {
					static Cache cache;
					return cache.GetCachedOrCreate(device, false, [&]()->Reference<SampleDiffuseShaderMaterialInstance> {
						Reference<Material> material = Object::Instantiate<Material>(device);
						{
							Material::Writer writer(material);
							writer.SetShader(SampleDiffuseShader::Instance());
						}
						return Object::Instantiate<SampleDiffuseShaderMaterialInstance>(Material::Reader(material));
						});
				}
			};
		};
#pragma warning(default: 4250)
	}

	Reference<Material::Instance> SampleDiffuseShader::MaterialInstance(Graphics::GraphicsDevice* device) {
		return SampleDiffuseShaderMaterialInstance::Cache::GetFor(device);
	}

	namespace {
		static const constexpr std::string_view TexSamplerName() { return "texSampler"; }
	}

	Reference<Material> SampleDiffuseShader::CreateMaterial(Graphics::Texture* texture, Graphics::GraphicsDevice* device) {
		Reference<Material> material = Object::Instantiate<Material>(device);
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

	Reference<const Graphics::ShaderClass::TextureSamplerBinding> SampleDiffuseShader::DefaultTextureSamplerBinding(const std::string_view& name, Graphics::GraphicsDevice* device)const {
		if (device == nullptr) return nullptr;
		else if (name == TexSamplerName()) return TexSamplerBinding::Cache::For(device);
		else return nullptr;
	}

	void SampleDiffuseShader::SerializeBindings(Callback<Serialization::SerializedObject> reportField, Bindings* bindings)const {
		typedef const Reference<const Serialization::ItemSerializer::Of<Bindings>> BindingSerializer;
		{
			typedef Graphics::TextureSampler*(*GetFn)(Bindings*);
			typedef void(*SetFn)(Graphics::TextureSampler* const&, Bindings*);
			static BindingSerializer serializer = Serialization::ValueSerializer<Graphics::TextureSampler*>::Create<Bindings>(
				"Diffuse", "Diffuse texture", 
				Function((GetFn)[](Bindings* target) -> Graphics::TextureSampler* { return target->GetTextureSampler(TexSamplerName()); }),
				Callback((SetFn)[](Graphics::TextureSampler* const& sampler, Bindings* target) { target->SetTextureSampler(TexSamplerName(), sampler); }));
			reportField(serializer->Serialize(bindings));
		}
	}

	SampleDiffuseShader::SampleDiffuseShader() : Graphics::ShaderClass("Jimara/Data/Materials/SampleDiffuse/Jimara_SampleDiffuseShader") {}
}
