#include "PointLight.h"
#include "../Transform.h"


namespace Jimara {
	namespace {
		class PointLightDescriptor : public virtual LightDescriptor, public virtual GraphicsContext::GraphicsObjectSynchronizer {
		public:
			const PointLight* m_owner;

		private:
			struct Data {
				alignas(16) Vector3 position;
				alignas(16) Vector3 color;
			} m_data;

			float m_radius;

			LightInfo m_info;

			void UpdateData() {
				if (m_owner == nullptr) return;
				const Transform* transform = m_owner->GetTransfrom();
				if (transform == nullptr) m_data.position = Vector3(0.0f, 0.0f, 0.0f);
				else m_data.position = transform->WorldPosition();
				m_data.color = m_owner->GetColor();
				m_radius = m_owner->GetRadius();
			}

		public:
			inline PointLightDescriptor(const PointLight* owner, uint32_t typeId) : m_owner(owner), m_info{} {
				UpdateData();
				m_info.typeId = typeId;
				m_info.data = &m_data;
				m_info.dataSize = sizeof(Data);
			}

			virtual LightInfo GetLightInfo()const override { return m_info; }

			virtual AABB GetLightBounds()const override {
				AABB bounds = {};
				bounds.start = m_data.position - Vector3(m_radius, m_radius, m_radius);
				bounds.end = m_data.position + Vector3(m_radius, m_radius, m_radius);
				return bounds;
			}

			virtual void OnGraphicsSynch() override { UpdateData(); }
		};
	}

	PointLight::PointLight(Component* parent, const std::string& name, Vector3 color, float radius)
		: Component(parent, name), m_color(color), m_radius(radius) {
		uint32_t typeId;
		if (Context()->Graphics()->GetLightTypeId("Jimara_PointLight", typeId))
			m_lightDescriptor = Object::Instantiate<PointLightDescriptor>(this, typeId);
		Context()->Graphics()->AddSceneLightDescriptor(m_lightDescriptor);
		OnDestroyed() += Callback<Component*>(&PointLight::RemoveWhenDestroyed, this);
	}

	PointLight::~PointLight() {
		OnDestroyed() -= Callback<Component*>(&PointLight::RemoveWhenDestroyed, this);
		RemoveWhenDestroyed(this); 
	}


	Vector3 PointLight::GetColor()const { return m_color; }

	void PointLight::SetColor(Vector3 color) { m_color = color; }


	float PointLight::GetRadius()const { return m_radius; }

	void PointLight::SetRadius(float radius) { m_radius = radius <= 0.0f ? 0.0f : radius; }


	void PointLight::RemoveWhenDestroyed(Component*) {
		if (m_lightDescriptor != nullptr) {
			Context()->Graphics()->RemoveSceneLightDescriptor(m_lightDescriptor);
			dynamic_cast<PointLightDescriptor*>(m_lightDescriptor.operator->())->m_owner = nullptr;
			m_lightDescriptor = nullptr;
		}
	}
}
