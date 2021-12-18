#include "ImGuiVulkanContext.h"
#include "ImGuiGLFWContext.h"
#include "ImGuiVulkanRenderer.h"

#pragma warning(disable: 26812)
#include <backends/imgui_impl_vulkan.h>
#pragma warning(default: 26812)


namespace Jimara {
	namespace Editor {
		ImGuiVulkanContext::ImGuiVulkanContext(Graphics::Vulkan::VulkanDevice* device, Graphics::Texture::PixelFormat frameFormat) : ImGuiDeviceContext(device)
			, m_renderPass([&]() -> Reference<Graphics::RenderPass> {
			return device->CreateRenderPass(Graphics::Texture::Multisampling::SAMPLE_COUNT_1, 1, &frameFormat, Graphics::Texture::PixelFormat::FORMAT_COUNT, false, false);
			}()) {
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
			{
				VkDescriptorPoolCreateInfo pool_info = {};
				pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
				pool_info.poolSizeCount = static_cast<uint32_t>((sizeof(POOL_SIZES) / sizeof(VkDescriptorPoolSize)));
				pool_info.maxSets = (MAX_DESCRIPTORS_PER_TYPE * pool_info.poolSizeCount);
				pool_info.pPoolSizes = POOL_SIZES;
				std::unique_lock<std::mutex> lock(device->PipelineCreationLock());
				if (vkCreateDescriptorPool(*device, &pool_info, nullptr, &m_descriptorPool) != VK_SUCCESS) {
					m_descriptorPool = VK_NULL_HANDLE;
					device->Log()->Fatal("ImGuiVulkanContext::ImGuiVulkanContext - vkCreateDescriptorPool() failed!");
					return;
				}
			}
			{
				ImGui_ImplVulkan_InitInfo init_info = {};

				init_info.Instance = *device->VulkanAPIInstance();
				init_info.PhysicalDevice = *device->PhysicalDeviceInfo();
				init_info.Device = *device;

				Graphics::Vulkan::VulkanDeviceQueue* graphicsQueue = dynamic_cast<Graphics::Vulkan::VulkanDeviceQueue*>(device->GraphicsQueue());
				init_info.QueueFamily = graphicsQueue->FamilyId();
				// Possibly unsafe:
				init_info.Queue = VK_NULL_HANDLE;
				vkGetDeviceQueue(init_info.Device, init_info.QueueFamily, 0, &init_info.Queue);
				if (init_info.Queue == VK_NULL_HANDLE) 
					device->Log()->Fatal("ImGuiVulkanContext::ImGuiVulkanContext - Could not retrieve graphics queue!");

				init_info.PipelineCache = VK_NULL_HANDLE;
				init_info.DescriptorPool = m_descriptorPool;
				init_info.Allocator = nullptr;
				init_info.MinImageCount = init_info.ImageCount = m_imageCount;

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
				m_vulkanContextInitialized = ImGui_ImplVulkan_Init(&init_info, *m_renderPass);
				if (!m_vulkanContextInitialized)
					device->Log()->Fatal("ImGuiVulkanContext::ImGuiVulkanContext - ImGui_ImplVulkan_Init() failed!");
			}
			{
				std::unique_lock<std::mutex> apiLock(ImGuiAPIContext::APILock());
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

		Reference<ImGuiWindowContext> ImGuiVulkanContext::CreateWindowContext(OS::Window* window) {
			if (window == nullptr) 
				return nullptr;
			{
				OS::GLFW_Window* glfwWindow = dynamic_cast<OS::GLFW_Window*>(window);
				if (glfwWindow != nullptr) return Object::Instantiate<ImGuiGLFWVulkanContext>(glfwWindow);
			}
			{
				if (window != nullptr)
					window->Log()->Error("ImGuiVulkanContext::CreateWindowContext - Unsupported window type!");
				return nullptr;
			}
		}

		Graphics::RenderPass* ImGuiVulkanContext::RenderPass()const {
			return m_renderPass;
		}
		
		//Graphics::Texture::PixelFormat ImGuiVulkanContext::FrameFormat() {
		//	return Graphics::Texture::PixelFormat::R16G16B16A16_SFLOAT;
		//}

		void ImGuiVulkanContext::SetImageCount(size_t imageCount) {
			if (m_imageCount < imageCount) {
				m_imageCount = (uint32_t)imageCount;
				ImGui_ImplVulkan_SetMinImageCount(m_imageCount);
			}
		}
	}
}
