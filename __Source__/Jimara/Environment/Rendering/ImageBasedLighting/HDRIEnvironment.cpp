#include "HDRIEnvironment.h"
#include "../SimpleComputeKernel.h"


namespace Jimara {
	struct HDRIEnvironment::Helpers {
		static const constexpr Size2 DEFAULT_IRRADIANCE_RESOLUTION = Size2(512u, 256u);
		static const constexpr Size3 KERNEL_WORKGROUP_SIZE = Size3(16u, 16u, 1u);

		struct PreFilterSettingsBuffer {
			alignas(8) Size2 resolution = {};
			alignas(4) float roughness = 0.0f;
		};
	
		template<typename FailFn>
		inline static Reference<Graphics::TextureSampler> CreateTexture(Graphics::GraphicsDevice* device, const Size2& resolution, const FailFn& fail) {
			const Reference<Graphics::Texture> texture = device->CreateTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R16G16B16A16_SFLOAT,
				Size3(resolution, 1u), 1u, true);
			if (texture == nullptr)
				return fail("Failed to create texture! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			const Reference<Graphics::TextureView> view = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
			if (view == nullptr)
				return fail("Failed to create texture view! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			const Reference<Graphics::TextureSampler> sampler = view->CreateSampler();
			if (sampler == nullptr)
				return fail("Failed to create texture sampler! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return sampler;
		}

		template<typename FailFn>
		inline static bool GenerateIrradianceMap(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader,
			Graphics::TextureSampler* hdri, Graphics::TextureView* irradianceMap,
			Graphics::CommandBuffer* commandBuffer, const FailFn& fail) {
			static const Graphics::ShaderClass GENERATOR_SHADER("Jimara/Environment/Rendering/ImageBasedLighting/Jimara_HDRIDiffuseIrradianceGenerator");
			Graphics::BindingSet::BindingSearchFunctions search = {};
			const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> hdriBinding =
				Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>(hdri);
			auto findSampler = [&](const auto&) { return hdriBinding; };
			search.textureSampler = &findSampler;
			const Reference<Graphics::ResourceBinding<Graphics::TextureView>> irradianceMapBinding =
				Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>(irradianceMap);
			auto findView = [&](const auto&) { return irradianceMapBinding; };
			search.textureView = &findView;
			const Reference<SimpleComputeKernel> irradianceGenerator = SimpleComputeKernel::Create(
				device, shaderLoader, 1u, &GENERATOR_SHADER, search);
			if (irradianceGenerator == nullptr) {
				fail("Failed to create irradiance generator kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return false;
			}

			irradianceGenerator->Dispatch({ commandBuffer, 0u },
				(irradianceMap->TargetTexture()->Size() + Helpers::KERNEL_WORKGROUP_SIZE - 1u) / Helpers::KERNEL_WORKGROUP_SIZE);
			irradianceMap->TargetTexture()->GenerateMipmaps(commandBuffer);
			return true;
		}

		template<typename FailFn>
		inline static bool GeneratePreFilteredMap(
			Graphics::GraphicsDevice* device, Graphics::ShaderLoader* shaderLoader,
			Graphics::TextureSampler* hdri, Graphics::Texture* preFilteredMap,
			Graphics::CommandBuffer* commandBuffer, const FailFn& fail) {
			static const Graphics::ShaderClass GENERATOR_SHADER("Jimara/Environment/Rendering/ImageBasedLighting/Jimara_HDRIPreFilteredEnvironmentMapGenerator");
			Graphics::BindingSet::BindingSearchFunctions search = {};
			
			const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> hdriBinding =
				Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureSampler>>(hdri);
			auto findSampler = [&](const auto&) { return hdriBinding; };
			search.textureSampler = &findSampler;
			
			const Reference<Graphics::ResourceBinding<Graphics::TextureView>> preFilteredMapBinding =
				Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>();
			auto findView = [&](const auto&) { return preFilteredMapBinding; };
			search.textureView = &findView;

			const Graphics::BufferReference<PreFilterSettingsBuffer> roughnessBuffer = 
				device->CreateConstantBuffer<PreFilterSettingsBuffer>();
			if (roughnessBuffer == nullptr) {
				fail("Failed to create pre-filter settings buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return false;
			}
			const Reference<const Graphics::ResourceBinding<Graphics::Buffer>> settingsBinding =
				Object::Instantiate<Graphics::ResourceBinding<Graphics::Buffer>>(roughnessBuffer);
			auto findCbuffer = [&](const auto&) { return settingsBinding; };
			search.constantBuffer = &findCbuffer;

			const Reference<SimpleComputeKernel> preFilteredMapGenerator = SimpleComputeKernel::Create(
				device, shaderLoader, preFilteredMap->MipLevels(), &GENERATOR_SHADER, search);
			if (preFilteredMapGenerator == nullptr) {
				fail("Failed to create irradiance generator kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return false;
			}

			const Size3 imageSize = preFilteredMap->Size();
			for (uint32_t i = 0u; i < preFilteredMap->MipLevels(); i++) {
				preFilteredMapBinding->BoundObject() = preFilteredMap->CreateView(Graphics::TextureView::ViewType::VIEW_2D, i, 1u);
				if (preFilteredMapBinding->BoundObject() == nullptr) {
					fail("Failed to create mip ", i, " view! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}

				const Size3 mipSize(
					Math::Max(imageSize.x >> i, 1u),
					Math::Max(imageSize.y >> i, 1u),
					Math::Max(imageSize.z >> i, 1u));

				roughnessBuffer.Map() = PreFilterSettingsBuffer{
					Size2(mipSize.x, mipSize.y),
					float(i) / float(preFilteredMap->MipLevels() - 1u)
				};
				roughnessBuffer->Unmap(true);

				Graphics::InFlightBufferInfo bufferInfo(commandBuffer, i);
				preFilteredMapGenerator->Dispatch(bufferInfo, (mipSize + KERNEL_WORKGROUP_SIZE - 1u) / KERNEL_WORKGROUP_SIZE);
			}

			return true;
		}
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

		// Create command buffer:
		const Reference<Graphics::CommandPool> commandPool = device->GraphicsQueue()->CreateCommandPool();
		if (commandPool == nullptr)
			return fail("Failed to create command pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		const Reference<Graphics::PrimaryCommandBuffer> commandBuffer = commandPool->CreatePrimaryCommandBuffer();
		if (commandBuffer == nullptr)
			return fail("Failed to create command buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		commandBuffer->BeginRecording();

		// Create irradiance texture:
		const Reference<Graphics::TextureSampler> irradianceSampler = Helpers::CreateTexture(device, Helpers::DEFAULT_IRRADIANCE_RESOLUTION, fail);
		if (irradianceSampler == nullptr)
			return nullptr;
		if (!Helpers::GenerateIrradianceMap(device, shaderLoader, hdri, irradianceSampler->TargetView(), commandBuffer, fail))
			return nullptr;

		// Create pre-filtered map:
		const Reference<Graphics::TextureSampler> preFilteredMap = Helpers::CreateTexture(device, hdri->TargetView()->TargetTexture()->Size(), fail);
		if (preFilteredMap == nullptr)
			return nullptr;
		if (!Helpers::GeneratePreFilteredMap(device, shaderLoader, hdri, preFilteredMap->TargetView()->TargetTexture(), commandBuffer, fail))
			return nullptr;

		// Submit command buffer:
		commandBuffer->EndRecording();
		device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
		commandBuffer->Wait();

		const Reference<HDRIEnvironment> result = new HDRIEnvironment(hdri, irradianceSampler, preFilteredMap);
		result->ReleaseRef();
		return result;
	}
}
