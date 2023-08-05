#include "ComponentInspector.h"
#include "../../Environment/EditorStorage.h"
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
				else inspector->EditorWindowName() = "Component Inspector";
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
			Reference<ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();
			auto drawTargetInspector = [&](Component* target) {
				const ComponentSerializer* serializer = serializers->FindSerializerOf(target);
				if (serializer != nullptr) {
					{
						const std::string label(serializer->TargetComponentType().Name());
						ImGui::LabelText("", label.c_str());
						ImGui::Separator();
					}
					if (DrawSerializedObject(serializer->Serialize(target), (size_t)this, editorScene->RootObject()->Context()->Log(), [&](const Serialization::SerializedObject& object) {
						const std::string name = CustomSerializedObjectDrawer::DefaultGuiItemName(object, (size_t)this);
						static thread_local std::vector<char> searchBuffer;
						return DrawObjectPicker(object, name, editorScene->RootObject()->Context()->Log(), editorScene->RootObject(), Context()->EditorAssetDatabase(), &searchBuffer);
						})) {
						editorScene->TrackComponent(target, false);
					}
					return true;
				}
				else return false;
			};
			Reference<Component> target = Target();
			if (target != nullptr) drawTargetInspector(target);
			else editorScene->Selection()->Iterate([&](Component* component) {
				if (editorScene->Selection()->Count() > 1) {
					if (component == nullptr) return;
					const std::string text = [&]() {
						std::stringstream stream;
						stream << component->Name() << "###component_inspector_view_view_" << ((size_t)this) << "_selection_tree_node" << ((size_t)component);
						return stream.str();
					}();
					if (ImGui::TreeNode(text.c_str())) {
						drawTargetInspector(component);
						ImGui::TreePop();
					}
				}
				else drawTargetInspector(component);
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

		namespace {
			static const EditorMainMenuCallback editorMenuCallback(
				"Scene/Component Inspector", "Open Component Inspector window for selection", Callback<EditorContext*>([](EditorContext* context) {
					Object::Instantiate<ComponentInspector>(context, nullptr);
					}));
			static EditorMainMenuAction::RegistryEntry action;

			class SceneHeirarchyViewSerializer : public virtual EditorStorageSerializer::Of<ComponentInspector> {
			private:
				inline static bool GetComponentIndex(Component* parent, Component* component, uint64_t& counter) {
					if (parent == nullptr) return false;
					else if (parent == component) return true;
					counter++;
					for (size_t i = 0; i < parent->ChildCount(); i++)
						if (GetComponentIndex(parent->GetChild(i), component, counter)) return true;
					return false;
				}

				inline static Component* FindComponentByIndex(Component* parent, uint64_t index, uint64_t& counter) {
					if (parent == nullptr) return nullptr;
					else if (counter == index) return parent;
					counter++;
					for (size_t i = 0; i < parent->ChildCount(); i++) {
						Component* result = FindComponentByIndex(parent->GetChild(i), index, counter);
						if (result != nullptr) return result;
					}
					return nullptr;
				}
			
			public:
				inline SceneHeirarchyViewSerializer() : Serialization::ItemSerializer("ComponentInspector", "Component Inspector (Editor Window)") {}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, ComponentInspector* target)const final override {
					EditorWindow::Serializer()->GetFields(recordElement, target);
					const Reference<EditorScene> scene = target->Scene();
					const Reference<Component> rootComponent = (scene == nullptr) ? nullptr : scene->RootObject();
					const Reference<Component> targetComponent = target->Target();
					uint64_t index = 0;
					if (!GetComponentIndex(rootComponent, targetComponent, index))
						index = UINT64_MAX;
					static const Reference<const Serialization::ItemSerializer::Of<uint64_t>> serializer =
						Serialization::Uint64Serializer::Create("Component Index", "Component Index");
					recordElement(serializer->Serialize(index));
					uint64_t counter = 0;
					target->SetTarget((index > 0) ? FindComponentByIndex(rootComponent, index, counter) : nullptr);
				}
			};
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::ComponentInspector>(const Callback<TypeId>& report) {
		report(TypeId::Of<Editor::EditorSceneWindow>());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::ComponentInspector>(const Callback<const Object*>& report) {
		static const Editor::SceneHeirarchyViewSerializer instance;
		report(&instance);
	}
	template<> void TypeIdDetails::OnRegisterType<Editor::ComponentInspector>() {
		Editor::action = &Editor::editorMenuCallback;
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::ComponentInspector>() {
		Editor::action = nullptr;
	}
}
