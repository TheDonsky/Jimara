#include "SampleDiffuseShader.h"
#include "../../Serialization/Attributes/ColorAttribute.h"


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
		static const constexpr std::string_view BaseColorName() { return "baseColor"; }
		static const constexpr std::string_view TexSamplerName() { return "texSampler"; }
	}

	Reference<Material> SampleDiffuseShader::CreateMaterial(Graphics::Texture* texture, Graphics::GraphicsDevice* device) {
		Reference<Material> material = Object::Instantiate<Material>(device);
		Material::Writer writer(material);
		writer.SetShader(Instance());
		writer.SetTextureSampler(TexSamplerName().data(), texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D)->CreateSampler());
		return material;
	}

	Reference<const Graphics::ShaderClass::ConstantBufferBinding> SampleDiffuseShader::DefaultConstantBufferBinding(const std::string_view& name, Graphics::GraphicsDevice* device)const {
		if (name == BaseColorName()) return SharedConstantBufferBinding<Vector3>(Vector3(1.0f), device);
		else return nullptr;
	}

	void SampleDiffuseShader::SerializeBindings(Callback<Serialization::SerializedObject> reportField, Bindings* bindings)const {
		{
			static const ConstantBufferSerializer<Vector3> serializer(BaseColorName(),
				Serialization::ValueSerializer<Vector3>::Create("Color", "Diffuse Color", { Object::Instantiate<Serialization::ColorAttribute>() }), Vector3(1.0f));
			serializer.Serialize(reportField, bindings);
		}
		{
			static const TextureSamplerSerializer serializer(TexSamplerName(), "Diffuse", "Diffuse texture");
			reportField(serializer.Serialize(bindings));
		}
	}

	SampleDiffuseShader::SampleDiffuseShader() : Graphics::ShaderClass("Jimara/Data/Materials/SampleDiffuse/Jimara_SampleDiffuseShader") {}
}
