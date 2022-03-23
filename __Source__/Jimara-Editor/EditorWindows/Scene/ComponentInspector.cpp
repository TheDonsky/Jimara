#include "ComponentInspector.h"
#include "../../GUI/Utils/DrawObjectPicker.h"
#include "../../GUI/Utils/DrawSerializedObject.h"


namespace Jimara {
	namespace Editor {
		ComponentInspector::ComponentInspector(EditorContext* context, Component* targetComponent) 
			: EditorSceneController(context), EditorWindow(context, "ComponentInspector") {
			SetTarget(targetComponent);
		}

		ComponentInspector::~ComponentInspector() {
			SetTarget(nullptr);
		}

		Reference<Component> ComponentInspector::Target()const {
			std::unique_lock<SpinLock> lock(m_componentLock);
			Reference<Component> result = m_component;
			return result;
		}

		namespace {
			inline static void UpdateComponentInspectorWindowName(Component* target, ComponentInspector* inspector) {
				if (target != nullptr)
					inspector->EditorWindowName() = target->Name();
				else inspector->EditorWindowName() = "ComponentInspector<nullptr>";
			}
		}

		void ComponentInspector::SetTarget(Component* target) {
			Callback<Component*> onTargetDestroyedCallback(&ComponentInspector::OnComponentDestroyed, this);
			std::unique_lock<SpinLock> lock(m_componentLock);
			if (m_component == target) return;
			else if (m_component != nullptr)
				m_component->OnDestroyed() -= onTargetDestroyedCallback;
			m_component = target;
			if (m_component != nullptr)
				m_component->OnDestroyed() += onTargetDestroyedCallback;
			UpdateComponentInspectorWindowName(m_component, this);
		}

		void ComponentInspector::DrawEditorWindow() {
			Reference<EditorScene> editorScene = GetOrCreateScene();
			std::unique_lock<std::recursive_mutex> lock(editorScene->UpdateLock());
			UpdateComponentInspectorWindowName(m_component, this);
			Reference<Component> target = Target();
			Reference<ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();
			const ComponentSerializer* serializer = serializers->FindSerializerOf(target);
			if (serializer != nullptr)
				if (DrawSerializedObject(serializer->Serialize(target), (size_t)this, editorScene->RootObject()->Context()->Log(), [&](const Serialization::SerializedObject& object) {
				const std::string name = CustomSerializedObjectDrawer::DefaultGuiItemName(object, (size_t)this);
				static thread_local std::vector<char> searchBuffer;
				return DrawObjectPicker(object, name, editorScene->RootObject()->Context()->Log(), editorScene->RootObject(), Context()->EditorAssetDatabase(), &searchBuffer);
					})) editorScene->TrackComponent(target, false);
		}


		void ComponentInspector::OnComponentDestroyed(Component* component) {
			std::unique_lock<SpinLock> lock(m_componentLock);
			if (m_component != component) return;
			else if (m_component != nullptr) {
				m_component->OnDestroyed() -= Callback<Component*>(&ComponentInspector::OnComponentDestroyed, this);
				m_component = nullptr;
				Close();
			}
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::ComponentInspector>(const Callback<TypeId>& report) {
		report(TypeId::Of<Editor::EditorSceneController>());
		report(TypeId::Of<Editor::EditorWindow>());
	}
}
