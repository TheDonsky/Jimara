#include "ImGuiVulkanContext.h"
#include "ImGuiGLFWContext.h"
#include "ImGuiVulkanRenderer.h"
#include <Math/Math.h>

#pragma warning(disable: 26812)
#include <backends/imgui_impl_vulkan.h>
#pragma warning(default: 26812)


namespace Jimara {
	namespace Editor {
		namespace {
			inline static Reference<ImGuiWindowContext> CreateWindowContext(OS::Window* window) {
				if (window == nullptr)
					return nullptr;
				{
					OS::GLFW_Window* glfwWindow = dynamic_cast<OS::GLFW_Window*>(window);
					if (glfwWindow != nullptr) return Object::Instantiate<ImGuiGLFWVulkanContext>(glfwWindow);
				}
				{
					if (window != nullptr)
						window->Log()->Fatal("ImGuiVulkanContext::CreateWindowContext - Unsupported window type!");
					return nullptr;
				}
			}

			inline static bool InitializeVulkanContext(Graphics::Vulkan::VulkanDevice* device, const Graphics::RenderEngineInfo* renderEngineInfo
				, Reference<Graphics::Vulkan::VulkanRenderPass>& renderPass, VkDescriptorPool& descriptorPool, bool& glfwVulkanInitialized, uint32_t& imageCount) {
				if (renderPass == nullptr) {
					const Graphics::Texture::PixelFormat format = renderEngineInfo->ImageFormat();
					renderPass = device->CreateRenderPass(Graphics::Texture::Multisampling::SAMPLE_COUNT_1, 1, &format, Graphics::Texture::PixelFormat::FORMAT_COUNT, false, false);
					if (renderPass == nullptr) {
						device->Log()->Error("ImGuiVulkanContext::InitializeVulkanContext - Failed to create VulkanRenderPass!");
						return false;
					}
				}
				static const uint32_t MAX_DESCRIPTORS_PER_TYPE = 1000;
				static const VkDescriptorPoolSize POOL_SIZES[] =
				{
					{ VK_DESCRIPTOR_TYPE_SAMPLER, MAX_DESCRIPTORS_PER_TYPE },
					{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_DESCRIPTORS_PER_TYPE },
					{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_DESCRIPTORS_PER_TYPE },
					{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_DESCRIPTORS_PER_TYPE },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, MAX_DESCRIPTORS_PER_TYPE },
					{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, MAX_DESCRIPTORS_PER_TYPE },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_DESCRIPTORS_PER_TYPE },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_DESCRIPTORS_PER_TYPE },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, MAX_DESCRIPTORS_PER_TYPE },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, MAX_DESCRIPTORS_PER_TYPE },
					{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, MAX_DESCRIPTORS_PER_TYPE }
				};
				if (descriptorPool == VK_NULL_HANDLE) {
					VkDescriptorPoolCreateInfo pool_info = {};
					pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
					pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
					pool_info.poolSizeCount = static_cast<uint32_t>((sizeof(POOL_SIZES) / sizeof(VkDescriptorPoolSize)));
					pool_info.maxSets = (MAX_DESCRIPTORS_PER_TYPE * pool_info.poolSizeCount);
					pool_info.pPoolSizes = POOL_SIZES;
					std::unique_lock<std::mutex> lock(device->PipelineCreationLock());
					if (vkCreateDescriptorPool(*device, &pool_info, nullptr, &descriptorPool) != VK_SUCCESS) {
						descriptorPool = VK_NULL_HANDLE;
						device->Log()->Error("ImGuiVulkanContext::ImGuiVulkanContext - vkCreateDescriptorPool() failed!");
						return false;
					}
				}
				if (!glfwVulkanInitialized) {
					ImGui_ImplVulkan_InitInfo init_info = {};

					init_info.Instance = *device->VulkanAPIInstance();
					init_info.PhysicalDevice = *device->PhysicalDeviceInfo();
					init_info.Device = *device;

					Graphics::Vulkan::VulkanDeviceQueue* graphicsQueue = dynamic_cast<Graphics::Vulkan::VulkanDeviceQueue*>(device->GraphicsQueue());
					init_info.QueueFamily = graphicsQueue->FamilyId();
					// Possibly unsafe:
					init_info.Queue = VK_NULL_HANDLE;
					vkGetDeviceQueue(init_info.Device, init_info.QueueFamily, 0, &init_info.Queue);
					if (init_info.Queue == VK_NULL_HANDLE) {
						device->Log()->Error("ImGuiVulkanContext::ImGuiVulkanContext - Could not retrieve graphics queue!");
						return false;
					}

					init_info.PipelineCache = VK_NULL_HANDLE;
					init_info.DescriptorPool = descriptorPool;
					init_info.Allocator = nullptr;
					init_info.MinImageCount = init_info.ImageCount = imageCount = max(static_cast<uint32_t>(renderEngineInfo->ImageCount() * 2), imageCount);

					static OS::Logger* logger = nullptr;
					static std::mutex loggerMutex;
					init_info.CheckVkResultFn = [](VkResult result) {
						if (result != VK_SUCCESS)
							logger->Fatal("ImGuiVulkanContext::ImGuiVulkanContext - ImGui_ImplVulkan_Init failed! <err:", result, ">");
					};

					std::unique_lock<std::mutex> loggerLock(loggerMutex);
					logger = device->Log();
					std::unique_lock<std::mutex> lock(device->PipelineCreationLock());

					std::unique_lock<std::mutex> apiLock(ImGuiAPIContext::APILock());
					glfwVulkanInitialized = ImGui_ImplVulkan_Init(&init_info, *renderPass);
					if (!glfwVulkanInitialized) {
						device->Log()->Error("ImGuiVulkanContext::ImGuiVulkanContext - ImGui_ImplVulkan_Init() failed!");
						return false;
					}

					{
						Reference<Graphics::Vulkan::VulkanPrimaryCommandBuffer> commandBuffer = device->GraphicsQueue()->CreateCommandPool()->CreatePrimaryCommandBuffer();
						commandBuffer->BeginRecording();
						bool success = ImGui_ImplVulkan_CreateFontsTexture(*commandBuffer);
						commandBuffer->EndRecording();
						device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
						commandBuffer->Wait();
						ImGui_ImplVulkan_DestroyFontUploadObjects();
						if (!success)
							device->Log()->Fatal("ImGuiVulkanContext::ImGuiVulkanContext - ImGui_ImplVulkan_CreateFontsTexture() failed!");
					}
				}
				return true;
			}
		}

		ImGuiVulkanContext::ImGuiVulkanContext(Graphics::Vulkan::VulkanDevice* device, OS::Window* window) 
			: ImGuiDeviceContext(device), m_windowContext(CreateWindowContext(window)) {}

		ImGuiVulkanContext::~ImGuiVulkanContext() {
			if (vkDeviceWaitIdle(*dynamic_cast<Graphics::Vulkan::VulkanDevice*>(GraphicsDevice())) != VK_SUCCESS)
				GraphicsDevice()->Log()->Error("ImGuiVulkanContext::~ImGuiVulkanContext - vkDeviceWaitIdle(*m_device) failed!");
			if (m_vulkanContextInitialized) {
				std::unique_lock<std::mutex> apiLock(ImGuiAPIContext::APILock());
				ImGui_ImplVulkan_Shutdown();
				m_vulkanContextInitialized = false;
			}
			if (m_descriptorPool != VK_NULL_HANDLE) {
				vkDestroyDescriptorPool(*dynamic_cast<Graphics::Vulkan::VulkanDevice*>(GraphicsDevice()), m_descriptorPool, nullptr);
				m_descriptorPool = VK_NULL_HANDLE;
			}
		}

		Reference<ImGuiRenderer> ImGuiVulkanContext::CreateRenderer(const Graphics::RenderEngineInfo* renderEngineInfo) {
			if (!InitializeVulkanContext(dynamic_cast<Graphics::Vulkan::VulkanDevice*>(GraphicsDevice()), renderEngineInfo,
				m_renderPass, m_descriptorPool, m_vulkanContextInitialized, m_imageCount)) return nullptr;
			else return Object::Instantiate<ImGuiVulkanRenderer>(this, m_windowContext, renderEngineInfo);
		}

		Graphics::RenderPass* ImGuiVulkanContext::RenderPass()const {
			return m_renderPass;
		}

		void ImGuiVulkanContext::SetImageCount(size_t imageCount) {
			if (m_imageCount < imageCount) {
				m_imageCount = (uint32_t)imageCount;
				ImGui_ImplVulkan_SetMinImageCount(m_imageCount);
			}
		}
	}
}
