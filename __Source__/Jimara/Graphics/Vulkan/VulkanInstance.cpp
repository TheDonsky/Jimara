#include "VulkanInstance.h"
#include "VulkanPhysicalDevice.h"
#include "Rendering/VulkanRenderSurface.h"
#include <unordered_set>
#include <sstream>
#include <cstring>

namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
#pragma warning(disable: 26812)
				// Main vulkan debug callback
				static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
					VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
					VkDebugUtilsMessageTypeFlagsEXT messageType,
					const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
					void* pUserData) {

					if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
						((VulkanInstance*)pUserData)->Log()->Log(
							messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT ? OS::Logger::LogLevel::LOG_DEBUG :
							messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT ? OS::Logger::LogLevel::LOG_INFO :
							messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT ? OS::Logger::LogLevel::LOG_WARNING :
							messageSeverity <= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ? OS::Logger::LogLevel::LOG_ERROR :
							OS::Logger::LogLevel::LOG_FATAL,
							std::string("Validation layer - ") + pCallbackData->pMessage);

					return VK_FALSE;
				}
#pragma warning(default: 26812)


				// Desired extensions
				static const char* JIMARA_DESIRED_EXTENSIONS[] = {
					VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
					VK_KHR_SURFACE_EXTENSION_NAME
#ifdef _WIN32
						, "VK_KHR_win32_surface"
#elif __APPLE__
						, "VK_EXT_metal_surface"
#else
						, "VK_KHR_xcb_surface"
#endif
#ifndef NDEBUG
						, VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#endif
				};

				// Number of desired extensions
#define JIMARA_DESIRED_EXTENSION_COUNT (sizeof(JIMARA_DESIRED_EXTENSIONS) / sizeof(const char*))


				// Application::AppInformation to VkApplicationInfo
				inline static VkApplicationInfo ApplicationInfo(const Application::AppInformation* appInfo) {
					VkApplicationInfo appInformation = {};
					appInformation.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;

					// Application:
					appInformation.pApplicationName = appInfo->ApplicationName();
					const Application::AppVersion appVersion = appInfo->ApplicationVersion();
					appInformation.applicationVersion = VK_MAKE_VERSION(appVersion.major, appVersion.minor, appVersion.patch);

					// Engine:
					appInformation.pEngineName = Application::AppInformation::EngineName();
					const Application::AppVersion engineVersion = Application::AppInformation::EngineVersion();
					appInformation.engineVersion = VK_MAKE_VERSION(engineVersion.major, engineVersion.minor, engineVersion.patch);

					// Vulkan API:
					appInformation.apiVersion = VK_API_VERSION_1_2;
					
					return appInformation;
				}

#ifndef NDEBUG
				// Logs available and used extensions, as well as the ones ignored by the engine
				inline static void EnumerateAvailableExtensions(OS::Logger* log) {
					uint32_t extensionCount = 0;
					vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
					std::vector<VkExtensionProperties> extensions(extensionCount);
					vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
					
					std::unordered_set<std::string> notFound;
					for (size_t i = 0; i < JIMARA_DESIRED_EXTENSION_COUNT; i++) 
						notFound.insert(JIMARA_DESIRED_EXTENSIONS[i]);

					log->Info("VulkanInstance::EnumerateAvailableExtensions - Enumerating available extensions...");
					for (size_t i = 0; i < extensions.size(); i++) {
						const VkExtensionProperties& extension = extensions[i];
						std::unordered_set<std::string>::iterator it = notFound.find(extension.extensionName);
						bool ignored;
						if (it != notFound.end()) {
							notFound.erase(it);
							ignored = false;
						}
						else ignored = true;
						auto getMessage = [&]() -> std::string {
							std::stringstream stream;
							stream << "VulkanInstance::EnumerateAvailableExtensions - Extension " 
								<< (ignored ? "ignored" : "found") << ": " << extension.extensionName << "(v." << extension.specVersion << ")";
							return stream.str();
						};
						if (ignored) log->Debug(getMessage());
						else log->Info(getMessage());
					}
					for (std::unordered_set<std::string>::iterator it = notFound.begin(); it != notFound.end(); ++it)
						log->Error("VulkanInstance::EnumerateAvailableExtensions - Extension missing: " + (*it));

					log->Info([&]() -> std::string {
						std::stringstream stream;
						stream << "VulkanInstance::EnumerateAvailableExtensions - Desired extensions found: " 
							<< (JIMARA_DESIRED_EXTENSION_COUNT - notFound.size()) << "; missing: " << notFound.size();
						return stream.str();
						}());
				}


				// Finds all available validation layers that matter
				static std::vector<const char*> GetValidationLayers(VulkanInstance* vulkanInstance) {
					static const char* VALIDATION_LAYERS[] = {
						"VK_LAYER_KHRONOS_validation"
					};
					static const size_t VALIDATION_LAYER_COUNT = (sizeof(VALIDATION_LAYERS) / sizeof(const char*));

					uint32_t layerCount;
					vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
					std::vector<VkLayerProperties> availableLayers(layerCount);
					vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

					std::vector<const char*> foundLayers;
					vulkanInstance->Log()->Info("Vulkan::VulkanInstance - Getting validation layers...");
					for (size_t i = 0; i < VALIDATION_LAYER_COUNT; i++) {
						const char* validationLayer = VALIDATION_LAYERS[i];
						bool found = false;
						for (size_t j = 0; j < availableLayers.size(); j++)
							if (strcmp(validationLayer, availableLayers[j].layerName) == 0) {
								availableLayers[j] = availableLayers.back();
								availableLayers.pop_back();
								found = true;
								break;
							}
						if (found) {
							foundLayers.push_back(validationLayer);
							vulkanInstance->Log()->Info(std::string("VulkanInstance::GetValidationLayers - Validation layer found: ") + validationLayer);
						}
						else vulkanInstance->Log()->Warning(std::string("VulkanInstance::GetValidationLayers - Validation layer missing: ") + validationLayer);
					}
					for (size_t i = 0; i < availableLayers.size(); i++)
						vulkanInstance->Log()->Debug(std::string("VulkanInstance::GetValidationLayers - Validation layer ignored: ") + availableLayers[i].layerName);
					vulkanInstance->Log()->Info([&]() -> std::string {
						std::stringstream stream;
						stream << "VulkanInstance::GetValidationLayers - Validation layers found: " << foundLayers.size() << "; missing: " << (VALIDATION_LAYER_COUNT - foundLayers.size());
						return stream.str();
						}());
					return foundLayers;
				}
#endif



				// Debug messenger create info
				inline static VkDebugUtilsMessengerCreateInfoEXT DebugMessengerCreateInfo(VulkanInstance* vulkanInstance) {
					VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
					createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
					createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
					createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
					createInfo.pfnUserCallback = VulkanDebugCallback;
					createInfo.pUserData = vulkanInstance;
					return createInfo;
				}



				// Creates a VkInstance
				inline static VkInstance CreateVulkanInstance(VulkanInstance* vulkanInstance, std::vector<const char*>& validationLayers) {
					// Application info:
					VkApplicationInfo appInformation = ApplicationInfo(vulkanInstance->AppInfo());
					
					// Extensions:
#ifndef NDEBUG
					EnumerateAvailableExtensions(vulkanInstance->Log());
#endif

					// Create info:
					VkInstanceCreateInfo createInfo{};
					createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
					createInfo.pApplicationInfo = &appInformation;
					createInfo.enabledExtensionCount = static_cast<uint32_t>(JIMARA_DESIRED_EXTENSION_COUNT);
					createInfo.ppEnabledExtensionNames = JIMARA_DESIRED_EXTENSIONS;
#ifdef NDEBUG
					validationLayers.clear();
#else
					validationLayers = GetValidationLayers(vulkanInstance);
#endif
					createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
					createInfo.ppEnabledLayerNames = validationLayers.size() > 0 ? validationLayers.data() : nullptr;
					VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = DebugMessengerCreateInfo(vulkanInstance);
					createInfo.pNext = validationLayers.size() > 0 ? (&debugMessengerCreateInfo) : nullptr;


					// VKInstance creation:
					VkInstance instance = VK_NULL_HANDLE;
					if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
						instance = VK_NULL_HANDLE;
						vulkanInstance->Log()->Fatal("VulkanInstance::CreateVulkanInstance - Failed to create instance");
					}
					return instance;
				}


				// Creates a VkDebugUtilsMessengerEXT
				inline static VkDebugUtilsMessengerEXT CreateDebugMessenger(VulkanInstance* vulkanInstance) {
					VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
					// We only need to create debug messenger if validation layers are active:
					if (vulkanInstance->ActiveValidationLayers().size() > 0) {
						VkDebugUtilsMessengerCreateInfoEXT createInfo = DebugMessengerCreateInfo(vulkanInstance);

						VkInstance instance = (*vulkanInstance);
						if (instance == nullptr)
							vulkanInstance->Log()->Fatal("VulkanInstance::CreateDebugMessenger - Can not create debug messenger without VKInstance");

						PFN_vkCreateDebugUtilsMessengerEXT createMessengerFunction = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
						if (createMessengerFunction == nullptr)
							vulkanInstance->Log()->Fatal("VulkanInstance::CreateDebugMessenger - Failed to retrieve debug messenger creation function");
						else if (createMessengerFunction(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
							debugMessenger = VK_NULL_HANDLE;
							vulkanInstance->Log()->Fatal("VulkanInstance::CreateDebugMessenger - Failed to create debug messenger");
						}
					}
					return debugMessenger;
				}

				// Collects physical device information
				inline static void CollectPhysicalDevices(
					VulkanInstance* vulkanInstance, std::vector<std::unique_ptr<PhysicalDevice>>& deviceInfos, 
					VulkanPhysicalDevice*(CreateVulkanPhysicalDevice)(VulkanInstance*, VkPhysicalDevice, size_t)) {
					VkInstance instance = (*vulkanInstance);
					if (instance == VK_NULL_HANDLE)
						vulkanInstance->Log()->Fatal("VulkanInstance::CollectPhysicalDevices - Can not collect physical devices without VKInstance");

					uint32_t deviceCount = 0;
					vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
					if (deviceCount <= 0)
						vulkanInstance->Log()->Fatal("VulkanInstance::CollectPhysicalDevices - No GPU with vulkan support present on the system");
					std::vector<VkPhysicalDevice> devices(deviceCount);
					vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

#ifndef NDEBUG
					std::stringstream stream;
					stream << "Vulkan::VulkanInstance::CollectPhysicalDevices:" << std::endl;
#endif
					for (size_t i = 0; i < devices.size(); i++) {
						VkPhysicalDevice device = devices[i];
						VulkanPhysicalDevice* deviceInfo = CreateVulkanPhysicalDevice(vulkanInstance, device, i);
						deviceInfos.push_back(std::unique_ptr<PhysicalDevice>(deviceInfo));
#ifndef NDEBUG
						stream << "    DEVICE " << i << ": " << deviceInfo->Name() << " {"
							<< (deviceInfo->Type() == PhysicalDevice::DeviceType::CPU ? "CPU" :
								deviceInfo->Type() == PhysicalDevice::DeviceType::INTEGRATED ? "INTEGRATED" :
								deviceInfo->Type() == PhysicalDevice::DeviceType::DESCRETE ? "DESCRETE" :
								deviceInfo->Type() == PhysicalDevice::DeviceType::VIRTUAL ? "VIRTUAL" : "OTHER")
							<< "; [graphics-" << (deviceInfo->HasFeature(PhysicalDevice::DeviceFeature::GRAPHICS) ? "YES" : "NO")
							<< "; compute-" << (deviceInfo->HasFeature(PhysicalDevice::DeviceFeature::COMPUTE) ? "YES" : "NO")
							<< "; synch_compute-" << (deviceInfo->HasFeature(PhysicalDevice::DeviceFeature::SYNCHRONOUS_COMPUTE) ? "YES" : "NO")
							<< "; asynch_compute-" << (deviceInfo->HasFeature(PhysicalDevice::DeviceFeature::ASYNCHRONOUS_COMPUTE) ? "YES" : "NO")
							<< "; swap_chain-" << (deviceInfo->HasFeature(PhysicalDevice::DeviceFeature::SWAP_CHAIN)? "YES" : "NO") << "]"
							<< "; VRAM:" << deviceInfo->VramCapacity() << " bytes"
							<< "}" << std::endl;
#endif
					}
#ifndef NDEBUG
					vulkanInstance->Log()->Debug(stream.str());
#endif
				}
			}



			VulkanInstance::VulkanInstance(OS::Logger* logger, const Application::AppInformation* appInfo)
				: GraphicsInstance(logger, appInfo), m_instance(VK_NULL_HANDLE), m_debugMessenger(VK_NULL_HANDLE) {
				m_instance = CreateVulkanInstance(this, m_validationLayers);
				m_debugMessenger = CreateDebugMessenger(this);
				CollectPhysicalDevices(this, m_physicalDevices,
					[](VulkanInstance* instance, VkPhysicalDevice device, size_t index) -> VulkanPhysicalDevice* {
						return new VulkanPhysicalDevice(instance, device, index);
					});
			}

			VulkanInstance::~VulkanInstance() {
				// Cleanup physical devices:
				if (m_physicalDevices.size() > 0) {
					for (size_t i = 0; i < m_physicalDevices.size(); i++)
						m_physicalDevices[i].reset();
					m_physicalDevices.clear();
				}

				// Destroy debug messenger (if it exists):
				if (m_debugMessenger != VK_NULL_HANDLE) {
					PFN_vkDestroyDebugUtilsMessengerEXT destroyFunction = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
					if (destroyFunction != VK_NULL_HANDLE) destroyFunction(m_instance, m_debugMessenger, nullptr);
					m_debugMessenger = VK_NULL_HANDLE;
				}

				// Destroy API instance:
				if (m_instance != VK_NULL_HANDLE) {
					vkDestroyInstance(m_instance, nullptr);
					m_instance = VK_NULL_HANDLE;
				}
			}

			size_t VulkanInstance::PhysicalDeviceCount()const {
				return m_physicalDevices.size(); 
			}

			PhysicalDevice* VulkanInstance::GetPhysicalDevice(size_t index)const {
				return index < m_physicalDevices.size() ? m_physicalDevices[index].get() : nullptr;
			}

			Reference<RenderSurface> VulkanInstance::CreateRenderSurface(OS::Window* window) {
				return Object::Instantiate<VulkanWindowSurface>(this, window);
			}

			VulkanInstance::operator VkInstance()const {
				return m_instance;
			}

			const std::vector<const char*>& VulkanInstance::ActiveValidationLayers()const {
				return m_validationLayers;
			}
		}
	}
}
