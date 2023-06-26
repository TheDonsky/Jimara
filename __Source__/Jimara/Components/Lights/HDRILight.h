#pragma once
#include "../Component.h"
#include "../../Environment/Rendering/SceneObjects/Lights/LightDescriptor.h"
#include "../../Environment/Rendering/ImageBasedLighting/HDRIEnvironment.h"
#include "../Camera.h"


namespace Jimara {
	/// <summary> This will make sure, Component is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::HDRILight);

	/// <summary>
	/// Light source from an HDRI environment map
	/// <para/> Note: HDRILight uses HDRIEnvironment and is only well-suited for PBR surface model; 
	/// non-PBR materials may produce strange visuals.
	/// </summary>
	class JIMARA_API HDRILight : public virtual Component {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		HDRILight(Component* parent, const std::string_view& name = "HDRILight");

		/// <summary> Virtual destructor </summary>
		virtual ~HDRILight();

		/// <summary> Base color </summary>
		inline Vector3 Color()const { return m_color; }

		/// <summary>
		/// Sets base color
		/// </summary>
		/// <param name="color"> Color multiplier </param>
		inline void SetColor(Vector3 color) { m_color = color; }

		/// <summary> Color multiplier </summary>
		inline float Intensity()const { return m_intensity; }

		/// <summary>
		/// Sets intensity
		/// </summary>
		/// <param name="intensity"> Color multiplier </param>
		inline void SetIntensity(float intensity) { m_intensity = Math::Max(intensity, 0.0f); }

		/// <summary> Environment HDRI texture </summary>
		inline HDRIEnvironment* Texture()const { return m_hdriEnvironment; }

		/// <summary>
		/// Sets environment texture
		/// </summary>
		/// <param name="texture"> HDRI environment </param>
		inline void SetTexture(HDRIEnvironment* texture) { m_hdriEnvironment = texture; }

		/// <summary> Camera (if set, skybox will be rendered) </summary>
		inline Jimara::Camera* Camera()const { return m_camera; }

		/// <summary>
		/// Sets Camera for skybox rendering
		/// </summary>
		/// <param name="camera"> Camera </param>
		void SetCamera(Jimara::Camera* camera);

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

		// Color multiplier
		Vector3 m_color = Vector3(1.0f);

		// Color intensity
		float m_intensity = 1.0f;

		// Environment HDRI textures
		Reference<HDRIEnvironment> m_hdriEnvironment;

		// Camera
		Reference<Jimara::Camera> m_camera;

		// Underlying light descriptor
		Reference<LightDescriptor::Set::ItemOwner> m_lightDescriptor;

		// Private stuff resides in here
		struct Helpers;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<HDRILight>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<HDRILight>(const Callback<const Object*>& report);
}
