#include "FocusOnSelectionAction.h"
#include <Components/Transform.h>
#include <Environment/Interfaces/BoundedObject.h>


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
					component->GetComponentsInChildren<BoundedObject>(childBuffer, true);
					for (size_t i = 0u; i < childBuffer.size(); i++)
						result.insert(childBuffer[i]);
					childBuffer.clear();
					});
				return result;
			}();
			if (boundedComponents.empty())
				return;
			
			// Calculate total boundaries:
			auto isBounded = [](const AABB& bnd) {
				return
					std::isfinite(bnd.start.x) && std::isfinite(bnd.start.y) && std::isfinite(bnd.start.z) &&
					std::isfinite(bnd.end.x) && std::isfinite(bnd.end.y) && std::isfinite(bnd.end.z);
			};
			const auto [bounds, averageSize] = [&]() {
				std::unordered_set<const BoundedObject*>::const_iterator it = boundedComponents.begin();
				float averageSize = 0.0f;
				float index = 0.0f;
				auto getNextBounds = [&]() {
					AABB bnd = (*it)->GetBoundaries();
					++it;
					if (!isBounded(bnd))
						return bnd;
					bnd = AABB(
						Vector3(
							Math::Min(bnd.start.x, bnd.end.x), 
							Math::Min(bnd.start.y, bnd.end.y), 
							Math::Min(bnd.start.z, bnd.end.z)),
						Vector3(
							Math::Max(bnd.start.x, bnd.end.x), 
							Math::Max(bnd.start.y, bnd.end.y), 
							Math::Max(bnd.start.z, bnd.end.z)));
					index++;
					averageSize = Math::Lerp(averageSize, Math::Max(
						bnd.end.x - bnd.start.x,
						bnd.end.y - bnd.start.y,
						bnd.end.z - bnd.start.z), 1.0f / index);
					return bnd;
				};
				AABB result = getNextBounds();
				while (it != boundedComponents.end()) {
					const AABB bnd = getNextBounds();
					if (!isBounded(bnd))
						continue;
					else if (!isBounded(result))
						result = bnd;
					else result = AABB(
						Vector3(
							Math::Min(result.start.x, bnd.start.x),
							Math::Min(result.start.y, bnd.start.y),
							Math::Min(result.start.z, bnd.start.z)),
						Vector3(
							Math::Max(result.end.x, bnd.end.x),
							Math::Max(result.end.y, bnd.end.y),
							Math::Max(result.end.z, bnd.end.z)));
				}
				return std::make_pair(result, averageSize);
			}();
			if (!isBounded(bounds))
				return;

			// Total boundary size:
			const float boundarySize = Math::Max(
				bounds.end.x - bounds.start.x,
				bounds.end.y - bounds.start.y,
				bounds.end.z - bounds.start.z);

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
