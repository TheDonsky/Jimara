#pragma once
#include <Jimara/Components/Transform.h>
#include <Jimara/Components/UI/UITransform.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


namespace Jimara {
	namespace SampleGame {
		JIMARA_REGISTER_TYPE(Jimara::SampleGame::Compass);

		class Compass : public virtual Scene::LogicContext::UpdatingComponent {
		public:
			inline Compass(Component* parent, const std::string_view& name = "Compass") : Component(parent, name) {}

			/// <summary>
			/// Override for Jimara::Serialization::Serializable to expose controller settings
			/// </summary>
			/// <param name="reportItem"> Callback, with which the serialization utilities access fields </param>
			inline virtual void GetFields(Callback<Serialization::SerializedObject> reportItem)override {
				Component::GetFields(reportItem);
				JIMARA_SERIALIZE_FIELDS(this, reportItem) {
					JIMARA_SERIALIZE_FIELD(m_target, "Target", "Target, for alignment");
				};
			}

		protected:
			inline virtual void Update()override {
				if (m_target == nullptr) return;
				else if (m_target->Destroyed()) {
					m_target = nullptr;
					return;
				}
				UI::UITransform* transform = GetComponentInParents<UI::UITransform>();
				if (transform != nullptr) transform->SetRotation(m_target->WorldEulerAngles().y);
			}

		private:
			Reference<Transform> m_target;
		};
	}

	// TypeIdDetails::GetTypeAttributesOf exposes the serializer
	template<> inline void TypeIdDetails::GetTypeAttributesOf<SampleGame::Compass>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<SampleGame::Compass> serializer("SampleGame/UI/Compass", "Compass");
		report(&serializer);
	}
}

