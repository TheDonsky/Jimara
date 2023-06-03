#include "SampleUIShader.h"


namespace Jimara {
	SampleUIShader* SampleUIShader::Instance() {
		static SampleUIShader instance;
		return &instance;
	}

	Reference<const Material::Instance> SampleUIShader::MaterialInstance(Graphics::GraphicsDevice* device) {
#pragma warning(disable: 4250)
		if (device == nullptr) return nullptr;
		struct CachedInstance : public virtual Material::Instance, public virtual ObjectCache<Reference<Object>>::StoredObject {
			inline CachedInstance(Material::Reader& reader) : Material::Instance(&reader) {}
			inline virtual ~CachedInstance() {}
		};
#pragma warning(default: 4250)
		struct Cache : public virtual ObjectCache<Reference<Object>> {
			inline static Reference<Material::Instance> GetFor(Graphics::GraphicsDevice* device) {
				static Cache cache;
				return cache.GetCachedOrCreate(device, false, [&]() -> Reference<CachedInstance> {
					Reference<Material> material = Object::Instantiate<Material>(device);
					{
						Material::Writer writer(material);
						writer.SetShader(Instance());
					}
					return Object::Instantiate<CachedInstance>(Material::Reader(material));
					});
			}
		};
		return Cache::GetFor(device);
	}

	void SampleUIShader::SerializeBindings(Callback<Serialization::SerializedObject> reportField, Bindings* bindings)const {}

	SampleUIShader::SampleUIShader() : Graphics::ShaderClass("Jimara/Data/Materials/SampleUI/Jimara_SampleUIShader", Graphics::GraphicsPipeline::BlendMode::ALPHA_BLEND) {}
}