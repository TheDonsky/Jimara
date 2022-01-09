#include "DirectionalLight.h"
#include "../Transform.h"
#include "../../Data/Serialization/Attributes/ColorAttribute.h"
#include <limits>


namespace Jimara {
	namespace {
		class DirectionalLightDescriptor : public virtual LightDescriptor, public virtual
#ifdef USE_REFACTORED_SCENE
			JobSystem::Job
#else
			GraphicsContext::GraphicsObjectSynchronizer
#endif
		{
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

#ifdef USE_REFACTORED_SCENE
		protected:
			virtual void Execute()override { UpdateData(); }
			virtual void CollectDependencies(Callback<Job*>)override {}
#else
			virtual void OnGraphicsSynch() override { UpdateData(); }
#endif
		};
	}

	DirectionalLight::DirectionalLight(Component* parent, const std::string_view& name, Vector3 color)
		: Component(parent, name)
#ifdef USE_REFACTORED_SCENE
		, m_allLights(LightDescriptor::Set::GetInstance(parent->Context()))
#endif
		, m_color(color) {
		uint32_t typeId;
		if (Context()->Graphics()->
#ifdef USE_REFACTORED_SCENE
			Configuration().
#endif
			GetLightTypeId("Jimara_DirectionalLight", typeId)) {
			Reference<DirectionalLightDescriptor> descriptor = Object::Instantiate<DirectionalLightDescriptor>(this, typeId);
#ifdef USE_REFACTORED_SCENE
			m_lightDescriptor = Object::Instantiate<LightDescriptor::Set::ItemOwner>(descriptor);
			m_allLights->Add(m_lightDescriptor);
			Context()->Graphics()->SynchPointJobs().Add(descriptor);
#else
			m_lightDescriptor = descriptor;
			Context()->Graphics()->AddSceneLightDescriptor(m_lightDescriptor);
#endif
		}
		OnDestroyed() += Callback<Component*>(&DirectionalLight::RemoveWhenDestroyed, this);
	}

	DirectionalLight::~DirectionalLight() {
		OnDestroyed() -= Callback<Component*>(&DirectionalLight::RemoveWhenDestroyed, this);
		RemoveWhenDestroyed(this); 
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

	void DirectionalLight::RemoveWhenDestroyed(Component*) {
		if (m_lightDescriptor != nullptr) {
#ifdef USE_REFACTORED_SCENE
			m_allLights->Remove(m_lightDescriptor);
			Context()->Graphics()->SynchPointJobs().Remove(dynamic_cast<JobSystem::Job*>(m_lightDescriptor->Item()));
#else
			Context()->Graphics()->RemoveSceneLightDescriptor(m_lightDescriptor);
#endif
			dynamic_cast<DirectionalLightDescriptor*>(
#ifdef USE_REFACTORED_SCENE
				m_lightDescriptor->Item()
#else
				m_lightDescriptor.operator->()
#endif
				)->m_owner = nullptr;
			m_lightDescriptor = nullptr;
		}
	}
}
