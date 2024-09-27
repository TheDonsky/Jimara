#include "SampleUIShader.h"
#include "../MaterialInstanceCache.h"


namespace Jimara {
	const OS::Path SampleUIShader::PATH = "Jimara/Data/Materials/SampleUI/Jimara_SampleUIShader";

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
}
