#include "RenderImages.h"


namespace Jimara {
	RenderImages::RenderImages(Graphics::GraphicsDevice* device, Size2 resolution, Graphics::Texture::Multisampling sampleCount)
		: m_device(device), m_resolution(resolution), m_sampleCount(min(sampleCount, device->PhysicalDevice()->MaxMultisapling())) {}

	RenderImages::~RenderImages() {}

	RenderImages::Image* RenderImages::GetImage(const ImageId* imageId) {
		if (imageId == nullptr) {
			m_device->Log()->Error("RenderImages::GetImage - Null imageId provided!");
			return nullptr;
		}

		std::unique_lock<std::mutex> lock(m_imageLock);
		{
			auto it = m_images.find(imageId);
			if (it != m_images.end())
				return it->second;
		}

		const Reference<Graphics::Texture> resolveTexture = m_device->CreateTexture(
			Graphics::Texture::TextureType::TEXTURE_2D, imageId->Format(), Size3(m_resolution, 1), 1, false);
		if (resolveTexture == nullptr) {
			m_device->Log()->Error("RenderImages::GetImage - Failed to create resolve texture! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		const Reference<Graphics::TextureView> resolveView = resolveTexture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
		if (resolveView == nullptr) {
			m_device->Log()->Error("RenderImages::GetImage - Failed to create resolve texture view! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		const Reference<Graphics::Texture> multisampledTexture = [&]() -> Reference<Graphics::Texture> {
			if (m_sampleCount == Graphics::Texture::Multisampling::SAMPLE_COUNT_1) return resolveTexture;
			else return m_device->CreateMultisampledTexture(Graphics::Texture::TextureType::TEXTURE_2D, imageId->Format(), Size3(m_resolution, 1), 1, m_sampleCount);
		}();
		if (multisampledTexture == nullptr) {
			m_device->Log()->Error("RenderImages::GetImage - Failed to create multisapled texture! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		const Reference<Graphics::TextureView> multisapledView = [&]() ->Reference<Graphics::TextureView> {
			if (resolveTexture == multisampledTexture) return resolveView;
			else multisampledTexture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
		}();
		if (multisapledView == nullptr) {
			m_device->Log()->Error("RenderImages::GetImage - Failed to create multisampled texture view! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		Reference<Image> image = new Image(multisapledView, resolveView);
		image->ReleaseRef();
		m_images[imageId] = image;
		return image;
	}

	const RenderImages::ImageId* RenderImages::MainColor() {
		static const ImageId id(Graphics::Texture::PixelFormat::R16G16B16A16_SINT);
		return &id;
	}

	const RenderImages::ImageId* RenderImages::DepthBuffer() {
		static const ImageId id(Graphics::Texture::PixelFormat::D32_SFLOAT);
		return &id;
	}
}
