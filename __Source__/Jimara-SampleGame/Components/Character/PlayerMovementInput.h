#pragma once
#include <Jimara/Components/Camera.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>
#include "CharacterMovement.h"


namespace Jimara {
	namespace SampleGame {
		/// <summary> Register controller class </summary>
		JIMARA_REGISTER_TYPE(Jimara::SampleGame::PlayerMovementInput);

		class PlayerMovementInput : public virtual Component, public virtual CharacterMovement::InputSource {
		public:
			inline PlayerMovementInput(Component* parent, const std::string_view& name = "PlayerMovementInput") : Component(parent, name) {}

			virtual CharacterMovement::Input GetMovementInput() override {
				const OS::Input* input = Context()->Input();
				CharacterMovement::Input rv;
				rv.movement = [&]() {
					const Vector2 rawInput = Vector2(
						(input->KeyPressed(OS::Input::KeyCode::A) ? -1.0f : 0.0f) + (input->KeyPressed(OS::Input::KeyCode::D) ? 1.0f : 0.0f),
						(input->KeyPressed(OS::Input::KeyCode::S) ? -1.0f : 0.0f) + (input->KeyPressed(OS::Input::KeyCode::W) ? 1.0f : 0.0f)) +
						Vector2(input->GetAxis(OS::Input::Axis::CONTROLLER_LEFT_ANALOG_X), input->GetAxis(OS::Input::Axis::CONTROLLER_LEFT_ANALOG_Y));
					if (m_camera == nullptr || m_camera->Destroyed()) {
						m_camera = nullptr;
						return rawInput;
					}
					Transform* viewTransform = m_camera->GetTransfrom();
					if (viewTransform == nullptr) return rawInput;
					const Vector3 rawRight = viewTransform->Right();
					const Vector3 rawForward = viewTransform->Forward();
					const Vector2 right = Math::Normalize(Vector2(rawRight.x, rawRight.z));
					const Vector2 forward = Math::Normalize(Vector2(rawForward.x, rawForward.z));
					return right * rawInput.x + forward * rawInput.y;
				}();
				rv.jump = (input->KeyDown(OS::Input::KeyCode::SPACE) || input->KeyDown(OS::Input::KeyCode::CONTROLLER_BUTTON_SOUTH));
				return rv;
			}

			/// <summary>
			/// Override for Jimara::Serialization::Serializable to expose controller settings
			/// </summary>
			/// <param name="reportItem"> Callback, with which the serialization utilities access fields </param>
			inline virtual void GetFields(Callback<Serialization::SerializedObject> reportItem)override {
				Component::GetFields(reportItem);
				JIMARA_SERIALIZE_FIELDS(this, reportItem) {
					JIMARA_SERIALIZE_FIELD(m_camera, "Camera", "Camera, for alignment");
				};
			}

		private:
			Reference<Camera> m_camera;
		};
	}

	// TypeIdDetails::GetTypeAttributesOf exposes the serializer
	template<> inline void TypeIdDetails::GetTypeAttributesOf<SampleGame::PlayerMovementInput>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<SampleGame::PlayerMovementInput>(
			"Player movement input", "SampleGame/PlayerMovementInput", "Player movement input provider");
		report(factory);
	}
}
