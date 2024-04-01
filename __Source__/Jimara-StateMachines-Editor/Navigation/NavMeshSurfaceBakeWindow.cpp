#include "NavMeshSurfaceBakeWindow.h"
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>
#include <Jimara-Editor/GUI/ImGuiWrappers.h>
#include <Jimara-StateMachines/Navigation/NavMesh/NavMeshBaker.h>

namespace Jimara {
	namespace Editor {
		NavMeshSurfaceBakeWindow::NavMeshSurfaceBakeWindow(EditorContext* context) 
			: EditorWindow(context, "Bake NavMesh Surface") {
			assert(context != nullptr);
		}

		NavMeshSurfaceBakeWindow::~NavMeshSurfaceBakeWindow() {}

		void NavMeshSurfaceBakeWindow::DrawEditorWindow() {
			Reference<EditorScene> scene = EditorWindowContext()->GetScene();
			if (scene == nullptr)
				return;
			if (!Button("Bake ### NavMeshSurfaceBakeWindow_Bake"))
				return;
			std::unique_lock<std::recursive_mutex> lock(scene->UpdateLock());
			NavMeshBaker::Settings settings;
			{
				settings.environmentRoot = scene->RootObject();
				std::vector<Collider*> colliders = settings.environmentRoot->GetComponentsInChildren<Collider>(true);
				auto isUnbound = [](const AABB& bnd) {
					return !(
						std::isfinite(bnd.start.x) && std::isfinite(bnd.end.x) &&
						std::isfinite(bnd.start.y) && std::isfinite(bnd.end.y) &&
						std::isfinite(bnd.start.z) && std::isfinite(bnd.end.z));
				};
				AABB bounds = AABB(
					Vector3(std::numeric_limits<float>::quiet_NaN()),
					Vector3(std::numeric_limits<float>::quiet_NaN()));
				for (size_t i = 0u; i < colliders.size(); i++) {
					const BoundedObject* boundedObject = dynamic_cast<BoundedObject*>(colliders[i]);
					if (boundedObject == nullptr)
						continue;
					const AABB bnd = boundedObject->GetBoundaries();
					if (isUnbound(bnd))
						continue;
					if (isUnbound(bounds))
						bounds = bnd;
					bounds = AABB(
						Vector3(
							Math::Min(bounds.start.x, bnd.start.x, bnd.end.x),
							Math::Min(bounds.start.y, bnd.start.y, bnd.end.y),
							Math::Min(bounds.start.z, bnd.start.z, bnd.end.z)),
						Vector3(
							Math::Max(bounds.end.x, bnd.start.x, bnd.end.x),
							Math::Max(bounds.end.y, bnd.start.y, bnd.end.y),
							Math::Max(bounds.end.z, bnd.start.z, bnd.end.z)));
				}
				if (isUnbound(bounds))
					return;
				settings.volumePose = Math::Identity();
				settings.volumePose[3u] = Vector4(0.5f * (bounds.start + bounds.end), 1.0f);
				settings.volumeSize = (bounds.end - bounds.start) * 1.01f;
			}
			settings.verticalSampleCount = Size2(
				static_cast<uint32_t>(settings.volumeSize.x / 0.1f),
				static_cast<uint32_t>(settings.volumeSize.z / 0.1f));
			NavMeshBaker baker(settings);
			Reference<TriMesh> mesh = baker.Result();
			if (mesh == nullptr)
				return;
			const Reference<Transform> transform = Object::Instantiate<Transform>(scene->RootObject(), "NavMesh shape");
			transform->SetLocalPosition(Vector3(0.0f, 0.1f, 0.0f));
			Object::Instantiate<MeshRenderer>(transform)->SetMesh(mesh);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::NavMeshSurfaceBakeWindow>(const Callback<const Object*>& report) {
		Reference<Editor::EditorMainMenuCallback> createAction = Object::Instantiate<Editor::EditorMainMenuCallback>(
			"State Machines/Navigation/Bake NavMesh Surface", "Utility for baking navigation mesh geometry",
			Callback<Editor::EditorContext*>([](Editor::EditorContext* context) {
				Object::Instantiate<Editor::NavMeshSurfaceBakeWindow>(context);
			}));
		report(createAction);
	}
}
