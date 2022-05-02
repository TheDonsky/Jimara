#pragma once
#include "../Scene/Scene.h"


namespace Jimara {
	class RenderImages : public virtual Object {
	public:
		class ImageId;
		class Image;

		RenderImages(Graphics::GraphicsDevice* device, Size2 resolution, Graphics::Texture::Multisampling sampleCount);

		virtual ~RenderImages();

		Image* GetImage(const ImageId* imageId);

		inline Size2 Resolution()const { return m_resolution; }

		inline Graphics::Texture::Multisampling SampleCount()const { return m_sampleCount; }

		static const ImageId* MainColor();

		static const ImageId* DepthBuffer();

		class ImageId : public virtual Object {
		public:
			inline ImageId(Graphics::Texture::PixelFormat format) : m_pixelFormat(format) {};

			inline Graphics::Texture::PixelFormat Format()const { return m_pixelFormat; }

		private:
			const Graphics::Texture::PixelFormat m_pixelFormat;
		};

		class Image : public virtual Object {
		public:
			inline virtual ~Image() {}

			inline Graphics::TextureView* Multisampled()const { return m_multisampledAttachment; }

			inline Graphics::TextureView* Resolve()const { m_resolveAttachmemt; }

			inline bool IsMultisampled()const { return m_multisampledAttachment == m_resolveAttachmemt; }

		private:
			const Reference<Graphics::TextureView> m_multisampledAttachment;
			const Reference<Graphics::TextureView> m_resolveAttachmemt;

			inline Image(Graphics::TextureView* multisampledAttachment, Graphics::TextureView* resolveAttachmemt)
				: m_multisampledAttachment(multisampledAttachment), m_resolveAttachmemt(resolveAttachmemt) {}

			friend class RenderImages;
		};

	private:
		const Reference<Graphics::GraphicsDevice> m_device;
		const Size2 m_resolution;
		const Graphics::Texture::Multisampling m_sampleCount;
		std::mutex m_imageLock;
		std::unordered_map<Reference<const ImageId>, Reference<Image>> m_images;
	};
}
