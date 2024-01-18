#pragma once
#include "../Scene/Scene.h"
#include <optional>


namespace Jimara {
	/// <summary>
	/// Some general information about the viewport
	/// </summary>
	enum class JIMARA_API RendererFrustrumFlags : uint64_t {
		/// <summary> Empty flags </summary>
		NONE = 0,

		/// <summary> If set, this means that the viewport is important enough for all graphics objects/lights to appear on it as soon as possible </summary>
		PRIMARY = (uint64_t(1u) << 0u),

		/// <summary> If set, this flag tells that the viewport is used by a shadowmapper (can be used to control if the items can cast shadows or not, for example) </summary>
		SHADOWMAPPER = (uint64_t(1u) << 1u)
	};


	/// <summary>
	/// Generic renderer frustrum descriptor; useful for culling and distance-checking, mostly
	/// </summary>
	class JIMARA_API RendererFrustrumDescriptor : public virtual Object {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="flags"> RendererFrustrumFlags </param>
		inline RendererFrustrumDescriptor(RendererFrustrumFlags flags = RendererFrustrumFlags::NONE) : m_flags(flags) {}

		/// <summary> Some general information about the viewport </summary>
		inline RendererFrustrumFlags Flags()const { return m_flags; }

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
		virtual Vector3 EyePosition()const = 0;

		/// <summary>
		/// RendererFrustrumDescriptor of the 'primary' viewport the scene is rendering to.
		/// By default, this is the same as this frustrum, but, let's say for shadowmappers, 
		/// this should refer to the camera viewport instead, to allow certain renderers to match LOD-s, for example.
		/// </summary>
		inline virtual const RendererFrustrumDescriptor* ViewportFrustrumDescriptor()const { return this; }

	private:
		// Flags
		const RendererFrustrumFlags m_flags;
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


	/// <summary>
	/// Logical 'or' between RendererFrustrumFlags
	/// </summary>
	/// <param name="a"> First bitamsk </param>
	/// <param name="b"> Second bitmask </param>
	/// <returns> a | b </returns>
	inline static RendererFrustrumFlags operator|(RendererFrustrumFlags a, RendererFrustrumFlags b) {
		return static_cast<RendererFrustrumFlags>(
			static_cast<std::underlying_type_t<RendererFrustrumFlags>>(a) | static_cast<std::underlying_type_t<RendererFrustrumFlags>>(b));
	}

	/// <summary>
	/// Logical 'and' between RendererFrustrumFlags
	/// </summary>
	/// <param name="a"> First bitamsk </param>
	/// <param name="b"> Second bitmask </param>
	/// <returns> a & b </returns>
	inline static RendererFrustrumFlags operator&(RendererFrustrumFlags a, RendererFrustrumFlags b) {
		return static_cast<RendererFrustrumFlags>(
			static_cast<std::underlying_type_t<RendererFrustrumFlags>>(a) & static_cast<std::underlying_type_t<RendererFrustrumFlags>>(b));
	}
}
