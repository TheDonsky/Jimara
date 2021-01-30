#pragma once
#include "../VulkanDevice.h"
#include "../Memory/VulkanTexture.h"
#include "../Pipeline/VulkanCommandPool.h"
#include "../Pipeline/VulkanPipeline.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Information about Vulkan-backed render engine
			/// </summary>
			class VulkanRenderEngineInfo : public virtual RenderEngineInfo {
			public:
				/// <summary> Number of target images in engine (usually something like swap chain image count; useful for triple buffering info) </summary>
				virtual size_t ImageCount()const = 0;

				/// <summary>
				/// Target image by index
				/// </summary>
				/// <param name="imageId"> Image index [0 - ImageCount()) </param>
				/// <returns> Target image </returns>
				virtual VulkanImage* Image(size_t imageId)const = 0;

				/// <summary> Format of target images </summary>
				virtual VkFormat ImageFormat()const = 0;

				/// <summary>
				/// Number of MSAA samples, based on hardware, configuration and the render engine's specificity.
				/// </summary>
				/// <param name="desired"> Desired (configured) number of samples </param>
				/// <returns> Sample flag bits (Vulkan API format) </returns>
				virtual VkSampleCountFlagBits MSAASamples(GraphicsSettings::MSAA desired)const = 0;

				/// <summary> Device as vulkan device </summary>
				inline Vulkan::VulkanDevice* VulkanDevice()const { return dynamic_cast<Vulkan::VulkanDevice*>(Device()); }
			};

			/// <summary>
			/// Vulkan-backed render engine.
			/// Supports arbitrary renderers that output to images.
			/// </summary>
			class VulkanRenderEngine : public RenderEngine {
			public:
				/// <summary> "Owner" Vulkan device </summary>
				VulkanDevice* Device()const;

			protected:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> "Owner" vulkan device </param>
				VulkanRenderEngine(VulkanDevice* device);

			private:
				// "Owner" vulkan device
				Reference<VulkanDevice> m_device;
			};

			/// <summary>
			/// Vulkan-backed image renderer
			/// </summary>
			class VulkanImageRenderer : public ImageRenderer {
			public:
				/// <summary>
				/// Vulkan image renderer data tied to a apecific render engine.
				/// Notes: 
				///		0. VulkanImageRenderer is just supposed to hold logic and may "serve" to multiple render engines (more than one windows, for example).
				///		This serves as a parent class of data structure that holds RenderEngine-specific information;
				///		1. EngineData is created and stored by VulkanImageRenderer when the renderer gets added to it;
				///		2. It's highly advised that only the RenderEngine object holds a permanent reference to the EngineData, since only it knows, 
				///		when it's supposed to go out of scope, saty unmodified etc;
				///		3. One engine may arbitrarily re-request RenderData if, let's say, it's target images change or the logic deems the data to be no longer viable.
				/// </summary>
				class EngineData : public virtual Object {
				private:
					// Target renderer
					Reference<VulkanImageRenderer> m_renderer;

					// "Owner" engine information (EngineData lifecycle depends on VulkanRenderEngineInfo, 
					// so Reference<> here would create circular dependency and, thus, a memory leak, even if VulkanRenderEngineInfo inherited Object)
					VulkanRenderEngineInfo* m_engineInfo;

				public:
					/// <summary>
					/// Constructor
					/// </summary>
					/// <param name="renderer"> Target renderer </param>
					/// <param name="engineInfo"> "Owner" render engine information </param>
					EngineData(VulkanImageRenderer* renderer, VulkanRenderEngineInfo* engineInfo);

					/// <summary> Target renderer </summary>
					VulkanImageRenderer* Renderer()const;

					/// <summary> "Owner" render engine information </summary>
					VulkanRenderEngineInfo* EngineInfo()const;

					/// <summary>
					/// Requests command buffer recording (equivalent of Renderer()->Render(this, commandRecorder))
					/// </summary>
					/// <param name="commandRecorder"> Command recorder </param>
					void Render(VulkanCommandRecorder* commandRecorder);
				};

				/// <summary>
				/// Creates RenderEngine-specific data (normally requested exclusively by VulkanRenderEngine objects)
				/// </summary>
				/// <param name="engineInfo"> Render engine information </param>
				/// <returns> New instance of an EngineData object </returns>
				virtual Reference<EngineData> CreateEngineData(VulkanRenderEngineInfo* engineInfo) = 0;

			protected:
				/// <summary>
				/// Should record all rendering commands via commandRecorder
				/// </summary>
				/// <param name="engineData"> RenderEngine-specific data </param>
				/// <param name="commandRecorder"> Command recorder </param>
				virtual void Render(EngineData* engineData, VulkanCommandRecorder* commandRecorder) = 0;
			};
		}
	}
}
