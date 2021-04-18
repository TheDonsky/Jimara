#pragma once
#include "../Component.h"
#include "../../Environment/GraphicsContext/Lights/LightDescriptor.h"


namespace Jimara {
	class PointLight : public virtual Component {
	public:
		PointLight(Component* parent, const std::string& name, Vector3 color = Vector3(1.0f, 1.0f, 1.0f), float radius = 100.0f);

		virtual ~PointLight();


		Vector3 GetColor()const;

		void SetColor(Vector3 color);


		float GetRadius()const;

		void SetRadius(float radius);


	private:
		Vector3 m_color;
		float m_radius;

		Reference<LightDescriptor> m_lightDescriptor;

		void RemoveWhenDestroyed(Component*);
	};
}
