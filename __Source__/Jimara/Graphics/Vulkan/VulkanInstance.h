#pragma once
#include "VulkanAPIIncludes.h"
#include "../GraphicsInstance.h"
#include <vector>
#include <memory>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// VULkan API backend instance
			/// </summary>
			class JIMARA_API VulkanInstance : public GraphicsInstance {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="logger"> Logger </param>
				/// /// <param name="appInfo"> Basic info abot the application </param>
				VulkanInstance(OS::Logger* logger, const Application::AppInformation* appInfo);

				/// <summary> Destructor </summary>
				virtual ~VulkanInstance();

				/// <summary> Number of available physical devices </summary>
				virtual size_t PhysicalDeviceCount()const override;

				/// <summary>
				/// Physical device by index 
				/// </summary>
				/// <param name="index"> Physical device index </param>
				/// <returns> Physical device </returns>
				virtual PhysicalDevice* GetPhysicalDevice(size_t index)const override;

				/// <summary>
				/// Creates a render surface from a window
				/// </summary>
				/// <param name="window"> Terget window </param>
				/// <returns> New instance of a render surface </returns>
				virtual Reference<RenderSurface> CreateRenderSurface(OS::Window* window) override;

				/// <summary> Cast to native Vulkan API instance </summary>
				operator VkInstance()const;

				/// <summary>
				/// Validation layers that are active 
				/// (Supposed to be non-empty for debug builds only)
				/// </summary>
				const std::vector<const char*>& ActiveValidationLayers()const;


			private:
				// UNDerlying API Instance
				VkInstance m_instance;

				// Validation layers that are active
				std::vector<const char*> m_validationLayers;

				// Debug messenger (should exist only in debug builds)
				VkDebugUtilsMessengerEXT m_debugMessenger;

				// Available physical devices (note, that their lifetime is securely tied to that of the Instance itself)
				std::vector<std::unique_ptr<PhysicalDevice>> m_physicalDevices;
			};
		}
	}
}
