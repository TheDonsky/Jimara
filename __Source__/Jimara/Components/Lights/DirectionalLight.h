#pragma once
#include "../Component.h"
#include "../../Environment/GraphicsContext/Lights/LightDescriptor.h"


namespace Jimara {
	/// <summary>
	/// Directional light component
	/// </summary>
	class DirectionalLight : public virtual Component {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Name of the light component </param>
		/// <param name="color"> Light color </param>
		DirectionalLight(Component* parent, const std::string& name, Vector3 color = Vector3(1.0f, 1.0f, 1.0f));

		/// <summary> Virtual destructor </summary>
		virtual ~DirectionalLight();

		/// <summary> Component serializer </summary>
		virtual Reference<const ComponentSerializer> GetSerializer()const override;

		/// <summary> Directional light color </summary>
		Vector3 Color()const;

		/// <summary>
		/// Sets light color
		/// </summary>
		/// <param name="color"> New color </param>
		void SetColor(Vector3 color);


	private:
		// Light color
		Vector3 m_color;

		// Underlying graphics descriptor
		Reference<LightDescriptor> m_lightDescriptor;

		// Removes from graphics scene when destroyed
		void RemoveWhenDestroyed(Component*);
	};
}