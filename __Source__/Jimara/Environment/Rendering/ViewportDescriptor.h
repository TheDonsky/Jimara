#pragma once
#include "../Scene/Scene.h"
#include <optional>


namespace Jimara {
	/// <summary>
	/// Generic renderer frustrum descriptor; useful for culling and distance-checking, mostly
	/// </summary>
	class JIMARA_API RendererFrustrumDescriptor : public virtual Object {
	public:
		/// <summary>
		/// Frustrum transform
		/// <para/> Note: For normal ViewportDescriptors, this is the same as ProjectionMatrix() * ViewMatrix(), 
		///	but as far as the renderers are concerned, this represents a virtual frustrum, objects within which will be visible in the output. 
		/// (Useful for frustrum culling; for any point p, FrustrumTransform() * Vector4(p, 1.0) divided by the resulting w value will give us a clip-space coordinate, 
		/// which can easily tell us if the point is visible or not, when checked against ((-1.0f, -1.0f, 0.0f) - (1.0f, 1.0f, 1.0f)) box).
		/// </summary>
		virtual Matrix4 FrustrumTransform()const = 0;

		/// <summary>
		/// Renderer 'camera position' in world-space
		/// <para/> Note: For normal ViewportDescriptors, this is just derived camera positionfrom inverse view matrix,
		/// but generally speaking, for non-standard renderers, this will always be the world-space center/origin point.
		/// (Can be used for distance-based LOD-s, for example)
		/// </summary>
		inline virtual Vector3 EyePosition()const = 0;
	};



	/// <summary>
	/// Render viewport descriptor
	/// </summary>
	class JIMARA_API ViewportDescriptor : public virtual RendererFrustrumDescriptor {
	private:
		// Context, the viewport is tied to
		const Reference<SceneContext> m_context;

	public:
		/// <summary> View matrix </summary>
		virtual Matrix4 ViewMatrix()const = 0;

		/// <summary> Projection matrix </summary>
		virtual Matrix4 ProjectionMatrix()const = 0;

		/// <summary> Color, the frame buffer should be cleared with before rendering the image (if renderer does not clear, this value may be ignored) </summary>
		virtual Vector4 ClearColor()const = 0;

		/// <summary> Context, the viewport is tied to </summary>
		inline SceneContext* Context()const { return m_context; }

		/// <summary>
		/// Frustrum transform
		/// <para/> Note: For normal ViewportDescriptors, this is the same as ProjectionMatrix() * ViewMatrix(), 
		///	but as far as the renderers are concerned, this represents a virtual frustrum, objects within which will be visible in the output. 
		/// (Useful for frustrum culling; for any point p, FrustrumTransform() * Vector4(p, 1.0) divided by the resulting w value will give us a clip-space coordinate, 
		/// which can easily tell us if the point is visible or not, when checked against ((-1.0f, -1.0f, 0.0f) - (1.0f, 1.0f, 1.0f)) box).
		/// </summary>
		inline virtual Matrix4 FrustrumTransform()const override { return ProjectionMatrix() * ViewMatrix(); }

		/// <summary>
		/// Renderer 'camera position' in world-space
		/// <para/> Note: For normal ViewportDescriptors, this is just derived camera positionfrom inverse view matrix,
		/// but generally speaking, for non-standard renderers, this will always be the world-space center/origin point.
		/// (Can be used for distance-based LOD-s, for example)
		/// </summary>
		inline virtual Vector3 EyePosition()const override { return Math::Inverse(ViewMatrix())[3u]; }

	protected:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="context"> Context, the viewport is tied to </param>
		inline ViewportDescriptor(SceneContext* context) : m_context(context) {}
	};
}
