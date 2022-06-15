#pragma once
#include "VulkanTextureSampler.h"
#include "../../../../Core/Synch/SpinLock.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Sampler for arbitrary vulkan image view
			/// </summary>
			class JIMARA_API VulkanDynamicTextureSampler : public virtual VulkanImageSampler {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="view"> Image view </param>
				/// <param name="filtering"> Image filtering mode </param>
				/// <param name="wrapping"> Tells, how the image outside the bounds is sampled </param>
				/// <param name="lodBias"> Lod bias </param>
				/// <returns> New instance of a texture sampler </returns>
				VulkanDynamicTextureSampler(VulkanImageView* view, FilteringMode filtering, WrappingMode wrapping, float lodBias);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanDynamicTextureSampler();

				/// <summary> Image filtering mode </summary>
				virtual FilteringMode Filtering()const override;

				/// <summary> Tells, how the image outside the bounds is sampled </summary>
				virtual WrappingMode Wrapping()const override;

				/// <summary> Lod bias </summary>
				virtual float LodBias()const override;

				/// <summary> Texture view, this sampler "belongs" to </summary>
				virtual TextureView* TargetView()const override;

				/// <summary>
				/// Access immutable handle to VkSampler
				/// </summary>
				/// <param name="commandBuffer"> Command buffer thar may rely on the resource </param>
				/// <returns> Reference to the sampler </returns>
				virtual Reference<VulkanStaticImageSampler> GetStaticHandle(VulkanCommandBuffer* commandBuffer) override;


			private:
				// "Target" view
				const Reference<VulkanImageView> m_view;

				// Image filtering mode
				const FilteringMode m_filtering;

				// Tells, how the image outside the bounds is sampled
				const WrappingMode m_wrapping;

				// Lod bias
				const float m_lodBias;

				// Spinlock for sampler
				SpinLock m_samplerSpin;

				// Underlying API object
				Reference<VulkanStaticImageSampler> m_sampler;

				// View reference protection
				std::mutex m_samplerLock;
			};
		}
	}
}
