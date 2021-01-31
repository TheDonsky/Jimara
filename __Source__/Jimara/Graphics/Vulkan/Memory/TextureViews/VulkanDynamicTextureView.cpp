#include "VulkanDynamicTextureView.h"
#include "../TextureSamplers/VulkanDynamicTextureSampler.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanDynamicTextureView::VulkanDynamicTextureView(
				VulkanImage* image, ViewType viewType
				, uint32_t baseMipLevel, uint32_t mipLevelCount
				, uint32_t baseArrayLayer, uint32_t arrayLayerCount)
				: m_image(image), m_viewType(viewType)
				, m_baseMipLevel(min(image->MipLevels(), baseMipLevel)), m_mipLevelCount(min(image->MipLevels() - min(image->MipLevels(), baseMipLevel), mipLevelCount))
				, m_baseArrayLayer(min(image->ArraySize(), baseArrayLayer)), m_arrayLayerCount(min(image->ArraySize() - min(image->ArraySize(), baseArrayLayer), arrayLayerCount)) { }

			VulkanDynamicTextureView::~VulkanDynamicTextureView() { }

			TextureView::ViewType VulkanDynamicTextureView::Type()const {
				return m_viewType;
			}

			Texture* VulkanDynamicTextureView::TargetTexture()const {
				return m_image;
			}

			uint32_t VulkanDynamicTextureView::BaseMipLevel()const {
				return m_baseMipLevel;
			}

			uint32_t VulkanDynamicTextureView::MipLevelCount()const {
				return m_mipLevelCount;
			}

			uint32_t VulkanDynamicTextureView::BaseArrayLayer()const {
				return m_baseArrayLayer;
			}

			uint32_t VulkanDynamicTextureView::ArrayLayerCount()const {
				return m_arrayLayerCount;
			}

			Reference<VulkanStaticImageView> VulkanDynamicTextureView::GetStaticHandle(VulkanCommandRecorder* commandRecorder) {
				Reference<VulkanStaticImage> image = m_image->GetStaticHandle(commandRecorder);
				Reference<VulkanStaticImageView> view = m_view;
				if (view == nullptr || image != view->TargetTexture()) {
					std::unique_lock<std::mutex> lock(m_viewLock);
					image = m_image->GetStaticHandle(commandRecorder);
					view = image->CreateView(m_viewType, m_baseMipLevel, m_mipLevelCount, m_baseArrayLayer, m_arrayLayerCount);
					m_view = view;
				}
				commandRecorder->RecordBufferDependency(view);
				return view;
			}

			Reference<TextureSampler> VulkanDynamicTextureView::CreateSampler(TextureSampler::FilteringMode filtering, TextureSampler::WrappingMode wrapping, float lodBias) {
				return Object::Instantiate<VulkanDynamicTextureSampler>(this, filtering, wrapping, lodBias);
			}
		}
	}
}
#pragma warning(default: 26812)
