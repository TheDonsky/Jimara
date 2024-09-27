#include "SampleTextShader.h"
#include "../MaterialInstanceCache.h"


namespace Jimara {
	const OS::Path SampleTextShader::PATH = "Jimara/Data/Materials/SampleText/Jimara_SampleTextShader";

	Reference<const Material::Instance> SampleTextShader::MaterialInstance(
		Graphics::GraphicsDevice* device,
		Graphics::BindlessSet<Graphics::ArrayBuffer>* bindlessBuffers,
		Graphics::BindlessSet<Graphics::TextureSampler>* bindlessSamplers,
		const Material::LitShaderSet* shaders) {
		if (device == nullptr || bindlessBuffers == nullptr || bindlessSamplers == nullptr || shaders == nullptr)
			return nullptr;
		const Reference<const Material::LitShader> shader =
			shaders->FindByPath("Jimara/Data/Materials/SampleText/Jimara_SampleTextShader");
		if (shader == nullptr) {
			device->Log()->Error("SampleTextShader::MaterialInstance - Failed to find lit-shader for SampleUIShader!");
			return nullptr;
		}
		return MaterialInstanceCache::SharedInstance(device, bindlessBuffers, bindlessSamplers, shader);
	}
}
