#include "SceneHeirarchyView.h"


namespace Jimara {
	namespace Editor {
		SceneHeirarchyView::SceneHeirarchyView(EditorContext* context) 
			: EditorSceneController(context), EditorWindow(context, "Scene Heirarchy") {
			std::stringstream stream;
			stream << "Scene Heirarchy###editor_heirarchy_view_" << ((size_t)this);
			EditorWindowName() = stream.str();
		}

		namespace {
			static const char ADD_COMPONENT_POPUP[] = "Add Component###editor_heirarchy_view_AddComponentPopup";

			static Reference<Component> addChildTarget;
			static SpinLock addParentLock;
			inline static void SetAddComponentParent(Component* component) {
				static const Callback<Component*> clearTargetOnDelete([](Component* component) {
					std::unique_lock<SpinLock> lock(addParentLock);
					if (component != addChildTarget) return;
					else addChildTarget = nullptr;
					});
				std::unique_lock<SpinLock> lock(addParentLock);
				if (addChildTarget == component) return;
				
				if (addChildTarget != nullptr)
					addChildTarget->OnDestroyed() -= clearTargetOnDelete;

				addChildTarget = component;
				if (addChildTarget != nullptr)
					addChildTarget->OnDestroyed() += clearTargetOnDelete;
			}
			inline static Reference<Component> GetAddComponentParent() {
				std::unique_lock<SpinLock> lock(addParentLock);
				Reference<Component> target = addChildTarget;
				return target;
			}

			struct DrawHeirarchyState {
				const SceneHeirarchyView* view;
				const Reference<const ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();
				bool addComponentPopupDrawn = false;

				inline DrawHeirarchyState(const SceneHeirarchyView* v) : view(v) {}
			};

			static const void DrawAddComponentMenu(Jimara::Component* component, DrawHeirarchyState& state) {
				const std::string text = [&]() {
					std::stringstream stream;
					stream << ADD_COMPONENT_POPUP << "###editor_heirarchy_view_" << ((size_t)state.view) << "_add_component_btn_" << ((size_t)component);
					return stream.str();
				}();
				if (ImGui::Button(text.c_str())) {
					SetAddComponentParent(component);
					ImGui::OpenPopup(ADD_COMPONENT_POPUP);
				}
				if (state.addComponentPopupDrawn) return;
				else if (!ImGui::BeginPopup(ADD_COMPONENT_POPUP)) return;
				state.addComponentPopupDrawn = true;
				if (GetAddComponentParent() == nullptr) {
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}
				ImGui::Text("Add Component");
				ImGui::Separator();
				for (size_t i = 0; i < state.serializers->Size(); i++) {
					const Jimara::ComponentSerializer* serializer = state.serializers->At(i);
					if (GetAddComponentParent() == nullptr) break;
					else if (ImGui::Selectable(serializer->TargetName().c_str())) {
						serializer->CreateComponent(GetAddComponentParent());
						SetAddComponentParent(nullptr);
					}
				}
				ImGui::EndPopup();
			};

			inline static void DrawDeleteComponentButton(Component* component, DrawHeirarchyState& state) {
				const std::string text = [&]() {
					std::stringstream stream;
					stream << "Delete###editor_heirarchy_view_" << ((size_t)state.view) << "_delete_btn_" << ((size_t)component);
					return stream.str();
				}();
				if (ImGui::Button(text.c_str())) component->Destroy();
			}

			inline static void DrawObjectHeirarchy(Component* root, DrawHeirarchyState& state) {
				for (size_t i = 0; i < root->ChildCount(); i++) {
					Component* child = root->GetChild(i);
					const std::string text = [&]() {
						std::stringstream stream;
						stream << child->Name() << "###editor_heirarchy_view_" << ((size_t)state.view) << "_child_tree_node" << ((size_t)child);
						return stream.str();
					}();
					if (ImGui::TreeNode(text.c_str())) {
						DrawObjectHeirarchy(child, state);
						ImGui::TreePop();
					}
				}
				// __TODO__: Maybe, some way to drag and drop could be incorporated here...
				DrawDeleteComponentButton(root, state);
				ImGui::SameLine();
				DrawAddComponentMenu(root, state);
			}
		}

		void SceneHeirarchyView::DrawEditorWindow() {
			Reference<EditorScene> editorScene = GetOrCreateScene();
			std::unique_lock<std::recursive_mutex> lock(editorScene->UpdateLock());
			DrawHeirarchyState state(this);
			DrawObjectHeirarchy(editorScene->RootObject(), state);
		}

		namespace {
			static const EditorMainMenuCallback editorMenuCallback(
				"Scene/Heirarchy", Callback<EditorContext*>([](EditorContext* context) {
					Reference<SceneHeirarchyView> view = Object::Instantiate<SceneHeirarchyView>(context);
					context->AddRenderJob(view);
					}));
			static EditorMainMenuAction::RegistryEntry action;
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::SceneHeirarchyView>(const Callback<TypeId>& report) {
		report(TypeId::Of<Editor::EditorSceneController>());
		report(TypeId::Of<Editor::EditorWindow>());
	}
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneHeirarchyView>() {
		Editor::action = &Editor::editorMenuCallback;
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneHeirarchyView>() {
		Editor::action = nullptr;
	}
}
