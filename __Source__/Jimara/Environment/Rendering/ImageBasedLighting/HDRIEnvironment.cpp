#include "HDRIEnvironment.h"
#include "../SimpleComputeKernel.h"


namespace Jimara {
	struct HDRIEnvironment::Helpers {
		static const constexpr Size2 DEFAULT_IRRADIANCE_RESOLUTION = Size2(512u, 256u);
		static const constexpr Size2 MAX_PRE_FILTERED_MAP_SIZE = Size2(1024u, 512u);
		static const constexpr Size2 BRDF_INTEGRATION_MAP_SIZE = Size2(512u, 512u);
		static const constexpr Size3 KERNEL_WORKGROUP_SIZE = Size3(16u, 16u, 1u);

		struct PreFilterSettingsBuffer {
			alignas(8) Size2 resolution = {};
			alignas(4) float roughness = 0.0f;
		};
	
		template<typename FailFn>
		inline static Reference<Graphics::TextureSampler> CreateTexture(
			Graphics::GraphicsDevice* device, const Size2& resolution,
			Graphics::Texture::PixelFormat format,
			bool createMipmaps, Graphics::TextureSampler::WrappingMode wrapMode,
			const FailFn& fail) {
			const Reference<Graphics::Texture> texture = device->CreateTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, format,
				Size3(resolution, 1u), 1u, createMipmaps, Graphics::ImageTexture::AccessFlags::SHADER_WRITE);
			if (texture == nullptr)
				return fail("Failed to create texture! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			const Reference<Graphics::TextureView> view = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
			if (view == nullptr)
				return fail("Failed to create texture view! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			const Reference<Graphics::TextureSampler> sampler = view->CreateSampler(
				Graphics::TextureSampler::FilteringMode::LINEAR, wrapMode);
			if (sampler == nullptr)
				return fail("Failed to create texture sampler! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return sampler;
		}

#pragma warning (disable: 4250)
		struct BRDF_IntegrationMapAsset
			: public virtual Asset::Of<Graphics::TextureSampler>
			, public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			Reference<Graphics::TextureSampler> integrationMap;

		protected:
			inline virtual Reference<Graphics::TextureSampler> LoadItem() final override {
				Reference<Graphics::TextureSampler> rv = integrationMap;
				integrationMap = nullptr;
				return rv;
			}

			inline virtual void UnloadItem(Graphics::TextureSampler* resource) final override {
				assert(integrationMap == nullptr);
				integrationMap = resource;
			}

		public:
			inline BRDF_IntegrationMapAsset(Graphics::TextureSampler* sampler)
				: Asset(GUID::Generate()), integrationMap(sampler) {}
			inline virtual ~BRDF_IntegrationMapAsset() {}

			struct Cache : ObjectCache<Reference<const Object>> {
				static Reference<Graphics::TextureSampler> GetSampler(
					Graphics::GraphicsDevice* device, ShaderLibrary* shaderLibrary,
					Graphics::CommandBuffer* commandBuffer) {
					
					if (device == nullptr)
						return nullptr;
					auto fail = [&](const auto&... message) {
						device->Log()->Error("HDRIEnvironment::Helpers::BRDF_IntegrationMapAsset::Cache::GetSampler - ", message...);
						return nullptr;
					};

					if (shaderLibrary == nullptr)
						return fail("Shader loader not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

					static Cache cache;
					const Reference<BRDF_IntegrationMapAsset> asset = cache.GetCachedOrCreate(device, [&]() -> Reference<BRDF_IntegrationMapAsset> {
						const Reference<Graphics::TextureSampler> sampler = CreateTexture(
							device, BRDF_INTEGRATION_MAP_SIZE, Graphics::Texture::PixelFormat::R16G16_SFLOAT,
							false, Graphics::TextureSampler::WrappingMode::CLAMP_TO_EDGE, fail);
						if (sampler == nullptr)
							return nullptr;
						const Reference<const Graphics::ResourceBinding<Graphics::TextureView>> viewBinding =
							Object::Instantiate<Graphics::ResourceBinding<Graphics::TextureView>>(sampler->TargetView());
						auto findView = [&](const auto&) { return viewBinding; };

						static const constexpr std::string_view GENERATOR_SHADER =
							"Jimara/Environment/Rendering/ImageBasedLighting/Jimara_HDRIBRDFIntegrationMapGenerator.comp";
						Graphics::BindingSet::BindingSearchFunctions search = {};
						search.textureView = &findView;
						const Reference<SimpleComputeKernel> preFilteredMapGenerator = SimpleComputeKernel::Create(
							device, shaderLibrary, 1u, GENERATOR_SHADER, search);
						if (preFilteredMapGenerator == nullptr)
							return fail("Failed to create BRDF integration map generator kernel! [File: ", __FILE__, "; Line: ", __LINE__, "]");

						Reference<Graphics::CommandBuffer> commands;
						if (commandBuffer == nullptr) {
							const Reference<Graphics::CommandPool> commandPool = device->GraphicsQueue()->CreateCommandPool();
							if (commandPool == nullptr)
								return fail("Failed to create command pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							commands = commandPool->CreatePrimaryCommandBuffer();
							if (commands == nullptr)
								return fail("Failed to create command buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							commands->BeginRecording();
						}
						else commands = commandBuffer;

						preFilteredMapGenerator->Dispatch(Graphics::InFlightBufferInfo { commands.operator->(), 0u },
							(sampler->TargetView()->TargetTexture()->Size() + KERNEL_WORKGROUP_SIZE - 1u) / KERNEL_WORKGROUP_SIZE);

						if (commandBuffer == nullptr) {
							commands->EndRecording();
							device->GraphicsQueue()->ExecuteCommandBuffer(dynamic_cast<Graphics::PrimaryCommandBuffer*>(commands.operator->()));
							dynamic_cast<Graphics::PrimaryCommandBuffer*>(commands.operator->())->Wait();
						}

						return Object::Instantiate<BRDF_IntegrationMapAsset>(sampler);
						});

					if (asset == nullptr)
						return nullptr;
					else return asset->Load();
				}
			};
		};
#pragma warning (default: 4250)

		template<typename FailFn>
		inline static bool GenerateIrradianceMap(
			Graphics::GraphicsDevice* device, ShaderLibrary* shaderLibrary,
			Graphics::TextureSampler* hdri, Graphics::TextureView* irradianceMap,
			Graphics::CommandBuffer* commandBuffer, const FailFn& fail) {
			static const constexpr std::string_view GENERATOR_SHADER =
				"Jimara/Environment/Rendering/ImageBasedLighting/Jimara_HDRIDiffuseIrradianceGenerator.comp";
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
				device, shaderLibrary, 1u, GENERATOR_SHADER, search);
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
			Graphics::GraphicsDevice* device, ShaderLibrary* shaderLibrary,
			Graphics::TextureSampler* hdri, Graphics::Texture* preFilteredMap,
			Graphics::OneTimeCommandPool* commandPool,
			const FailFn& fail) {
			static const std::string_view GENERATOR_SHADER =
				"Jimara/Environment/Rendering/ImageBasedLighting/Jimara_HDRIPreFilteredEnvironmentMapGenerator.comp";
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
				device, shaderLibrary, preFilteredMap->MipLevels(), GENERATOR_SHADER, search);
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

				Graphics::OneTimeCommandPool::Buffer commandBuffer(commandPool);
				if (!commandBuffer) {
					fail("Failed to create command buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}

				Graphics::InFlightBufferInfo bufferInfo(commandBuffer, i);
				preFilteredMapGenerator->Dispatch(bufferInfo, (mipSize + KERNEL_WORKGROUP_SIZE - 1u) / KERNEL_WORKGROUP_SIZE);
			}

			return true;
		}
	};

	Reference<HDRIEnvironment> HDRIEnvironment::Create(
		Graphics::GraphicsDevice* device,
		ShaderLibrary* shaderLibrary,
		Graphics::TextureSampler* hdri) {
		if (device == nullptr)
			return nullptr;
		auto fail = [&](const auto&... message) {
			device->Log()->Error("HDRIEnvironment::Create - ", message...);
			return nullptr;
		};
		if (shaderLibrary == nullptr)
			return fail("Shader library not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		if (hdri == nullptr)
			return fail("Texture not provided! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Get command buffer:
		const Reference<Graphics::OneTimeCommandPool> oneTimeCommandPool = Graphics::OneTimeCommandPool::GetFor(device);
		if (oneTimeCommandPool == nullptr)
			return fail("Failed to get one-time command pool! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		Graphics::OneTimeCommandPool::Buffer irradianceCommands(oneTimeCommandPool);
		if (!irradianceCommands)
			return fail("Failed to create command buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		// Create irradiance texture:
		const Reference<Graphics::TextureSampler> irradianceSampler = Helpers::CreateTexture(
			device, Helpers::DEFAULT_IRRADIANCE_RESOLUTION, Graphics::Texture::PixelFormat::R16G16B16A16_SFLOAT,
			true, Graphics::TextureSampler::WrappingMode::REPEAT, fail);
		if (irradianceSampler == nullptr)
			return nullptr;
		if (!Helpers::GenerateIrradianceMap(device, shaderLibrary, hdri, irradianceSampler->TargetView(), irradianceCommands, fail))
			return nullptr;

		// Create pre-filtered map:
		const Reference<Graphics::TextureSampler> preFilteredMap = Helpers::CreateTexture(
			device, Size3(
				Math::Min(hdri->TargetView()->TargetTexture()->Size().x, Helpers::MAX_PRE_FILTERED_MAP_SIZE.x),
				Math::Min(hdri->TargetView()->TargetTexture()->Size().y, Helpers::MAX_PRE_FILTERED_MAP_SIZE.y), 1),
			Graphics::Texture::PixelFormat::R16G16B16A16_SFLOAT,
			true, Graphics::TextureSampler::WrappingMode::REPEAT, fail);
		if (preFilteredMap == nullptr)
			return nullptr;
		if (!Helpers::GeneratePreFilteredMap(device, shaderLibrary, hdri, preFilteredMap->TargetView()->TargetTexture(), oneTimeCommandPool, fail))
			return nullptr;

		// Create/Get BRDF integration map:
		const Reference<Graphics::TextureSampler> brdfIntegrationMap = Helpers::BRDF_IntegrationMapAsset::Cache::GetSampler(
			device, shaderLibrary, nullptr);
		if (brdfIntegrationMap == nullptr)
			return nullptr;

		const Reference<HDRIEnvironment> result = new HDRIEnvironment(hdri, irradianceSampler, preFilteredMap, brdfIntegrationMap, oneTimeCommandPool);
		result->ReleaseRef();
		return result;
	}


	Reference<Graphics::TextureSampler> HDRIEnvironment::BrdfIntegrationMap(
		Graphics::GraphicsDevice* device,
		ShaderLibrary* shaderLibrary) {
		return Helpers::BRDF_IntegrationMapAsset::Cache::GetSampler(device, shaderLibrary, nullptr);
	}
}
