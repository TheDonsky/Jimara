#pragma once
#include "../Component.h"
#include "../../Environment/Rendering/SceneObjects/Lights/LightmapperJobs.h"
#include "LocalLightShadowSettings.h"


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
		void SetColor(const Vector3& color);

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

		/// <summary> Shadow settings provider </summary>
		inline Reference<LocalLightShadowSettingsProvider> GetShadowSettings()const { return m_shadowSettings; }

		/// <summary>
		/// Sets shadow settings provider
		/// </summary>
		/// <param name="provider"> Shadow Settings input </param>
		inline void SetShadowSettings(LocalLightShadowSettingsProvider* provider) { m_shadowSettings = provider; }

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		/// <summary>
		/// Reports actions associated with the component.
		/// </summary>
		/// <param name="report"> Actions will be reported through this callback </param>
		virtual void GetSerializedActions(Callback<Serialization::SerializedCallback> report)override;


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
		WeakReference<LocalLightShadowSettingsProvider> m_shadowSettings;
		const Reference<LocalLightShadowSettings> m_defaultShadowSettings = Object::Instantiate<LocalLightShadowSettings>();

		// Some private stuff resides here...
		struct Helpers;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<PointLight>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<PointLight>(const Callback<const Object*>& report);
}
