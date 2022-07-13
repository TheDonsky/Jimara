#include "VulkanTextureSampler.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			namespace {
				inline static VkFilter VulkanFilter(TextureSampler::FilteringMode filter) {
					static const uint8_t FILTER_COUNT = static_cast<uint8_t>(TextureSampler::FilteringMode::FILTER_COUNT);
					static const VkFilter* FILTERS = []() -> VkFilter* {
						static VkFilter filters[FILTER_COUNT];
						for (uint8_t i = 0; i < FILTER_COUNT; i++) filters[i] = VK_FILTER_MAX_ENUM;
						filters[static_cast<uint8_t>(TextureSampler::FilteringMode::NEAREST)] = VK_FILTER_NEAREST;
						filters[static_cast<uint8_t>(TextureSampler::FilteringMode::LINEAR)] = VK_FILTER_LINEAR;
						return filters;
					}();
					return (filter < TextureSampler::FilteringMode::FILTER_COUNT) ? FILTERS[static_cast<uint8_t>(filter)] : VK_FILTER_MAX_ENUM;
				}

				inline static VkSamplerAddressMode VulkanAddressMode(TextureSampler::WrappingMode mode) {
					static const uint8_t WRAPPING_COUNT = static_cast<uint8_t>(TextureSampler::WrappingMode::MODE_COUNT);
					static const VkSamplerAddressMode* MODES = []() -> VkSamplerAddressMode* {
						static VkSamplerAddressMode modes[WRAPPING_COUNT];
						for (uint8_t i = 0; i < WRAPPING_COUNT; i++) modes[i] = VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
						modes[static_cast<uint8_t>(TextureSampler::WrappingMode::REPEAT)] = VK_SAMPLER_ADDRESS_MODE_REPEAT;
						modes[static_cast<uint8_t>(TextureSampler::WrappingMode::MIRRORED_REPEAT)] = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
						modes[static_cast<uint8_t>(TextureSampler::WrappingMode::CLAMP_TO_EDGE)] = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
						modes[static_cast<uint8_t>(TextureSampler::WrappingMode::CLAMP_TO_BORDER)] = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
						return modes;
					}();
					return (mode < TextureSampler::WrappingMode::MODE_COUNT) ? MODES[static_cast<uint8_t>(mode)] : VK_SAMPLER_ADDRESS_MODE_MAX_ENUM;
				}
			}

			VulkanTextureSampler::VulkanTextureSampler(VulkanTextureView* view, FilteringMode filtering, WrappingMode wrapping, float lodBias)
				: m_view(view), m_filtering(filtering), m_wrapping(wrapping), m_lodBias(lodBias), m_sampler(VK_NULL_HANDLE) {
				VkSamplerCreateInfo samplerInfo = {};
				{
					samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

					VkFilter filter = VulkanFilter(m_filtering);
					samplerInfo.magFilter = filter;
					samplerInfo.minFilter = filter;

					VkSamplerAddressMode addressMode = VulkanAddressMode(m_wrapping);
					samplerInfo.addressModeU = addressMode;
					samplerInfo.addressModeV = addressMode;
					samplerInfo.addressModeW = addressMode;

					samplerInfo.anisotropyEnable = dynamic_cast<VulkanImage*>(m_view->TargetTexture())->Device()->PhysicalDeviceInfo()->DeviceFeatures().samplerAnisotropy;
					samplerInfo.maxAnisotropy = samplerInfo.anisotropyEnable ? 
						dynamic_cast<VulkanImage*>(m_view->TargetTexture())->Device()->PhysicalDeviceInfo()->DeviceProperties().limits.maxSamplerAnisotropy : 1;

					samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;

					samplerInfo.unnormalizedCoordinates = VK_FALSE;

					samplerInfo.compareEnable = VK_FALSE;
					samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

					samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
					samplerInfo.mipLodBias = m_lodBias;
					samplerInfo.minLod = 0.0f;
					samplerInfo.maxLod = static_cast<float>(m_view->MipLevelCount());
				}
				if (vkCreateSampler(*dynamic_cast<VulkanImage*>(m_view->TargetTexture())->Device(), &samplerInfo, nullptr, &m_sampler) != VK_SUCCESS)
					dynamic_cast<VulkanImage*>(m_view->TargetTexture())->Device()->Log()->Fatal("VulkanImageSampler - Failed to create texture sampler!");
			}

			VulkanTextureSampler::~VulkanTextureSampler() {
				if (m_sampler != VK_NULL_HANDLE) {
					vkDestroySampler(*dynamic_cast<VulkanImage*>(m_view->TargetTexture())->Device(), m_sampler, nullptr);
					m_sampler = VK_NULL_HANDLE;
				}
			}

			TextureSampler::FilteringMode VulkanTextureSampler::Filtering()const {
				return m_filtering;
			}

			TextureSampler::WrappingMode VulkanTextureSampler::Wrapping()const {
				return m_wrapping;
			}

			float VulkanTextureSampler::LodBias()const {
				return m_lodBias;
			}

			TextureView* VulkanTextureSampler::TargetView()const {
				return m_view;
			}

			VulkanTextureSampler::operator VkSampler()const {
				return m_sampler;
			}
		}
	}
}
#pragma warning(default: 26812)
