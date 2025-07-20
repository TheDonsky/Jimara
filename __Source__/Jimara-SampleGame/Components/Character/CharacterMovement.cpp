#include "CharacterMovement.h"
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>
#include <Jimara/Data/Serialization/Attributes/SliderAttribute.h>


namespace Jimara {
	namespace SampleGame {
		CharacterMovement::CharacterMovement(Component* parent, const std::string_view& name) 
			: Component(parent, name) {}

		void CharacterMovement::GetFields(Callback<Serialization::SerializedObject> reportItem) {
			Component::GetFields(reportItem);
			JIMARA_SERIALIZE_FIELDS(this, reportItem) {
				JIMARA_SERIALIZE_FIELD(m_inputSource, "Input", "Input source");
				JIMARA_SERIALIZE_FIELD(m_inputDeadzone,
					"Movement deadzone", "Movement inputs below this threshold will be treated as zero",
					Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 1.0f));
				JIMARA_SERIALIZE_FIELD(m_minVelocity, "Min velocity", "Baseline velocity when movement input magnitude is same as deadzone");
				JIMARA_SERIALIZE_FIELD(m_maxVelocity, "Max velocity", "Maximal velocity, applied when movement input magnitude is equal to or greater than 1");
				JIMARA_SERIALIZE_FIELD(m_acceleration, "Acceleration", "Velocity change rate");
				JIMARA_SERIALIZE_FIELD(m_jumpSpeed, "Jump speed", "Vertical velocity on jump");
				JIMARA_SERIALIZE_FIELD(m_groundCheckHeight, "Ground Check Origin", "Ground Check origin offset from transform position in Up() direction");
				JIMARA_SERIALIZE_FIELD(m_groundCheckDistance, "Ground Check Distance", "Raycast distance for ground check");
			};
		}

		void CharacterMovement::Update() {
			Rigidbody* body = GetComponentInParents<Rigidbody>();
			if (body == nullptr) return;

			Transform* transform = body->GetTransform();
			if (transform == nullptr) return;

			const float deltaTime = Context()->Time()->ScaledDeltaTime();

			Input input = [&]() {
				{
					Component* component = dynamic_cast<Component*>(m_inputSource.operator->());
					if (component != nullptr && component->Destroyed())
						m_inputSource = nullptr;
				}
				if (m_inputSource != nullptr)
					return m_inputSource->GetMovementInput();
				return Input {};
			}();

			const float movementInputMagnitude = Math::Magnitude(input.movement);
			if (movementInputMagnitude < m_inputDeadzone) input.movement = Vector2(0.0f);
			else {
				const float rangeSize = (1.0f - m_inputDeadzone);
				const float lerpAmount = (rangeSize > std::numeric_limits<float>::epsilon())
					? ((movementInputMagnitude - m_inputDeadzone) / rangeSize) : 1.0f;
				input.movement *= Math::Lerp(m_minVelocity, m_maxVelocity, Math::Min(lerpAmount, 1.0f)) / movementInputMagnitude;
			}

			const Vector3 currentVelocity = body->Velocity();
			const Vector3 desiredVelocity = Vector3(input.movement.x, currentVelocity.y, input.movement.y);
			Vector3 velocity = Math::Lerp(currentVelocity, desiredVelocity, 1.0f - std::exp(-deltaTime * m_acceleration));
			if (input.jump) {
				const Vector3 origin = transform->WorldPosition() + Math::Up() * m_groundCheckHeight;
				auto preFilter = [&](Collider* collider) {
					return collider->GetComponentInParents<Rigidbody>() != body
						? Physics::PhysicsScene::QueryFilterFlag::REPORT : Physics::PhysicsScene::QueryFilterFlag::DISCARD;
				};
				const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*> onPreFilter =
					Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>::FromCall(&preFilter);
				if (Context()->Physics()->Raycast(
					origin, Math::Down(), m_groundCheckDistance, Callback(Unused<const RaycastHit&>),
					Physics::PhysicsCollider::LayerMask::All(), 0, &onPreFilter) > 0)
					velocity.y = m_jumpSpeed;
				else input.jump = false;
			}

			body->AddVelocity(velocity - currentVelocity);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<SampleGame::CharacterMovement>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<SampleGame::CharacterMovement>(
			"Character Movement", "SampleGame/CharacterMovement", "Character movement controller");
		report(factory);
	}
}
