#pragma once
#include "../VulkanDevice.h"
#include "../Memory/VulkanTexture.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			class VulkanRenderEngineInfo : public virtual RenderEngineInfo {
			public:
				virtual size_t ImageCount()const = 0;

				virtual VulkanImage* Image(size_t imageId)const = 0;

				virtual VkSampleCountFlagBits MSAASamples(GraphicsSettings::MSAA desired)const = 0;
			};

			class VulkanRenderEngine : public RenderEngine {
			public:
				class CommandRecorder {
				public:
					inline virtual ~CommandRecorder() {}

					virtual size_t ImageIndex()const = 0;

					virtual VulkanImage* Image()const = 0;

					virtual VkCommandBuffer CommandBuffer()const = 0;

					virtual void RecordBufferDependency(Object* object) = 0;
				};

				VulkanDevice* Device()const;

			protected:
				VulkanRenderEngine(VulkanDevice* device);

			private:
				Reference<VulkanDevice> m_device;
			};

			class VulkanImageRenderer : public ImageRenderer {
			public:
				class EngineData : public virtual Object {
				private:
					Reference<VulkanImageRenderer> m_renderer;
					VulkanRenderEngineInfo* m_engineInfo;

				public:
					EngineData(VulkanImageRenderer* renderer, VulkanRenderEngineInfo* engineInfo);

					VulkanImageRenderer* Renderer()const;

					VulkanRenderEngineInfo* EngineInfo()const;

					void Render(VulkanRenderEngine::CommandRecorder* commandRecorder);
				};

				virtual Reference<EngineData> CreateEngineData(VulkanRenderEngineInfo* engineInfo) = 0;

			protected:
				virtual void Render(EngineData* engineData, VulkanRenderEngine::CommandRecorder* commandRecorder) = 0;
			};
		}
	}
}
