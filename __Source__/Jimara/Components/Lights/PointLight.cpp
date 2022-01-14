#include "PointLight.h"
#include "../Transform.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"


namespace Jimara {
	namespace {
		class PointLightDescriptor : public virtual LightDescriptor, public virtual JobSystem::Job {
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
				m_data.color = m_owner->Color();
				m_radius = m_owner->Radius();
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

		protected:
			virtual void Execute()override { UpdateData(); }
			virtual void CollectDependencies(Callback<Job*>)override {}
		};
	}

	PointLight::PointLight(Component* parent, const std::string_view& name, Vector3 color, float radius)
		: Component(parent, name)
		, m_allLights(LightDescriptor::Set::GetInstance(parent->Context()))
		, m_color(color)
		, m_radius(radius) {}

	PointLight::~PointLight() {
		OnComponentDisabled();
	}

	namespace {
		class PointLightSerializer : public ComponentSerializer::Of<PointLight> {
		public:
			inline PointLightSerializer()
				: ItemSerializer("Jimara/Lights/PointLight", "Point light component") {}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, PointLight* target)const override {
				TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>()->GetFields(recordElement, target);

				static const Reference<const FieldSerializer> colorSerializer = Serialization::Vector3Serializer::For<PointLight>(
					"Color", "Light color",
					[](PointLight* target) { return target->Color(); },
					[](const Vector3& value, PointLight* target) { target->SetColor(value); },
					{ Object::Instantiate<Serialization::ColorAttribute>() });
				recordElement(colorSerializer->Serialize(target));

				static const Reference<const FieldSerializer> radiusSerializer = Serialization::FloatSerializer::For<PointLight>(
					"Radius", "Light reach",
					[](PointLight* target) { return target->Radius(); },
					[](const float& value, PointLight* target) { target->SetRadius(value); });
				recordElement(radiusSerializer->Serialize(target));
			}

			inline static const ComponentSerializer* Instance() {
				static const PointLightSerializer instance;
				return &instance;
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<PointLight>(const Callback<const Object*>& report) { report(PointLightSerializer::Instance()); }


	Vector3 PointLight::Color()const { return m_color; }

	void PointLight::SetColor(Vector3 color) { m_color = color; }


	float PointLight::Radius()const { return m_radius; }

	void PointLight::SetRadius(float radius) { m_radius = radius <= 0.0f ? 0.0f : radius; }

	void PointLight::OnComponentEnabled() {
		if (!ActiveInHeirarchy())
			OnComponentDisabled();
		else if (m_lightDescriptor == nullptr) {
			uint32_t typeId;
			if (Context()->Graphics()->Configuration().GetLightTypeId("Jimara_PointLight", typeId)) {
				Reference<PointLightDescriptor> descriptor = Object::Instantiate<PointLightDescriptor>(this, typeId);
				m_lightDescriptor = Object::Instantiate<LightDescriptor::Set::ItemOwner>(descriptor);
				m_allLights->Add(m_lightDescriptor);
				Context()->Graphics()->SynchPointJobs().Add(descriptor);
			}
		}
	}

	void PointLight::OnComponentDisabled() {
		if (ActiveInHeirarchy())
			OnComponentDisabled();
		else if (m_lightDescriptor != nullptr) {
			m_allLights->Remove(m_lightDescriptor);
			Context()->Graphics()->SynchPointJobs().Remove(dynamic_cast<JobSystem::Job*>(m_lightDescriptor->Item()));
			dynamic_cast<PointLightDescriptor*>(m_lightDescriptor->Item())->m_owner = nullptr;
			m_lightDescriptor = nullptr;
		}
	}
}
