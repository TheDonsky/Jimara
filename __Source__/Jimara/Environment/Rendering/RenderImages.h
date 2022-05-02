#pragma once
#include "../Scene/Scene.h"


namespace Jimara {
	/// <summary>
	/// Collection of shared images, that can be used one after another by multiple renderers, for example, from RenderStack
	/// <para/> Note: These come in handy, when, let's say, we have a camera, then postFX, 
	///		then overlay and all of those need more than just the final color output of the previous one (like depth or normals)
	/// </summary>
	class RenderImages : public virtual Object {
	public:
		/// <summary>
		/// Unique identifier of an image within RenderImages (can be used as a key and you will normally have a bounch of singletons)
		/// <para/> Note: Stuff like MainColor and DepthBuffer are defined here, but feel free to add more keys anywhere if your renderers require those
		/// </summary>
		class ImageId : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="format"> Image format </param>
			inline ImageId(Graphics::Texture::PixelFormat format) : m_pixelFormat(format) {};

			/// <summary> Image format </summary>
			inline Graphics::Texture::PixelFormat Format()const { return m_pixelFormat; }

		private:
			// Image format
			const Graphics::Texture::PixelFormat m_pixelFormat;
		};


		/// <summary>
		/// Pair of multisampled and resolve (single sample) images
		/// </summary>
		class Image : public virtual Object {
		public:
			/// <summary> Virtual destructor </summary>
			inline virtual ~Image() {}

			/// <summary> Multisampled image view (same as Resolve() if RenderImages' sample count is 1) </summary>
			inline Graphics::TextureView* Multisampled()const { return m_multisampledAttachment; }

			/// <summary> Non-multisampled image view, that can be used as resolve attachment of various passes (or regular attachment when there's no multisampling) </summary>
			inline Graphics::TextureView* Resolve()const { return m_resolveAttachmemt; }

			/// <summary> True, if sample count is not 1 (same as Multisampled() != Resolve()) </summary>
			inline bool IsMultisampled()const { return m_multisampledAttachment != m_resolveAttachmemt; }

		private:
			// Multisampled image view
			const Reference<Graphics::TextureView> m_multisampledAttachment;

			// Non-multisampled image view
			const Reference<Graphics::TextureView> m_resolveAttachmemt;

			// Constructor
			inline Image(Graphics::TextureView* multisampledAttachment, Graphics::TextureView* resolveAttachmemt)
				: m_multisampledAttachment(multisampledAttachment), m_resolveAttachmemt(resolveAttachmemt) {}

			// Only RenderImages can create instances
			friend class RenderImages;
		};

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="device"> Graphics device </param>
		/// <param name="resolution"> Image resolution (all contained images will share this) </param>
		/// <param name="sampleCount"> Multisampling (msaa) </param>
		RenderImages(Graphics::GraphicsDevice* device, Size2 resolution, Graphics::Texture::Multisampling sampleCount);

		/// <summary> Virtual destructor </summary>
		virtual ~RenderImages();

		/// <summary>
		/// Gets images for given ImageId
		/// </summary>
		/// <param name="imageId"> Unique image identifier object </param>
		/// <returns> Image, mapped to given id </returns>
		Image* GetImage(const ImageId* imageId);

		/// <summary> Image set resolution </summary>
		inline Size2 Resolution()const { return m_resolution; }

		/// <summary> Sample count of multisampled images </summary>
		inline Graphics::Texture::Multisampling SampleCount()const { return m_sampleCount; }

		/// <summary>
		/// Constant Image identifier for the main color output 
		/// <para/> Note: This will be used by most presenters (swap chain, Editor views, etc) as the result of a RenderStack render process.
		/// </summary>
		static const ImageId* MainColor();

		/// <summary>
		/// Constant Image identifier for the main depth buffer
		/// <para/> Note: This will be useful if we have several viewports rendering different layers and we need common depth buffers to properly merge images.
		/// </summary>
		static const ImageId* DepthBuffer();

		
	private:
		// Graphics device
		const Reference<Graphics::GraphicsDevice> m_device;

		// Image resolution
		const Size2 m_resolution;

		// Sample count
		const Graphics::Texture::Multisampling m_sampleCount;

		// Lock for image collection
		std::mutex m_imageLock;

		// Image collection
		std::unordered_map<Reference<const ImageId>, Reference<Image>> m_images;
	};
}
