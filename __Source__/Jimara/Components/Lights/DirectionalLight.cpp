#include "DirectionalLight.h"
#include "../Transform.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"
#include <limits>


namespace Jimara {
	namespace {
		class DirectionalLightDescriptor : public virtual LightDescriptor, public virtual JobSystem::Job {
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

		protected:
			virtual void Execute()override { UpdateData(); }
			virtual void CollectDependencies(Callback<Job*>)override {}
		};
	}

	DirectionalLight::DirectionalLight(Component* parent, const std::string_view& name, Vector3 color)
		: Component(parent, name)
		, m_allLights(LightDescriptor::Set::GetInstance(parent->Context()))
		, m_color(color) {}

	DirectionalLight::~DirectionalLight() {
		OnComponentDisabled();
	}

	namespace {
		class DirectionalLightSerializer : public ComponentSerializer::Of<DirectionalLight> {
		public:
			inline DirectionalLightSerializer()
				: ItemSerializer("Jimara/Lights/DirectionalLight", "Directional light component") {}

			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, DirectionalLight* target)const override {
				TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>()->GetFields(recordElement, target);

				static const Reference<const FieldSerializer> colorSerializer = Serialization::Vector3Serializer::For<DirectionalLight>(
					"Color", "Light color",
					[](DirectionalLight* target) { return target->Color(); },
					[](const Vector3& value, DirectionalLight* target) { target->SetColor(value); },
					{ Object::Instantiate<Serialization::ColorAttribute>() });
				recordElement(colorSerializer->Serialize(target));
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

	void DirectionalLight::OnComponentEnabled() {
		if (!ActiveInHeirarchy())
			OnComponentDisabled();
		else if (m_lightDescriptor == nullptr) {
			uint32_t typeId;
			if (Context()->Graphics()->Configuration().GetLightTypeId("Jimara_DirectionalLight", typeId)) {
				Reference<DirectionalLightDescriptor> descriptor = Object::Instantiate<DirectionalLightDescriptor>(this, typeId);
				m_lightDescriptor = Object::Instantiate<LightDescriptor::Set::ItemOwner>(descriptor);
				m_allLights->Add(m_lightDescriptor);
				Context()->Graphics()->SynchPointJobs().Add(descriptor);
			}
		}
	}

	void DirectionalLight::OnComponentDisabled() {
		if (ActiveInHeirarchy())
			OnComponentEnabled();
		else if (m_lightDescriptor != nullptr) {
			m_allLights->Remove(m_lightDescriptor);
			Context()->Graphics()->SynchPointJobs().Remove(dynamic_cast<JobSystem::Job*>(m_lightDescriptor->Item()));
			dynamic_cast<DirectionalLightDescriptor*>(m_lightDescriptor->Item())->m_owner = nullptr;
			m_lightDescriptor = nullptr;
		}
	}
}
