#pragma once
#include "../Component.h"
#include "../../Environment/Rendering/SceneObjects/Lights/LightmapperJobs.h"


namespace Jimara {
	/// <summary> This will make sure, Component is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::PointLight);

	/// <summary>
	/// Point light component
	/// </summary>
	class JIMARA_API PointLight : public virtual Component {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Name of the light component </param>
		/// <param name="color"> Point light color </param>
		/// <param name="radius"> Light radius </param>
		PointLight(Component* parent, const std::string_view& name = "PointLight", Vector3 color = Vector3(1.0f, 1.0f, 1.0f), float radius = 100.0f);

		/// <summary> Virtual destructor </summary>
		virtual ~PointLight();

		/// <summary> Point light color </summary>
		Vector3 Color()const;

		/// <summary>
		/// Sets light color
		/// </summary>
		/// <param name="color"> New color </param>
		void SetColor(Vector3 color);

		/// <summary> Color multiplier </summary>
		inline float Intensity()const { return m_intensity; }

		/// <summary>
		/// Sets intensity
		/// </summary>
		/// <param name="intensity"> Color multiplier </param>
		inline void SetIntensity(float intensity) { m_intensity = Math::Max(intensity, 0.0f); }

		/// <summary> Illuminated sphere radius </summary>
		float Radius()const;

		/// <summary>
		/// Sets the radius of the illuminated area
		/// </summary>
		/// <param name="radius"> New radius </param>
		void SetRadius(float radius);

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
		
		/// <summary> Shadow distance from viewport origin, before it starts fading </summary>
		inline float ShadowDistance()const { return m_shadowDistance; }

		/// <summary>
		/// Sets shadow distance
		/// </summary>
		/// <param name="distance"> Shadow distance from viewport origin, before it starts fading </param>
		inline void SetShadowDistance(float distance) { m_shadowDistance = Math::Max(distance, 0.0f); }

		/// <summary> Shadow fade-out distance after ShadowDistance, before it fully disapears </summary>
		inline float ShadowFadeDistance()const { return m_shadowFadeDistance; }

		/// <summary>
		/// Sets shadow fade distance
		/// </summary>
		/// <param name="distance"> Shadow fade-out distance after ShadowDistance, before it fully disapears </param>
		inline void SetShadowFadeDistance(float distance) { m_shadowFadeDistance = Math::Max(distance, 0.0f); }

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

		// Color multipler
		float m_intensity = 1.0f;

		// Light area radius
		float m_radius;

		// Underlying light descriptor
		Reference<LightDescriptor::Set::ItemOwner> m_lightDescriptor;

		// Shadow settings
		uint32_t m_shadowResolution = 0u;
		float m_shadowDistance = 20.0f;
		float m_shadowFadeDistance = 10.0f;

		// Shadow softness
		float m_shadowSoftness = 0.5f;
		uint32_t m_shadowSampleCount = 5u;

		// Some private stuff resides here...
		struct Helpers;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<PointLight>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<PointLight>(const Callback<const Object*>& report);
}
