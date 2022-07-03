#include "ThirdPersonCamera.h"
#include <Jimara/Components/Physics/Collider.h>
#include <Jimara/Components/Physics/Rigidbody.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>
#include <Jimara/Data/Serialization/Attributes/SliderAttribute.h>

namespace Jimara {
	namespace SampleGame {
		// This file is overly documented, so that the user may use this one as sort of a guid of some of the functionality, provided by the framework...
		class ThirdPersonCamera::Helpers {
		private:
			// Rotates camera:
			inline static void UpdateRotation(ThirdPersonCamera* self, Transform* cameraTransform) {
				// Lets ignore input outside the play mode:
				if (!self->Context()->Updating()) return; 
				const OS::Input* input = self->Context()->Input();
				
				// Get current rotation:
				Vector3 cameraRotation = cameraTransform->WorldEulerAngles();
				
				// Increments rotation based on input: 
				auto addRotationFromAxis = [&](OS::Input::Axis x, OS::Input::Axis y, float sensitivity) {
					cameraRotation.y += input->GetAxis(x) * sensitivity;
					cameraRotation.x += input->GetAxis(y) * sensitivity;
				};

				// Take mouse input:
				{
					const constexpr float MOUSE_SENSITIVITY = 4.0f;
					addRotationFromAxis(OS::Input::Axis::MOUSE_X, OS::Input::Axis::MOUSE_Y, MOUSE_SENSITIVITY);
				}

				// Take controller input:
				{
					const constexpr float CONTROLLER_SENSITIVITY = 180.0f;
					addRotationFromAxis(OS::Input::Axis::CONTROLLER_RIGHT_ANALOG_X, OS::Input::Axis::CONTROLLER_RIGHT_ANALOG_Y, 
						CONTROLLER_SENSITIVITY * self->Context()->Time()->UnscaledDeltaTime());
				}

				// Apply input:
				{
					cameraRotation.x = Math::Min(Math::Max(self->m_minPicth, cameraRotation.x), self->m_maxPitch);
					cameraTransform->SetWorldEulerAngles(cameraRotation);
				}
			}

			// Calculates offset direction 
			inline static Vector3 CalculateOffsetDirection(ThirdPersonCamera* self, Camera* camera, Transform* cameraTransform, const Vector3& targetPosition) {
				// Let us calculate screen aspect ratio for targetOnScreenPosition::
				const float aspectRatio = [&]() {
					// Camera operates on the main RenderStack, so let us get it:
					const Reference<const RenderStack> renderStack = RenderStack::Main(self->Context());

					// RenderStack will never be null, but dumb sanity checks are my style sometimes...
					if (renderStack == nullptr) {
						self->Context()->Log()->Error("ThirdPersonCamera::Helpers::Update - Got null RenderStack! [File: '", __FILE__, "'; Line: ", __LINE__, "]");
						return 1.0f;
					}

					// Calculate aspect ratio from the resolution:
					const Vector2 renderResolution = renderStack->Resolution();
					return (renderResolution.y > std::numeric_limits<float>::epsilon()) ? (renderResolution.x / renderResolution.y) : 1.0f;
				}();

				// Extract meaningfull information from the rotation matrix of the camera transform:
				const Matrix4 rotation = cameraTransform->WorldRotationMatrix();
				const Vector3 right = rotation[0];
				const Vector3 up = rotation[1];
				const Vector3 forward = rotation[2];

				// With Camera's field of view and RenderStack's aspect ratio, we can interpret targetOnScreenPosition as multipliers for up and right directions:
				const float tangentY = std::tan(camera->FieldOfView() * 0.5f) * 2.0f;
				const float tangentX = tangentY * aspectRatio;

				// Calculate offset direction:
				return -Math::Normalize(forward +
					(self->m_targetOnScreenPosition.x * tangentX * right) +
					(self->m_targetOnScreenPosition.y * tangentY * up));
			}

			// Calculates 'distance' by making a raycast and interpolating between current and desired distances
			inline static float GetDistance(ThirdPersonCamera* self, Camera* camera, Transform* cameraTransform, const Vector3& targetPosition, const Vector3& offsetDirection) {
				float maxDistance = self->m_targetDistance * 1000.0f;
				{
					auto onHitFound = [&](const RaycastHit& hit) { maxDistance = Math::Min(hit.distance, maxDistance); };
					auto preFilter = [&](Collider* collider) {
						return collider->GetComponentInParents<Rigidbody>() == nullptr
							? Physics::PhysicsScene::QueryFilterFlag::REPORT : Physics::PhysicsScene::QueryFilterFlag::DISCARD;
					};
					const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*> onPreFilter =
						Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>::FromCall(&preFilter);
					self->Context()->Physics()->Sweep(
						Physics::SphereShape(camera->ClosePlane() * 2.0f),
						Matrix4(
							Vector4(1.0f, 0.0f, 0.0f, 0.0f),
							Vector4(0.0f, 1.0f, 0.0f, 0.0f),
							Vector4(0.0f, 0.0f, 1.0f, 0.0f),
							Vector4(targetPosition, 1.0f)),
						offsetDirection, maxDistance,
						Callback<const RaycastHit&>::FromCall(&onHitFound),
						Physics::PhysicsCollider::LayerMask::All(), 0, &onPreFilter);
				}
				const float currentDistance = Math::Magnitude(cameraTransform->WorldPosition() - targetPosition);
				const float lerpSpeed = (currentDistance >= self->m_targetDistance) ? self->m_zoomInSpeed : self->m_zoomOutSpeed;
				const float lerpAmount = (1.0f - std::exp(-self->Context()->Time()->UnscaledDeltaTime() * lerpSpeed));
				return Math::Min(Math::Lerp(currentDistance, self->m_targetDistance, lerpAmount), maxDistance);
			}

		public:
			// Camera update routine
			inline static void Update(ThirdPersonCamera* self) {
				// With no target transform, there's no way we can calculate the placement:
				Transform* target = self->m_targetTransform;
				if (target == nullptr) return;

				// We need Camera component in parents for calculations:
				Camera* camera = self->GetComponentInParents<Camera>();
				if (camera == nullptr) return;

				// If Camera has no transform, we can not place it anywhere:
				Transform* cameraTransform = camera->GetTransfrom();
				if (cameraTransform == nullptr) return;

				// Rotate camera based on input:
				UpdateRotation(self, cameraTransform);

				// Establish targetPosition and offsetDirection:
				const Vector3 targetPosition = target->WorldPosition();
				const Vector3 offsetDirection = CalculateOffsetDirection(self, camera, cameraTransform, targetPosition);

				// Set actual position:
				const float distance = GetDistance(self, camera, cameraTransform, targetPosition, offsetDirection);
				cameraTransform->SetWorldPosition(targetPosition + offsetDirection * distance);
			}
		};

		ThirdPersonCamera::ThirdPersonCamera(Component* parent, const std::string_view& name) : Component(parent, name) {
			// Constructor, in this case only needs to invoke the base class constructor...
		}

		ThirdPersonCamera::~ThirdPersonCamera() {
			// This is not strictly necessary, but let us make sure some derived class has not messed with the OnComponentDisabled routine:
			Context()->Graphics()->PreGraphicsSynch() -= Callback(Helpers::Update, this);
		}

		void ThirdPersonCamera::GetFields(Callback<Serialization::SerializedObject> reportItem) {
			// Let's use JIMARA_SERIALIZE_FIELDS since it simplifies serialization code a lot
			JIMARA_SERIALIZE_FIELDS(this, reportItem) {
				// Expose target transform reference:
				JIMARA_SERIALIZE_FIELD(m_targetTransform, "Target", "Target transform to look at");

				// Expose m_targetOnScreenPosition x and y separately, with sliders:
				{
					static const auto screenPointRange = Object::Instantiate<Serialization::SliderAttribute<float>>(-0.5f, 0.5f);
					JIMARA_SERIALIZE_FIELD(m_targetOnScreenPosition.x, "Screen X", "Target's position on screen (X axis)", screenPointRange);
					JIMARA_SERIALIZE_FIELD(m_targetOnScreenPosition.y, "Screen Y", "Target's position on screen (Y axis)", screenPointRange);
				}

				// Expose pitch settings:
				{
					// Later validation will need initial values:
					const float initialMinPitch = m_minPicth;

					static const auto pitchRange = Object::Instantiate<Serialization::SliderAttribute<float>>(-90.f, 90.0f);
					JIMARA_SERIALIZE_FIELD(m_minPicth, "Min pitch", "Minimal pitch angle for the camera", pitchRange);
					JIMARA_SERIALIZE_FIELD(m_maxPitch, "Max pitch", "Maximal pitch angle for the camera", pitchRange);

					// Let us make sure maxPitch is always greater than minPitxh:
					if (m_maxPitch < m_minPicth) {
						if (initialMinPitch != m_minPicth)
							m_maxPitch = m_minPicth;
						else m_minPicth = m_maxPitch;
					}
				}

				// Expose zoom-in and zoom-out speeds:
				{
					JIMARA_SERIALIZE_FIELD(m_zoomInSpeed, "Zoom-in speed", "Speed by which the camera 'zooms in' if it gets too far");
					JIMARA_SERIALIZE_FIELD(m_zoomOutSpeed, "Zoom-out speed", "Speed by which the camera 'zooms out' if it gets too close");
				}

				// Expose target distance:
				JIMARA_SERIALIZE_FIELD(m_targetDistance, "Distance", "Distance to target");
			};
		}

		void ThirdPersonCamera::OnComponentEnabled() {
			// If enabled, the ThirdPersonCamera will update on PreGraphicsSynch event to make sure all physics and logic update routines 
			// have already been executed, but Camera's graphics synch point has not yet happened when we calculate the final placement of the camera.
			// (This is the primary reason, we did not go for the UpdateingComponent routine)
			// As a side note, OnComponentEnabled and PreGraphicsSynch will execute regardless if the Editor is in play mode or not.
			Context()->Graphics()->PreGraphicsSynch() += Callback(Helpers::Update, this);
		}

		void ThirdPersonCamera::OnComponentDisabled() {
			// Disabled ThirdPersonCamera should not be updating anything
			Context()->Graphics()->PreGraphicsSynch() -= Callback(Helpers::Update, this);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<SampleGame::ThirdPersonCamera>(const Callback<const Object*>& report) {
		// ComponentSerializer will expose ThirdPersonCamera to the Editor application, as well as enable saving/loading it as a part of a scene:
		static const ComponentSerializer::Of<SampleGame::ThirdPersonCamera> serializer("SampleGame/ThirdPersonCamera", "Third person camera controller");
		report(&serializer);
	}
}
