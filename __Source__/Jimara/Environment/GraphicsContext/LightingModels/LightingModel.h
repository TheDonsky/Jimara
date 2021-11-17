#pragma once
namespace Jimara { class LightingModel; }
#include "../../../Graphics/Data/ShaderBinaries/ShaderLoader.h"
#include "../GraphicsContext.h"

namespace Jimara {
	/// <summary>
	/// Generic interface, each scene renderer is supposed to implement;
	/// Basically, responsible for turning the data from GraphicsContext into images.
	/// </summary>
	class LightingModel : public virtual Object {
	public:
		/// <summary>
		/// Render viewport descriptor
		/// </summary>
		class ViewportDescriptor : public virtual Object {
		private:
			// Graphics context, the viewport is tied to
			const Reference<GraphicsContext> m_graphicsContext;

		public:
			/// <summary> View matrix </summary>
			virtual Matrix4 ViewMatrix()const = 0;

			/// <summary>
			/// Projection matrix
			/// </summary>
			/// <param name="aspect"> Aspect ratio of the target image (Renderer can operate on multiple target images, so this should be able to adjust on the fly) </param>
			/// <returns> Projection matrix </returns>
			virtual Matrix4 ProjectionMatrix(float aspect)const = 0;

			/// <summary> Color, the frame buffer should be cleared with before rendering the image </summary>
			virtual Vector4 ClearColor()const = 0;

			/// <summary> Graphics context, the viewport is tied to </summary>
			inline GraphicsContext* Context()const { return m_graphicsContext; }

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Graphics context, the viewport is tied to </param>
			inline ViewportDescriptor(GraphicsContext* context) : m_graphicsContext(context) {}
		};

		/// <summary>
		/// Creates a scene renderer based on the viewport
		/// </summary>
		/// <param name="viewport"> Viewport descriptor </param>
		/// <returns> New instance of a renderer if successful, nullptr otherwise </returns>
		virtual Reference<Graphics::ImageRenderer> CreateRenderer(const ViewportDescriptor* viewport) = 0;
	};
}
