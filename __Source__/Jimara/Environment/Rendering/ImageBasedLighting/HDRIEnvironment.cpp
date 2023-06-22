#include "HDRIEnvironment.h"
#include "../SimpleComputeKernel.h"


namespace Jimara {
	struct HDRIEnvironment::Helpers {
		static const constexpr Size2 DEFAULT_IRRADIANCE_RESOLUTION = Size2(512u, 256u);
		static const constexpr Size3 KERNEL_WORKGROUP_SIZE = Size3(16u, 16u, 1u);
	};

	Reference<HDRIEnvironment> HDRIEnvironment::Create(
		Graphics::GraphicsDevice* device,
		Graphics::ShaderLoader* shaderLoader,
		Graphics::TextureSampler* hdri) {
		if (device == nullptr)
			return nullptr;
		auto fail = [&](const auto&... message) {
			device->Log()->Error("HDRIEnvironment::Create - ", message...);
			return nullptr;
		};
		if (shaderLoader == nullptr)
			return fail("Shader loader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		if (hdri == nullptr)
			return fail("Texture not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Create irradiance texture:
		const Reference<Graphics::Texture> irradianceTexture = device->CreateTexture(
			Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R16G16B16A16_SFLOAT,
			Size3(Helpers::DEFAULT_IRRADIANCE_RESOLUTION, 1u), 1u, true);
		if (irradianceTexture == nullptr)
			return fail("Failed to create texture! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<Graphics::TextureView> irradianceView = irradianceTexture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
		if (irradianceView == nullptr)
			return fail("Failed to create texture view! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<Graphics::TextureSampler> irradianceSampler = irradianceView->CreateSampler();
		if (irradianceSampler == nullptr)
			return fail("Failed to create texture sampler! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		static const Graphics::ShaderClass GENERATOR_SHADER("Jimara/Environment/Rendering/ImageBasedLighting/Jimara_HDRIDiffuseIrradianceGenerator");
		Graphics::BindingSet::BindingSearchFunctions search = {};
		const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> hdriBinding =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>(hdri);
		auto findSampler = [&](const auto&) { return hdriBinding; };
		search.textureSampler = &findSampler;
		const Reference<Graphics::ResourceBinding<Graphics::TextureView>> irradianceMapBinding =
			Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>(irradianceView);
		auto findView = [&](const auto&) { return irradianceMapBinding; };
		search.textureView = &findView;
		const Reference<SimpleComputeKernel> irradianceGenerator = SimpleComputeKernel::Create(
			device, shaderLoader, 1u, &GENERATOR_SHADER, search);
		if (irradianceGenerator == nullptr)
			return fail("Failed to create irradiance generator kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Graphics::CommandPool> commandPool = device->GraphicsQueue()->CreateCommandPool();
		if (commandPool == nullptr)
			return fail("Failed to create command pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<Graphics::PrimaryCommandBuffer> commandBuffer = commandPool->CreatePrimaryCommandBuffer();
		if (commandBuffer == nullptr)
			return fail("Failed to create command buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		commandBuffer->BeginRecording();
		irradianceGenerator->Dispatch({ commandBuffer, 0u },
			(irradianceTexture->Size() + Helpers::KERNEL_WORKGROUP_SIZE - 1u) / Helpers::KERNEL_WORKGROUP_SIZE);
		irradianceTexture->GenerateMipmaps(commandBuffer);
		commandBuffer->EndRecording();
		device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
		commandBuffer->Wait();

		const Reference<HDRIEnvironment> result = new HDRIEnvironment(hdri, irradianceSampler);
		result->ReleaseRef();
		return result;
	}
}
