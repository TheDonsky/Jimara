#include "NavMeshSurfaceBakeWindow.h"
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>
#include <Jimara-Editor/GUI/ImGuiWrappers.h>
#include <Jimara-Editor/GUI/Utils/DrawSerializedObject.h>
#include <Jimara-Editor/GUI/Utils/DrawObjectPicker.h>

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
			std::unique_lock<std::recursive_mutex> lock(scene->UpdateLock());
			Reference<Component> rootObject = m_root;
			{
				bool found = false;
				for (Component* ptr = rootObject; ptr != nullptr; ptr = ptr->Parent())
					if (ptr == scene->RootObject()) {
						found = true;
						break;
					}
				if (!found)
					rootObject = nullptr;
			}
			m_settings.environmentRoot = rootObject;
			static const NavMeshBaker::Settings::Serializer serializer("Settings");
			DrawSerializedObject(serializer.Serialize(m_settings), (size_t)this, EditorWindowContext()->Log(),
				[&](const Serialization::SerializedObject& object) -> bool {
					const std::string name = CustomSerializedObjectDrawer::DefaultGuiItemName(object, (size_t)this);
					static thread_local std::vector<char> searchBuffer;
					return DrawObjectPicker(object, name, EditorWindowContext()->Log(), scene->RootObject(), nullptr, &searchBuffer);
				});
			m_root = m_settings.environmentRoot;
			if (m_settings.environmentRoot == nullptr)
				m_settings.environmentRoot = scene->RootObject();
			rootObject = m_settings.environmentRoot;

			if (!Button("Bake ### NavMeshSurfaceBakeWindow_Bake"))
				return;

			{
				std::vector<Collider*> colliders = m_settings.environmentRoot->GetComponentsInChildren<Collider>(true);
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
				m_settings.volumePose = Math::Identity();
				m_settings.volumePose[3u] = Vector4(0.5f * (bounds.start + bounds.end), 1.0f);
				m_settings.volumeSize = (bounds.end - bounds.start) * 1.01f;
			}
			NavMeshBaker baker(m_settings);
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
