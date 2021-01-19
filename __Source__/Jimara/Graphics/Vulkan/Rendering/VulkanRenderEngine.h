#pragma once
#include "../VulkanDevice.h"
#include "../Memory/VulkanTexture.h"


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

				/// <summary>
				/// Number of MSAA samples, based on hardware, configuration and the render engine's specificity.
				/// </summary>
				/// <param name="desired"> Desired (configured) number of samples </param>
				/// <returns> Sample flag bits (Vulkan API format) </returns>
				virtual VkSampleCountFlagBits MSAASamples(GraphicsSettings::MSAA desired)const = 0;
			};

			/// <summary>
			/// Vulkan-backed render engine.
			/// Supports arbitrary renderers that output to images.
			/// </summary>
			class VulkanRenderEngine : public RenderEngine {
			public:
				/// <summary>
				/// Command recorder.
				/// Note: When VulkanRenderEngine performs an update, 
				///		it requests all the underlying renderers to record their imege-specific commands 
				///		to a pre-constructed command buffer from the render engine.
				/// </summary>
				class CommandRecorder {
				public:
					/// <summary> Virtual destructor </summary>
					inline virtual ~CommandRecorder() {}

					/// <summary> Target image index </summary>
					virtual size_t ImageIndex()const = 0;

					/// <summary> Target image (same as VulkanRenderEngineInfo::Image(ImageIndex())) </summary>
					virtual VulkanImage* Image()const = 0;

					/// <summary> Command buffer to record to </summary>
					virtual VkCommandBuffer CommandBuffer()const = 0;

					/// <summary> 
					/// If there are resources that should stay alive during command buffer execution, but might otherwise go out of scope,
					/// user can record those in a kind of a set that will delay their destruction till buffer execution ends using this call.
					/// </summary>
					virtual void RecordBufferDependency(Object* object) = 0;
				};

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
					void Render(VulkanRenderEngine::CommandRecorder* commandRecorder);
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
				virtual void Render(EngineData* engineData, VulkanRenderEngine::CommandRecorder* commandRecorder) = 0;
			};
		}
	}
}
