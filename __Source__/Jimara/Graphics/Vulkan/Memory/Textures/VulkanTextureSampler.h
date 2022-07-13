#pragma once
#include "VulkanTextureView.h"


namespace Jimara {
	namespace Graphics {
		namespace Vulkan {
			/// <summary>
			/// Immutable wrapper on top of a VkSampler object
			/// </summary>
			class JIMARA_API VulkanTextureSampler : public virtual TextureSampler {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="view"> Image view </param>
				/// <param name="filtering"> Image filtering mode </param>
				/// <param name="wrapping"> Tells, how the image outside the bounds is sampled </param>
				/// <param name="lodBias"> Lod bias </param>
				/// <returns> New instance of a texture sampler </returns>
				VulkanTextureSampler(VulkanTextureView* view, FilteringMode filtering, WrappingMode wrapping, float lodBias);

				/// <summary> Virtual destructor </summary>
				virtual ~VulkanTextureSampler();

				/// <summary> Type cast to API object </summary>
				operator VkSampler()const;

				/// <summary> Image filtering mode </summary>
				virtual FilteringMode Filtering()const override;

				/// <summary> Tells, how the image outside the bounds is sampled </summary>
				virtual WrappingMode Wrapping()const override;

				/// <summary> Lod bias </summary>
				virtual float LodBias()const override;

				/// <summary> Texture view, this sampler "belongs" to </summary>
				virtual TextureView* TargetView()const override;


			private:
				// "Target" view
				const Reference<VulkanTextureView> m_view;

				// Image filtering mode
				const FilteringMode m_filtering;

				// Tells, how the image outside the bounds is sampled
				const WrappingMode m_wrapping;

				// Lod bias
				const float m_lodBias;

				// Underlying API object
				VkSampler m_sampler;
			};
		}
	}
}
