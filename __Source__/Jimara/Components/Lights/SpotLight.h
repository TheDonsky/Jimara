#pragma once
#include "../Component.h"
#include "../../Environment/Rendering/SceneObjects/Lights/LightDescriptor.h"


namespace Jimara {
	/// <summary> This will make sure, Component is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::SpotLight);

	/// <summary> Spot light component </summary>
	class JIMARA_API SpotLight : public virtual Component {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		SpotLight(Component* parent, const std::string_view& name = "SpotLight");

		/// <summary> Virtual destructor </summary>
		virtual ~SpotLight();

		/// <summary> Maximal distance, the SpotLight will illuminate at </summary>
		inline float Range()const { return m_range; }

		/// <summary>
		/// Sets SpotLight range
		/// </summary>
		/// <param name="range"> Maximal distance, a SpotLight will illuminate at </param>
		inline void SetRange(float range) { m_range = Math::Max(range, 0.0001f); }

		/// <summary> Projection cone angle, before the intencity starts fading out </summary>
		inline float InnerAngle()const { return m_innerAngle; }

		/// <summary>
		/// Sets inner angle of the spotlight
		/// </summary>
		/// <param name="angle"> Projection cone angle, before the intencity starts fading out </param>
		inline void SetInnerAngle(float angle) { 
			m_innerAngle = Math::Min(Math::Max(0.0f, angle), 90.0f);
			if (m_outerAngle < m_innerAngle) m_outerAngle = m_innerAngle;
		}

		/// <summary> Projection cone angle at which the intencity will drop to zero </summary>
		inline float OuterAngle()const { return m_outerAngle; }

		/// <summary>
		/// Sets outer angle of the spotlight
		/// </summary>
		/// <param name="angle"> Projection cone angle at which the intencity will drop to zero </param>
		inline void SetOuterAngle(float angle) {
			m_outerAngle = Math::Min(Math::Max(0.0f, angle), 90.0f);
			if (m_outerAngle < m_innerAngle) m_innerAngle = m_outerAngle;
		}

		/// <summary> Base color of spotlight emission </summary>
		inline Vector3 Color()const { return m_baseColor; }

		/// <summary>
		/// Sets spotlight color
		/// </summary>
		/// <param name="color"> Projection color </param>
		inline void SetColor(Vector3 color) { m_baseColor = color; }

		/// <summary> Color multiplier </summary>
		inline float Intensity()const { return m_intensity; }

		/// <summary>
		/// Sets intensity
		/// </summary>
		/// <param name="intensity"> Color multiplier </param>
		inline void SetIntensity(float intensity) { m_intensity = Math::Max(intensity, 0.0f); }

		/// <summary> Optionally, the spotlight projection can take color form this texture </summary>
		inline Graphics::TextureSampler* Texture()const { return m_projectedTexture; }

		/// <summary>
		/// Sets projected texture
		/// </summary>
		/// <param name="texture"> Texture for coloring spot non-uniformally (can be nullptr) </param>
		inline void SetTexture(Graphics::TextureSampler* texture) { m_projectedTexture = texture; }

		/// <summary> Resolution of the shadow (0 means no shadows) </summary>
		inline uint32_t ShadowResolution()const { return m_shadowResolution; }

		/// <summary>
		/// Sets the resolution of the shadow
		/// </summary>
		/// <param name="resolution"> Shadow resolution (0 means no shadows) </param>
		inline void SetShadowResolution(uint32_t resolution) { m_shadowResolution = resolution; }

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

		// Maximal range of the spotlight
		float m_range = 10.0f;

		// 'Inner' spot angle
		float m_innerAngle = 30.0f;

		// 'Outer' spot angle (fade angle)
		float m_outerAngle = 45.0f;

		// Base color
		Vector3 m_baseColor = Vector3(1.0f);

		// Color multipler
		float m_intensity = 1.0f;

		// Projected texture
		Reference<Graphics::TextureSampler> m_projectedTexture;

		// Underlying light descriptor
		Reference<LightDescriptor::Set::ItemOwner> m_lightDescriptor;

		// Shadow texture
		uint32_t m_shadowResolution = 512;
		Reference<JobSystem::Job> m_shadowRenderJob;
		Reference<Graphics::TextureSampler> m_shadowTexture;

		// Some private stuff resides here...
		struct Helpers;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<SpotLight>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<SpotLight>(const Callback<const Object*>& report);
}
