#pragma once
#include "../Scene/Scene.h"
#include <optional>


namespace Jimara {
	/// <summary>
	/// Render viewport descriptor
	/// </summary>
	class JIMARA_API ViewportDescriptor : public virtual Object {
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

		/// <summary> Color, the frame buffer should be cleared with before rendering the image (if renderer does not clear, this value may be ignored) </summary>
		virtual Vector4 ClearColor()const = 0;

		/// <summary> Context, the viewport is tied to </summary>
		inline SceneContext* Context()const { return m_context; }

	protected:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="context"> Context, the viewport is tied to </param>
		inline ViewportDescriptor(SceneContext* context) : m_context(context) {}
	};
}
