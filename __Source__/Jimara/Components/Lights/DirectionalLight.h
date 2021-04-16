#pragma once
#include "../Component.h"
#include "../../Environment/GraphicsContext/LightDescriptor.h"


namespace Jimara {
	class DirectionalLight : public virtual Component {
	public:
		DirectionalLight(Component* parent, const std::string& name, Vector3 color = Vector3(1.0f, 1.0f, 1.0f));

		virtual ~DirectionalLight();


		Vector3 GetColor()const;

		void SetColor(Vector3 color);


	private:
		Vector3 m_color;

		Reference<LightDescriptor> m_lightDescriptor;

		void RemoveWhenDestroyed(Component*);
	};
}