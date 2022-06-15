#pragma once
#include "../Component.h"
#include "../../Environment/Rendering/SceneObjects/Lights/LightDescriptor.h"


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

		// Underlying light descriptor
		Reference<LightDescriptor::Set::ItemOwner> m_lightDescriptor;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<DirectionalLight>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<DirectionalLight>(const Callback<const Object*>& report);
}