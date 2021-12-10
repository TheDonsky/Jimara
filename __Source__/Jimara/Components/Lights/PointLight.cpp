#include "PointLight.h"
#include "../Transform.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"


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

	namespace {
		class PointLightSerializer : public virtual ComponentSerializer {
		public:
			inline PointLightSerializer()
				: ItemSerializer("PointLight", "Point light component"), ComponentSerializer("Jimara/Lights/PointLight") {}

			inline virtual Reference<Component> CreateComponent(Component* parent) const override {
				return Object::Instantiate<PointLight>(parent, "Point Light");
			}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, void* targetAddr)const override {
				PointLight* target = dynamic_cast<PointLight*>((Component*)targetAddr);
				target->Component::GetSerializer()->GetFields(recordElement, targetAddr);

				static const Reference<const Serialization::Vector3Serializer> colorSerializer = Serialization::Vector3Serializer::Create(
					"Color", "Light color",
					Function<Vector3, void*>([](void* targetAddr) { return dynamic_cast<PointLight*>((Component*)targetAddr)->Color(); }),
					Callback<const Vector3&, void*>([](const Vector3& value, void* targetAddr) { dynamic_cast<PointLight*>((Component*)targetAddr)->SetColor(value); }),
					{ Object::Instantiate<Serialization::ColorAttribute>() });
				recordElement(Serialization::SerializedObject(colorSerializer, targetAddr));

				static const Reference<const Serialization::FloatSerializer> radiusSerializer = Serialization::FloatSerializer::Create(
					"Radius", "Light reach",
					Function<float, void*>([](void* targetAddr) { return dynamic_cast<PointLight*>((Component*)targetAddr)->Radius(); }),
					Callback<const float&, void*>([](const float& value, void* targetAddr) { dynamic_cast<PointLight*>((Component*)targetAddr)->SetRadius(value); }));
				recordElement(Serialization::SerializedObject(radiusSerializer, targetAddr));
			}

			inline static const ComponentSerializer* Instance() {
				static const PointLightSerializer instance;
				return &instance;
			}
		};

		static ComponentSerializer::RegistryEntry POINT_LIGHT_SERIALIZER;
	}

	Reference<const ComponentSerializer> PointLight::GetSerializer()const {
		return PointLightSerializer::Instance();
	}

	template<> void TypeIdDetails::OnRegisterType<PointLight>() { POINT_LIGHT_SERIALIZER = PointLightSerializer::Instance(); }
	template<> void TypeIdDetails::OnUnregisterType<PointLight>() { POINT_LIGHT_SERIALIZER = nullptr; }


	Vector3 PointLight::Color()const { return m_color; }

	void PointLight::SetColor(Vector3 color) { m_color = color; }


	float PointLight::Radius()const { return m_radius; }

	void PointLight::SetRadius(float radius) { m_radius = radius <= 0.0f ? 0.0f : radius; }


	void PointLight::RemoveWhenDestroyed(Component*) {
		if (m_lightDescriptor != nullptr) {
			Context()->Graphics()->RemoveSceneLightDescriptor(m_lightDescriptor);
			dynamic_cast<PointLightDescriptor*>(m_lightDescriptor.operator->())->m_owner = nullptr;
			m_lightDescriptor = nullptr;
		}
	}
}
