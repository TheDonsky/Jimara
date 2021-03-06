#include "VulkanDynamicTextureSampler.h"


#pragma warning(disable: 26812)
namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			VulkanDynamicTextureSampler::VulkanDynamicTextureSampler(VulkanImageView* view, FilteringMode filtering, WrappingMode wrapping, float lodBias)
				: m_view(view), m_filtering(filtering), m_wrapping(wrapping), m_lodBias(lodBias), m_sampler(nullptr) {}

			VulkanDynamicTextureSampler::~VulkanDynamicTextureSampler() {}

			TextureSampler::FilteringMode VulkanDynamicTextureSampler::Filtering()const {
				return m_filtering;
			}

			TextureSampler::WrappingMode VulkanDynamicTextureSampler::Wrapping()const {
				return m_wrapping;
			}

			float VulkanDynamicTextureSampler::LodBias()const {
				return m_lodBias;
			}

			TextureView* VulkanDynamicTextureSampler::TargetView()const {
				return m_view;
			}

			Reference<VulkanStaticImageSampler> VulkanDynamicTextureSampler::GetStaticHandle(VulkanCommandBuffer* commandBuffer) {
				Reference<VulkanStaticImageView> view = m_view->GetStaticHandle(commandBuffer);
				Reference<VulkanStaticImageSampler> sampler;
				{
					std::unique_lock<SpinLock> samplerLock(m_samplerSpin);
					sampler = m_sampler;
				}
				if (sampler == nullptr || view != sampler->TargetView()) {
					std::unique_lock<std::mutex> lock(m_samplerLock);
					view = m_view->GetStaticHandle(commandBuffer);
					sampler = view->CreateSampler(m_filtering, m_wrapping, m_lodBias);
					std::unique_lock<SpinLock> samplerLock(m_samplerSpin);
					m_sampler = sampler;
				}
				commandBuffer->RecordBufferDependency(sampler);
				return sampler;
			}
		}
	}
}
#pragma warning(default: 26812)
