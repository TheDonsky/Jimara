#include "FocusOnSelectionAction.h"
#include <Jimara/Components/Transform.h>
#include <Jimara/Environment/Interfaces/BoundedObject.h>


namespace Jimara {
	namespace Editor {
		FocusOnSelectionAction::FocusOnSelectionAction(Scene::LogicContext* context) 
			: Component(context, "FocusOnSelectionAction") {}

		FocusOnSelectionAction::~FocusOnSelectionAction() {}

		void FocusOnSelectionAction::Update() {
			// Check key press:
			if (!Context()->Input()->KeyDown(KEY))
				return;

			// Collect all BoundedObject components:
			const std::unordered_set<const BoundedObject*> boundedComponents = [&]() {
				std::unordered_set<const BoundedObject*> result;
				std::vector<const BoundedObject*> childBuffer;
				GizmoContext()->Selection()->Iterate([&](Component* component) {
					assert(component != nullptr);
					assert(childBuffer.empty());
					{
						const BoundedObject* boundedComponent = dynamic_cast<const BoundedObject*>(component);
						if (boundedComponent != nullptr)
							result.insert(boundedComponent);
					}
#pragma warning(disable: 28182)
					component->GetComponentsInChildren<BoundedObject>(childBuffer, true);
#pragma warning(default: 28182)
					for (size_t i = 0u; i < childBuffer.size(); i++)
						result.insert(childBuffer[i]);
					childBuffer.clear();
					});
				return result;
			}();
			

			// Define a few tools for boundary calculation:
			static const auto isBounded = [](const AABB& bnd) {
				return
					std::isfinite(bnd.start.x) && std::isfinite(bnd.start.y) && std::isfinite(bnd.start.z) &&
					std::isfinite(bnd.end.x) && std::isfinite(bnd.end.y) && std::isfinite(bnd.end.z);
			};
			static const auto expand = [](AABB& bounds, const AABB& bnd) {
				if (!isBounded(bnd))
					return false;
				else if (isBounded(bounds))
					bounds = AABB(
						Vector3(
							Math::Min(bounds.start.x, bnd.start.x),
							Math::Min(bounds.start.y, bnd.start.y),
							Math::Min(bounds.start.z, bnd.start.z)),
						Vector3(
							Math::Max(bounds.end.x, bnd.end.x),
							Math::Max(bounds.end.y, bnd.end.y),
							Math::Max(bounds.end.z, bnd.end.z)));
				else bounds = bnd;
				return true;
			};
			static const auto boundSize = [](const AABB& bnd) {
				return Math::Max(
					bnd.end.x - bnd.start.x,
					bnd.end.y - bnd.start.y,
					bnd.end.z - bnd.start.z);
			};
			AABB bounds = AABB(
				Vector3(std::numeric_limits<float>::quiet_NaN()), 
				Vector3(std::numeric_limits<float>::quiet_NaN()));
			float averageSize = 0.0f;

			// Include transform positions in the total boundary:
			{
				GizmoContext()->Selection()->Iterate([&](Component* component) {
					Transform* transform = component->GetTransfrom();
					if (transform == nullptr)
						return;
					const Vector3 position = transform->WorldPosition();
					expand(bounds, AABB(position, position));
					});
				if (isBounded(bounds))
					averageSize = boundSize(bounds);
			}

			// Include bounded objects in the total boundary:
			{
				float boundIndex = 0.0f;
				for (decltype(boundedComponents)::const_iterator it = boundedComponents.begin(); it != boundedComponents.end(); ++it) {
					AABB bnd = (*it)->GetBoundaries();
					if (!isBounded(bnd))
						continue;
					bnd = AABB(
						Vector3(
							Math::Min(bnd.start.x, bnd.end.x),
							Math::Min(bnd.start.y, bnd.end.y),
							Math::Min(bnd.start.z, bnd.end.z)),
						Vector3(
							Math::Max(bnd.start.x, bnd.end.x),
							Math::Max(bnd.start.y, bnd.end.y),
							Math::Max(bnd.start.z, bnd.end.z)));
					boundIndex++;
					averageSize = Math::Lerp(averageSize, boundSize(bnd), 1.0f / boundIndex);
					expand(bounds, bnd);
				}
			}

			// Do some final checks and size calculations:
			if (!isBounded(bounds))
				return;
			if (averageSize <= std::numeric_limits<float>::epsilon())
				averageSize = 1.0f;
			const float boundarySize = boundSize(bounds);


			// Adjust gizmo viewport position and size:
			GizmoViewport* viewport = GizmoContext()->Viewport();
			Transform* viewportTransform = viewport->ViewportTransform();
			viewportTransform->SetWorldPosition(
				(bounds.start + bounds.end) * 0.5f -
				viewportTransform->Forward() * (boundarySize + averageSize) * 0.5f *
				std::abs(1.0f / std::tan(Math::Radians(viewport->FieldOfView()) * 0.5f)));
			viewport->SetOrthographicSize(boundarySize + averageSize);
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection FocusOnSelectionAction_GizmoConnection =
				Gizmo::ComponentConnection::Targetless<FocusOnSelectionAction>();
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::FocusOnSelectionAction>() {
		Editor::Gizmo::AddConnection(Editor::FocusOnSelectionAction_GizmoConnection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::FocusOnSelectionAction>() {
		Editor::Gizmo::RemoveConnection(Editor::FocusOnSelectionAction_GizmoConnection);
	}
}
