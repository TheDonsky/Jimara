#include "DirectionalLight.h"
#include "../Transform.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"
#include <limits>


namespace Jimara {
	namespace {
		class DirectionalLightDescriptor : public virtual LightDescriptor, public virtual GraphicsContext::GraphicsObjectSynchronizer {
		public:
			const DirectionalLight* m_owner;

		private:
			struct Data {
				alignas(16) Vector3 direction;
				alignas(16) Vector3 color;
			} m_data;

			LightInfo m_info;

			void UpdateData() {
				if (m_owner == nullptr) return;
				const Transform* transform = m_owner->GetTransfrom();
				if (transform == nullptr) m_data.direction = Vector3(0.0f, -1.0f, 0.0f);
				else m_data.direction = transform->Forward();
				m_data.color = m_owner->Color();
			}

		public:
			inline DirectionalLightDescriptor(const DirectionalLight* owner, uint32_t typeId) : m_owner(owner), m_info{} {
				UpdateData();
				m_info.typeId = typeId;
				m_info.data = &m_data;
				m_info.dataSize = sizeof(Data);
			}

			virtual LightInfo GetLightInfo()const override { return m_info; }

			virtual AABB GetLightBounds()const override {
				static const AABB BOUNDS = []{
					static const float inf = std::numeric_limits<float>::infinity();
					AABB bounds = {};
					bounds.start = Vector3(-inf, -inf, -inf);
					bounds.end = Vector3(inf, inf, inf);
					return bounds;
				}();
				return BOUNDS;
			}

			virtual void OnGraphicsSynch() override { UpdateData(); }
		};
	}

	DirectionalLight::DirectionalLight(Component* parent, const std::string_view& name, Vector3 color)
		: Component(parent, name), m_color(color) {
		uint32_t typeId;
		if (Context()->Graphics()->GetLightTypeId("Jimara_DirectionalLight", typeId))
			m_lightDescriptor = Object::Instantiate<DirectionalLightDescriptor>(this, typeId);
		Context()->Graphics()->AddSceneLightDescriptor(m_lightDescriptor);
		OnDestroyed() += Callback<Component*>(&DirectionalLight::RemoveWhenDestroyed, this);
	}

	DirectionalLight::~DirectionalLight() {
		OnDestroyed() -= Callback<Component*>(&DirectionalLight::RemoveWhenDestroyed, this);
		RemoveWhenDestroyed(this); 
	}

	namespace {
		class DirectionalLightSerializer : public virtual ComponentSerializer::Of<DirectionalLight> {
		public:
			inline DirectionalLightSerializer()
				: ItemSerializer("Jimara/Lights/DirectionalLight", "Directional light component") {}

			inline virtual void SerializeTarget(const Callback<Serialization::SerializedObject>& recordElement, DirectionalLight* target)const override {
				TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>()->SerializeComponent(recordElement, target);

				static const Reference<const Serialization::Vector3Serializer> colorSerializer = Serialization::Vector3Serializer::Create(
					"Color", "Light color",
					Function<Vector3, void*>([](void* targetAddr) { return ((DirectionalLight*)targetAddr)->Color(); }),
					Callback<const Vector3&, void*>([](const Vector3& value, void* targetAddr) { ((DirectionalLight*)targetAddr)->SetColor(value); }),
					{ Object::Instantiate<Serialization::ColorAttribute>() });
				recordElement(Serialization::SerializedObject(colorSerializer, target));
			}

			inline static const ComponentSerializer* Instance() {
				static const DirectionalLightSerializer instance;
				return &instance;
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<DirectionalLight>(const Callback<const Object*>& report) { report(DirectionalLightSerializer::Instance()); }


	Vector3 DirectionalLight::Color()const { return m_color; }

	void DirectionalLight::SetColor(Vector3 color) { m_color = color; }

	void DirectionalLight::RemoveWhenDestroyed(Component*) {
		if (m_lightDescriptor != nullptr) {
			Context()->Graphics()->RemoveSceneLightDescriptor(m_lightDescriptor);
			dynamic_cast<DirectionalLightDescriptor*>(m_lightDescriptor.operator->())->m_owner = nullptr;
			m_lightDescriptor = nullptr;
		}
	}
}
