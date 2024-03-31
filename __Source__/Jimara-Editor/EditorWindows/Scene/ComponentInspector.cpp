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
			Reference<ComponentFactory::Set> factories = ComponentFactory::All();
			auto drawTargetInspector = [&](Component* target) {
				const ComponentFactory* factory = factories->FindFactory(target);
				bool rv;
				if (factory != nullptr) {
					{
						const std::string label(factory->InstanceType().Name());
						ImGui::LabelText("", label.c_str());
						ImGui::Separator();
					}
					rv = true;
				}
				else rv = false;
				{
					static const Serialization::Serializable::Serializer serializer("Component Serializer");
					if (DrawSerializedObject(serializer.Serialize(target), (size_t)this, editorScene->RootObject()->Context()->Log(), [&](const Serialization::SerializedObject& object) {
						const std::string name = CustomSerializedObjectDrawer::DefaultGuiItemName(object, (size_t)this);
						static thread_local std::vector<char> searchBuffer;
						return DrawObjectPicker(object, name, editorScene->RootObject()->Context()->Log(), editorScene->RootObject(), Context()->EditorAssetDatabase(), &searchBuffer);
						})) {
						editorScene->TrackComponent(target, false);
					}
				}
				return rv;
			};

			// Draw a single target if we have Target set:
			Reference<Component> target = Target();
			if (target != nullptr) {
				drawTargetInspector(target);
				return;
			}

			// Get and sort selection:
			Stacktor<Reference<Component>, 8u> selection;
			{
				editorScene->Selection()->Iterate([&](Component* component) {
					if (component != nullptr)
						selection.Push(component);
					});
				auto constructParentChain = [&](const Component* component, auto& chain) {
					chain.Clear();
					while (component != nullptr) {
						chain.Push(component->IndexInParent());
						component = component->Parent();
					}
					std::reverse(chain.Data(), chain.Data() + chain.Size());
				};
				Stacktor<size_t, 8u> parentChainA;
				Stacktor<size_t, 8u> parentChainB;
				std::sort(selection.Data(), selection.Data() + selection.Size(),
					[&](Component* componentA, Component* componentB) {
						constructParentChain(componentA, parentChainA);
						constructParentChain(componentB, parentChainB);
						const size_t minLength = Math::Min(parentChainA.Size(), parentChainB.Size());
						for (size_t i = 0u; i < minLength; i++) {
							const size_t indexA = parentChainA[i];
							const size_t indexB = parentChainB[i];
							if (indexA < indexB)
								return true;
							else if (indexA > indexB)
								return false;
						}
						return parentChainA.Size() < parentChainB.Size();
					});
			}

			// Draw selection:
			for (size_t i = 0u; i < selection.Size(); i++) {
				Component* component = selection[i];
				if (editorScene->Selection()->Count() > 1) {
					const std::string text = [&]() {
						std::stringstream stream;
						stream << component->Name() << "###component_inspector_view_view_" << ((size_t)this) << "_selection_tree_node" << ((size_t)component);
						return stream.str();
						}();
						if (editorScene->Selection()->Count() <= 8u)
							ImGui::SetNextItemOpen(true, ImGuiTreeNodeFlags_DefaultOpen);
						if (ImGui::TreeNode(text.c_str())) {
							drawTargetInspector(component);
							ImGui::TreePop();
						}
				}
				else drawTargetInspector(component);
			}
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
		static const Editor::EditorMainMenuCallback editorMenuCallback(
			"Scene/Component Inspector", "Open Component Inspector window for selection", Callback<Editor::EditorContext*>([](Editor::EditorContext* context) {
				Object::Instantiate<Editor::ComponentInspector>(context, nullptr);
				}));
		report(&editorMenuCallback);
	}
}
