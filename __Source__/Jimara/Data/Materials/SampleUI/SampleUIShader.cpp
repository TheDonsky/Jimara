#include "SampleUIShader.h"
#include "../MaterialInstanceCache.h"


namespace Jimara {
	SampleUIShader* SampleUIShader::Instance() {
		static SampleUIShader instance;
		return &instance;
	}

	Reference<const Material::Instance> SampleUIShader::MaterialInstance(
		Graphics::GraphicsDevice* device, 
		Graphics::BindlessSet<Graphics::ArrayBuffer>* bindlessBuffers,
		Graphics::BindlessSet<Graphics::TextureSampler>* bindlessSamplers,
		const Material::LitShaderSet* shaders) {
		if (device == nullptr || bindlessBuffers == nullptr || bindlessSamplers == nullptr || shaders == nullptr)
			return nullptr;
		const Reference<const Material::LitShader> shader = 
			shaders->FindByPath("Jimara/Data/Materials/SampleUI/Jimara_SampleUIShader");
		if (shader == nullptr) {
			device->Log()->Error("SampleUIShader::MaterialInstance - Failed to find lit-shader for SampleUIShader!");
			return nullptr;
		}
		return MaterialInstanceCache::SharedInstance(device, bindlessBuffers, bindlessSamplers, shader);
	}

	void SampleUIShader::SerializeBindings(Callback<Serialization::SerializedObject> reportField, Bindings* bindings)const {}

	SampleUIShader::SampleUIShader() : Graphics::ShaderClass("Jimara/Data/Materials/SampleUI/Jimara_SampleUIShader", Graphics::GraphicsPipeline::BlendMode::ALPHA_BLEND) {}
}
