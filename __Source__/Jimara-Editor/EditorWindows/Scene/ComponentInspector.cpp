#include "ComponentInspector.h"
#include "../Utils/DrawSerializedObject.h"


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
				std::stringstream stream;
				if (target != nullptr) {
					Reference<ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();
					const ComponentSerializer* serializer = serializers->FindSerializerOf(typeid(*target));
					if (serializer == nullptr)
						stream << "ComponentInspector<Unknown Type>";
					else stream << serializer->TargetComponentType().Name();
				}
				else stream << "ComponentInspector<nullptr>";
				stream << "###editor_component_inspector_view_" << ((size_t)inspector);
				inspector->EditorWindowName() = stream.str();
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
			Reference<Component> target = Target();
			Reference<ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();
			const ComponentSerializer* serializer = serializers->FindSerializerOf(target);
			if (serializer != nullptr)
				DrawSerializedObject(serializer->Serialize(target), (size_t)this, editorScene->RootObject()->Context()->Log(), [&](const Serialization::SerializedObject&) {
				editorScene->RootObject()->Context()->Log()->Error("ComponentInspector::DrawEditorWindow - Object Pointer not yet supported inside the inspector!");
					});
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
