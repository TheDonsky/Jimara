#pragma once
#include <Jimara/Components/Camera.h>
#include <Jimara/Components/Physics/Collider.h>


namespace Jimara {
	namespace SampleGame {
		/// <summary> Register controller class </summary>
		JIMARA_REGISTER_TYPE(Jimara::SampleGame::CharacterMovement);

		class CharacterMovement : public virtual SceneContext::UpdatingComponent {
		public:
			CharacterMovement(Component* parent, const std::string_view& name = "CharacterMovement");

			/// <summary>
			/// Override for Jimara::Serialization::Serializable to expose controller settings
			/// </summary>
			/// <param name="reportItem"> Callback, with which the serialization utilities access fields </param>
			virtual void GetFields(Callback<Serialization::SerializedObject> reportItem)override;

			struct Input {
				Vector2 movement = Vector2(0.0f);
				bool jump = false;
			};

			class InputSource : public virtual Object {
			public:
				virtual Input GetMovementInput() = 0;
			};

		protected:
			/// <summary> Updates component </summary>
			virtual void Update() override;

		private:
			Reference<InputSource> m_inputSource;
			float m_inputDeadzone = 0.25f;
			float m_minVelocity = 1.0f;
			float m_maxVelocity = 4.0f;
			float m_acceleration = 1.0f;
			float m_jumpSpeed = 8.0f;
			float m_groundCheckHeight = 0.25f;
			float m_groundCheckDistance = 0.3f;
		};
	}

	// TypeIdDetails::GetTypeAttributesOf exposes the serializer
	template<> void TypeIdDetails::GetTypeAttributesOf<SampleGame::CharacterMovement>(const Callback<const Object*>& report);
}
