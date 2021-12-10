#pragma once
#include "../Component.h"
#include "../../Environment/GraphicsContext/Lights/LightDescriptor.h"


namespace Jimara {
	/// <summary> This will make sure, Component is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::PointLight);

	/// <summary>
	/// Point light component
	/// </summary>
	class PointLight : public virtual Component {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Name of the light component </param>
		/// <param name="color"> Point light color </param>
		/// <param name="radius"> Light radius </param>
		PointLight(Component* parent, const std::string& name, Vector3 color = Vector3(1.0f, 1.0f, 1.0f), float radius = 100.0f);

		/// <summary> Virtual destructor </summary>
		virtual ~PointLight();

		/// <summary> Component serializer </summary>
		virtual Reference<const ComponentSerializer> GetSerializer()const override;

		/// <summary> Point light color </summary>
		Vector3 Color()const;

		/// <summary>
		/// Sets light color
		/// </summary>
		/// <param name="color"> New color </param>
		void SetColor(Vector3 color);

		/// <summary> Illuminated sphere radius </summary>
		float Radius()const;

		/// <summary>
		/// Sets the radius of the illuminated area
		/// </summary>
		/// <param name="radius"> New radius </param>
		void SetRadius(float radius);


	private:
		// Light color
		Vector3 m_color;

		// Light area radius
		float m_radius;

		// Underlying graphics descriptor
		Reference<LightDescriptor> m_lightDescriptor;

		// Removes from graphics scene when destroyed
		void RemoveWhenDestroyed(Component*);
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<PointLight>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> void TypeIdDetails::OnRegisterType<PointLight>();
	template<> void TypeIdDetails::OnUnregisterType<PointLight>();
}
