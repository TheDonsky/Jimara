#pragma once
#include "../Component.h"
#include "../../Environment/Rendering/SceneObjects/Lights/LightDescriptor.h"
#include "../Camera.h"


namespace Jimara {
	/// <summary> This will make sure, Component is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::DirectionalLight);

	/// <summary>
	/// Directional light component
	/// </summary>
	class JIMARA_API DirectionalLight : public virtual Component {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Name of the light component </param>
		/// <param name="color"> Light color </param>
		DirectionalLight(Component* parent, const std::string_view& name = "DirectionalLight", Vector3 color = Vector3(1.0f, 1.0f, 1.0f));

		/// <summary> Virtual destructor </summary>
		virtual ~DirectionalLight();

		/// <summary> Directional light color </summary>
		Vector3 Color()const;

		/// <summary>
		/// Sets light color
		/// </summary>
		/// <param name="color"> New color </param>
		void SetColor(Vector3 color);

		/// <summary> Optionally, the spotlight projection can take color form this texture </summary>
		inline Graphics::TextureSampler* Texture()const { return m_projectedTexture; }

		/// <summary>
		/// Sets projected texture
		/// </summary>
		/// <param name="texture"> Texture for coloring spot non-uniformally (can be nullptr) </param>
		inline void SetTexture(Graphics::TextureSampler* texture) { m_projectedTexture = texture; }

		/// <summary> Projection texture tiling (ignored if there is no texture) </summary>
		inline Vector2 TextureTiling()const { return m_projectedTextureTiling; }

		/// <summary>
		/// Sets projected texture tiling
		/// </summary>
		/// <param name="tiling"> Tells, how many times should the texture repeat itself </param>
		inline void SetTextureTiling(const Vector2& tiling) { m_projectedTextureTiling = tiling; }

		/// <summary> Projection texture offset (ignored if there is no texture) </summary>
		inline Vector2 TextureOffset()const { return m_projectedTextureOffset; }

		/// <summary>
		/// Sets projected texture offset
		/// </summary>
		/// <param name="tiling"> Tells, how to shift the texture around </param>
		inline void SetTextureOffset(const Vector2& offset) { m_projectedTextureOffset = offset; }

		/// <summary> Resolution of the shadow (0 means no shadows) </summary>
		inline uint32_t ShadowResolution()const { return m_shadowResolution; }

		/// <summary>
		/// Sets the resolution of the shadow
		/// </summary>
		/// <param name="resolution"> Shadow resolution (0 means no shadows) </param>
		inline void SetShadowResolution(uint32_t resolution) { m_shadowResolution = resolution; }

		/// <summary> Tells, how soft the cast shadow is </summary>
		inline float ShadowSoftness()const { return m_shadowSoftness; }

		/// <summary>
		/// Sets shadow softness
		/// </summary>
		/// <param name="softness"> Softness (valid range is 0.0f to 1.0f) </param>
		inline void SetShadowSoftness(float softness) { m_shadowSoftness = Math::Min(Math::Max(0.0f, softness), 1.0f); }

		/// <summary> Tells, what size kernel is used for rendering soft shadows </summary>
		inline uint32_t ShadowFilterSize()const { return m_shadowSampleCount; }

		/// <summary>
		/// Sets blur filter size
		/// </summary>
		/// <param name="filterSize"> Gaussian filter size (odd numbers from 1 to 65 are allowed) </param>
		inline void SetShadowFilterSize(uint32_t filterSize) { m_shadowSampleCount = Math::Min(filterSize, 65u) | 1u; }

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

	protected:
		/// <summary> Invoked, whenever the component becomes active in herarchy </summary>
		virtual void OnComponentEnabled()override;

		/// <summary> Invoked, whenever the component stops being active in herarchy </summary>
		virtual void OnComponentDisabled()override;

	private:
		// Set of all lights from the scene
		const Reference<LightDescriptor::Set> m_allLights;

		// Light color
		Vector3 m_color;

		// [Temporary] Camera reference and shadow range
		Reference<Camera> m_camera;
		float m_shadowRange = 100.0f;
		float m_depthEpsilon = 0.1f;

		// Projected texture
		Reference<Graphics::TextureSampler> m_projectedTexture;
		Vector2 m_projectedTextureTiling = Vector2(1.0f);
		Vector2 m_projectedTextureOffset = Vector2(0.0f);

		// Shadow texture
		uint32_t m_shadowResolution = 0u;
		Reference<JobSystem::Job> m_shadowRenderJob;
		Reference<Graphics::TextureSampler> m_shadowTexture;

		// Shadow softness
		float m_shadowSoftness = 0.5f;
		uint32_t m_shadowSampleCount = 5u;

		// Underlying light descriptor
		Reference<LightDescriptor::Set::ItemOwner> m_lightDescriptor;

		// Some private helper stuff here
		struct Helpers;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<DirectionalLight>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<DirectionalLight>(const Callback<const Object*>& report);
}