#include "SceneHeirarchyView.h"
#include "ComponentInspector.h"


namespace Jimara {
	namespace Editor {
		SceneHeirarchyView::SceneHeirarchyView(EditorContext* context) 
			: EditorSceneController(context), EditorWindow(context, "Scene Heirarchy")
			, m_addComponentPopupName([&]() {
			std::stringstream stream;
			stream << "Add Component###editor_heirarchy_view_AddComponentPopup_for" << ((size_t)this);
			return stream.str();
				}()) {
			std::stringstream stream;
			stream << "Scene Heirarchy###editor_heirarchy_view_" << ((size_t)this);
			EditorWindowName() = stream.str();
		}

		namespace {
			struct DrawHeirarchyState {
				const SceneHeirarchyView* view = nullptr;
				Reference<Component>* addChildTarget = nullptr;
				const char* AddComponentPopupId = nullptr;

				const Reference<const ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();
				bool addComponentPopupDrawn = false;
			};

			inline static void SetAddComponentParent(Component* component, DrawHeirarchyState& state) {
				static void (*clearCallback)(Reference<Component>*, Component*) = [](Reference<Component>* current, Component* deleted) {
					if ((*current) == deleted)
						(*current) = nullptr;
				};
				const Callback<Component*> clearTargetOnDelete(clearCallback, state.addChildTarget);
				if ((*state.addChildTarget) == component) return;

				if ((*state.addChildTarget) != nullptr)
					(*state.addChildTarget)->OnDestroyed() -= clearTargetOnDelete;

				(*state.addChildTarget) = component;
				if ((*state.addChildTarget) != nullptr)
					(*state.addChildTarget)->OnDestroyed() += clearTargetOnDelete;
			}

			static const void DrawAddComponentMenu(Jimara::Component* component, DrawHeirarchyState& state) {
				const std::string text = [&]() {
					std::stringstream stream;
					stream << "Add Component###editor_heirarchy_view_" << ((size_t)state.view) << "_add_component_btn_" << ((size_t)component);
					return stream.str();
				}();
				if (ImGui::Button(text.c_str())) {
					SetAddComponentParent(component, state);
					ImGui::OpenPopup(state.AddComponentPopupId);
				}
				if (state.addComponentPopupDrawn) return;
				else if (!ImGui::BeginPopup(state.AddComponentPopupId)) return;
				state.addComponentPopupDrawn = true;
				if ((*state.addChildTarget) == nullptr) {
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}
				ImGui::Text("Add Component");
				ImGui::Separator();
				for (size_t i = 0; i < state.serializers->Size(); i++) {
					const Jimara::ComponentSerializer* serializer = state.serializers->At(i);
					if ((*state.addChildTarget) == nullptr) break;
					else if (ImGui::Selectable(serializer->TargetName().c_str())) {
						serializer->CreateComponent(*state.addChildTarget);
						SetAddComponentParent(nullptr, state);
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

			inline static void DrawEditComponentButton(Component* component, DrawHeirarchyState& state) {
				const std::string text = [&]() {
					std::stringstream stream;
					stream << "Edit###editor_heirarchy_view_" << ((size_t)state.view) << "_edit_btn_" << ((size_t)component);
					return stream.str();
				}();
				if (ImGui::Button(text.c_str())) Object::Instantiate<ComponentInspector>(state.view->Context(), component);
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
				if (root->RootObject() != root) {
					DrawDeleteComponentButton(root, state);
					ImGui::SameLine();
				}
				DrawAddComponentMenu(root, state);
				if (state.serializers->FindSerializerOf(root) != nullptr) {
					ImGui::SameLine();
					DrawEditComponentButton(root, state);
				}
			}
		}

		void SceneHeirarchyView::DrawEditorWindow() {
			Reference<EditorScene> editorScene = GetOrCreateScene();
			std::unique_lock<std::recursive_mutex> lock(editorScene->UpdateLock());
			DrawHeirarchyState state;
			state.view = this;
			state.addChildTarget = &m_addChildTarget;
			state.AddComponentPopupId = m_addComponentPopupName.c_str();
			DrawObjectHeirarchy(editorScene->RootObject(), state);
		}

		namespace {
			static const EditorMainMenuCallback editorMenuCallback(
				"Scene/Heirarchy", Callback<EditorContext*>([](EditorContext* context) {
					Object::Instantiate<SceneHeirarchyView>(context);
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
