#include "SampleTextShader.h"
#include "../MaterialInstanceCache.h"


namespace Jimara {
	SampleTextShader* SampleTextShader::Instance() {
		static SampleTextShader instance;
		return &instance;
	}

	Reference<const Material::Instance> SampleTextShader::MaterialInstance(
		Graphics::GraphicsDevice* device,
		Graphics::BindlessSet<Graphics::ArrayBuffer>* bindlessBuffers,
		Graphics::BindlessSet<Graphics::TextureSampler>* bindlessSamplers,
		ShaderLibrary* library) {
		if (device == nullptr || bindlessBuffers == nullptr || bindlessSamplers == nullptr || library == nullptr)
			return nullptr;
		const Reference<const Material::LitShader> shader =
			library->LitShaders()->FindByPath("Jimara/Data/Materials/SampleText/Jimara_SampleTextShader");
		if (shader == nullptr) {
			device->Log()->Error("SampleTextShader::MaterialInstance - Failed to find lit-shader for SampleUIShader!");
			return nullptr;
		}
		return MaterialInstanceCache::SharedInstance(device, bindlessBuffers, bindlessSamplers, shader);
	}

	void SampleTextShader::SerializeBindings(Callback<Serialization::SerializedObject> reportField, Bindings* bindings)const {}

	SampleTextShader::SampleTextShader() : Graphics::ShaderClass("Jimara/Data/Materials/SampleText/Jimara_SampleTextShader", Graphics::GraphicsPipeline::BlendMode::ALPHA_BLEND) {}
}
