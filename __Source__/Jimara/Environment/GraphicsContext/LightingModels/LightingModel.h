#pragma once
namespace Jimara { class LightingModel; }
#include "../../../Graphics/Data/ShaderBinaries/ShaderLoader.h"
#include "../../Rendering/RenderStack.h"
#include <optional>

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
			// Context, the viewport is tied to
			const Reference<SceneContext> m_context;

		public:
			/// <summary> View matrix </summary>
			virtual Matrix4 ViewMatrix()const = 0;

			/// <summary>
			/// Projection matrix
			/// </summary>
			/// <param name="aspect"> Aspect ratio of the target image (Renderer can operate on multiple target images, so this should be able to adjust on the fly) </param>
			/// <returns> Projection matrix </returns>
			virtual Matrix4 ProjectionMatrix(float aspect)const = 0;

			/// <summary> Color, the frame buffer should be cleared with before rendering the image (if there's no value, 'Do not clear' assumed) </summary>
			virtual std::optional<Vector4> ClearColor()const = 0;

			/// <summary> Context, the viewport is tied to </summary>
			inline SceneContext* Context()const { return m_context; }

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="context"> Context, the viewport is tied to </param>
			inline ViewportDescriptor(SceneContext* context) : m_context(context) {}
		};

		/// <summary>
		/// Creates a scene renderer based on the viewport
		/// </summary>
		/// <param name="viewport"> Viewport descriptor </param>
		/// <returns> New instance of a renderer if successful, nullptr otherwise </returns>
		virtual Reference<RenderStack::Renderer> CreateRenderer(const ViewportDescriptor* viewport) = 0;
	};
}
