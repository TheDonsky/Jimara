#include "SceneHeirarchyView.h"
#include "ComponentInspector.h"
#include "../../GUI/Utils/DrawTooltip.h"
#include "../../GUI/Utils/DrawMenuAction.h"
#include "../../GUI/Utils/DrawSerializedObject.h"
#include "../../Environment/EditorStorage.h"
#include <Data/ComponentHeirarchySpowner.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>
#include <IconFontCppHeaders/IconsMaterialDesign.h>
#include <Core/Stopwatch.h>


namespace Jimara {
	namespace Editor {
		SceneHeirarchyView::SceneHeirarchyView(EditorContext* context) 
			: EditorSceneController(context), EditorWindow(context, "Scene Heirarchy")
			, m_addComponentPopupName([&]() {
			std::stringstream stream;
			stream << "Add Component###editor_heirarchy_view_AddComponentPopup_for" << ((size_t)this);
			return stream.str();
				}()) {}

		struct SceneHeirarchyView::Tools {
			struct DrawHeirarchyState {
				SceneHeirarchyView* view = nullptr;
				EditorScene* scene = nullptr;

				const Reference<const ComponentSerializer::Set> serializers = ComponentSerializer::Set::All();
				bool addComponentPopupDrawn = false;
			};

			inline static bool CtrlPressed(const DrawHeirarchyState& state) {
				return
					state.view->Context()->InputModule()->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL) ||
					state.view->Context()->InputModule()->KeyPressed(OS::Input::KeyCode::RIGHT_CONTROL);
			}

			inline static const std::string_view SceneHeirarchyView_DRAG_DROP_TYPE = "SceneHeirarchyView_DRAG_TYPE";

			template<typename Process>
			inline static void AcceptDragAndDropTarget(DrawHeirarchyState& state, const Process& process) {
				if (ImGui::BeginDragDropTarget()) {
					const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(SceneHeirarchyView_DRAG_DROP_TYPE.data());
					if (payload != nullptr &&
						payload->DataSize == sizeof(SceneHeirarchyView*) &&
						((SceneHeirarchyView**)payload->Data)[0] == state.view)
						process(state.scene->Selection()->Current());
					ImGui::EndDragDropTarget();
				}
			}

			inline static void DrawComponentHeirarchySpownerSelector(Jimara::Component* component, DrawHeirarchyState& state) {
				ImGui::Separator();
				state.view->Context()->EditorAssetDatabase()->GetAssetsOfType<ComponentHeirarchySpowner>(
					[&](const FileSystemDatabase::AssetInformation& info) {
						std::string path = info.SourceFilePath();
						{
							size_t count = 0;
							state.view->Context()->EditorAssetDatabase()->GetAssetsFromFile<ComponentHeirarchySpowner>(
								info.SourceFilePath(), [&](const FileSystemDatabase::AssetInformation&) { count++; });
							if (count > 1)
								path += "/" + info.ResourceName();
						}
						if (DrawMenuAction(path, path, info.AssetRecord())) {
							Stopwatch totalTime;
							Stopwatch stopwatch;
							auto logProgress = [&](Asset::LoadInfo progress) {
								if (stopwatch.Elapsed() < 0.25f && progress.Fraction() < 1.0f) return;
								stopwatch.Reset();
								state.view->Context()->Log()->Info(
									"Loading '", path, "': ", (progress.Fraction() * 100.0f), "% [", 
									progress.stepsTaken, " / ", progress.totalSteps, "] (", 
									totalTime.Elapsed(), " sec...)");
							};
							Reference<ComponentHeirarchySpowner> spowner = info.AssetRecord()->LoadResource(
								Callback<Asset::LoadInfo>::FromCall(&logProgress));
							if (spowner != nullptr) {
								Reference<Component> substree = spowner->SpownHeirarchy(component);
								state.scene->TrackComponent(substree, true);
								state.view->m_addChildTarget = nullptr;
							}
						}
					});
			}

			inline static void DrawAddComponentMenu(Jimara::Component* component, DrawHeirarchyState& state) {
				const std::string text = [&]() {
					std::stringstream stream;
					stream << ICON_FA_PLUS << " Add Component###editor_heirarchy_view_" << ((size_t)state.view) << "_add_component_btn_" << ((size_t)component);
					return stream.str();
				}();
				const bool buttonClicked = ImGui::Button(text.c_str());
				DrawTooltip(text.c_str(), "Click to add [sub]-components or prefabricated/loaded component heirarchies");
				if (buttonClicked) {
					state.view->m_addChildTarget = component;
					ImGui::OpenPopup(state.view->m_addComponentPopupName.c_str());
				}
				if (state.addComponentPopupDrawn) return;
				else if (!ImGui::BeginPopup(state.view->m_addComponentPopupName.c_str())) return;
				state.addComponentPopupDrawn = true;
				if (state.view->m_addChildTarget == nullptr) {
					ImGui::CloseCurrentPopup();
					ImGui::EndPopup();
					return;
				}
				ImGui::Text("Add Component");
				ImGui::Separator();
				for (size_t i = 0; i < state.serializers->Size(); i++) {
					const Jimara::ComponentSerializer* serializer = state.serializers->At(i);
					if (state.view->m_addChildTarget == nullptr) break;
					else if (DrawMenuAction(serializer->TargetName().c_str(), serializer->TargetHint(), serializer)) {
						Reference<Component> component = serializer->CreateComponent(state.view->m_addChildTarget);
						state.scene->TrackComponent(component, true);
						state.view->m_addChildTarget = nullptr;
					}
				}
				DrawComponentHeirarchySpownerSelector(component, state);
				ImGui::EndPopup();
			};

			inline static void DrawEditNameField(Component* component, DrawHeirarchyState& state, float reservedWidth) {
				ImGui::SameLine();
				{
					float indent = ImGui::GetItemRectMin().x - ImGui::GetWindowPos().x;
					ImGui::PushItemWidth(ImGui::GetWindowWidth() - indent - 32.0f - reservedWidth);
				}
				if (state.view->m_componentBeingRenamed.reference == component) {
					static const Reference<const Serialization::ItemSerializer::Of<Component>> serializer = Serialization::ValueSerializer<std::string_view>::Create<Component>(
						"", "<Name>",
						Function<std::string_view, Component*>([](Component* target) -> std::string_view { return target->Name(); }),
						Callback<const std::string_view&, Component*>([](const std::string_view& value, Component* target) { target->Name() = value; }));
					const std::string initialName = component->Name();
					const bool componentBeingRenamedIsNew = state.view->m_componentBeingRenamed.justStartedRenaming;
					if (componentBeingRenamedIsNew) {
						ImGui::SetKeyboardFocusHere();
						state.view->m_componentBeingRenamed.justStartedRenaming = false;
					}
					if (DrawSerializedObject(serializer->Serialize(component), (size_t)state.view, state.view->Context()->Log(),
						[](const Serialization::SerializedObject&) { return false; })) {
						state.scene->TrackComponent(component, false);
						state.view->m_componentBeingRenamed.reference = nullptr;
					}
					else if ((!componentBeingRenamedIsNew) && (!ImGui::IsItemActivated()) && (!ImGui::IsItemActive()))
						state.view->m_componentBeingRenamed.reference = nullptr;
				}
				else {
					ImGui::Selectable(component->Name().c_str(), state.scene->Selection()->Contains(component), 0, ImVec2(ImGui::CalcItemWidth(), 0.0f));
					if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
						state.view->m_componentBeingRenamed.reference = component;
						state.view->m_componentBeingRenamed.justStartedRenaming = true;
					}
				}
				
				// Selection:
				if (ImGui::IsItemClicked()) {
					if (!CtrlPressed(state)) {
						state.scene->Selection()->DeselectAll();
						state.scene->Selection()->Select(component);
					}
					else if (state.scene->Selection()->Contains(component))
						state.scene->Selection()->Deselect(component);
					else state.scene->Selection()->Select(component);
				}
				
				// Drag & Drop Start:
				if (ImGui::BeginDragDropSource()) {
					state.scene->Selection()->Select(component);
					ImGui::SetDragDropPayload(SceneHeirarchyView_DRAG_DROP_TYPE.data(), &state.view, sizeof(SceneHeirarchyView*));
					ImGui::Text(component->Name().c_str());
					ImGui::EndDragDropSource();
				}

				// Drag & Drop End:
				AcceptDragAndDropTarget(state, [&](const auto& draggedComponents) {
					for (const auto& draggedComponent : draggedComponents) {
						draggedComponent->SetParent(component);
						state.scene->TrackComponent(draggedComponent, false);
					}
					state.scene->TrackComponent(component, false);
					});

				ImGui::PopItemWidth();
			}

			inline static void DrawEnabledCheckbox(Component* component, DrawHeirarchyState& state) {
				const std::string text = [&]() {
					std::stringstream stream;
					stream << "###editor_heirarchy_view_" << ((size_t)state.view) << "_enabled_checkbox_" << ((size_t)component);
					return stream.str();
				}();
				bool enabled = component->Enabled();
				if (ImGui::Checkbox(text.c_str(), &enabled)) {
					component->SetEnabled(enabled);
					state.scene->TrackComponent(component, false);
				}
				DrawTooltip(text.c_str(), "Disable/Enable the component");
			}

			inline static void DrawDeleteComponentButton(Component* component, DrawHeirarchyState& state) {
				ImGui::SameLine();
				const std::string text = [&]() {
					std::stringstream stream;
					stream << ICON_FA_MINUS_CIRCLE << "###editor_heirarchy_view_" << ((size_t)state.view) << "_delete_btn_" << ((size_t)component);
					return stream.str();
				}();
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
				if (ImGui::Button(text.c_str())) {
					state.scene->TrackComponent(component, true);
					component->Destroy();
				}
				ImGui::PopStyleColor();
				DrawTooltip(text.c_str(), "Destroy the component");
			}

			inline static void DrawEditComponentButton(Component* component, DrawHeirarchyState& state) {
				ImGui::SameLine();
				const std::string text = [&]() {
					std::stringstream stream;
					stream << ICON_FA_EDIT << "###editor_heirarchy_view_" << ((size_t)state.view) << "_edit_btn_" << ((size_t)component);
					return stream.str();
				}();
				if (ImGui::Button(text.c_str())) 
					Object::Instantiate<ComponentInspector>(state.view->Context(), component);
				DrawTooltip(text.c_str(), "Open separate inspector window for the component");
			}

			inline static void DragComponent(Component* component, DrawHeirarchyState& state) {
				// Drag & Drop End:
				AcceptDragAndDropTarget(state, [&](const auto& draggedComponents) {
					for (size_t i = 0; i < draggedComponents.size(); i++) {
						Component* draggedComponent = draggedComponents[i];
						draggedComponent->SetParent(component->Parent());
						draggedComponent->SetIndexInParent(component->IndexInParent() + i + 1);
						state.scene->TrackComponent(draggedComponent, false);
					}
					state.scene->TrackComponent(component->Parent(), false);
					});
			}

			inline static void DrawPopupContextMenu(Component* component, DrawHeirarchyState& state) {
				const bool isRoot = (component == component->Context()->RootObject() || component->Parent() == nullptr);
				if ((!(isRoot ? ((!ImGui::IsAnyItemHovered()) && ImGui::IsWindowHovered()) : ImGui::IsItemHovered()))
					&& (state.view->m_rightClickMenuTarget != component)) return;
				else if (ImGui::BeginPopupContextWindow())
					state.view->m_rightClickMenuTarget = component; 
				else {
					state.view->m_rightClickMenuTarget = nullptr;
					return;
				}

				// Rename:
				if (!isRoot)
					if (ImGui::MenuItem("Rename")) {
						state.view->m_rightClickMenuTarget = nullptr;
						state.view->m_componentBeingRenamed.reference = component;
						state.view->m_componentBeingRenamed.justStartedRenaming = true;
					}

				// Delete:
				if (!isRoot)
					if (ImGui::MenuItem("Delete"))
						component->Destroy();

				// Delete selection:
				if (ImGui::MenuItem("Delete Selection")) {
					const auto selection = state.scene->Selection()->Current();
					for (const auto& element : selection) element->Destroy();
				}

				// Copy:
				if (!isRoot)
					if (ImGui::MenuItem("Copy"))
						state.scene->Clipboard()->CopyComponents(component);
				
				// Copy selection:
				if (ImGui::MenuItem("Copy Selection"))
					state.scene->Clipboard()->CopyComponents(state.scene->Selection()->Current());
				DrawTooltip("Copy Selection (SceneHeirarchy_ContextMenu)", "CTRL + C");

				// Cut:
				if (!isRoot)
					if (ImGui::MenuItem("Cut")) {
						state.scene->Clipboard()->CopyComponents(component);
						component->Destroy();
					}

				// Cut selection:
				if (ImGui::MenuItem("Cut selection")) {
					const auto selection = state.scene->Selection()->Current();
					state.scene->Clipboard()->CopyComponents(selection);
					for (const auto& element : selection) element->Destroy();
				}
				DrawTooltip("Cut Selection (SceneHeirarchy_ContextMenu)", "CTRL + X");

				// Paste:
				if (ImGui::MenuItem("Paste"))
					state.scene->Clipboard()->PasteComponents(isRoot ? component : component->Parent());
				DrawTooltip("Paste (SceneHeirarchy_ContextMenu)", "CTRL + V");

				// Paste as child(ren):
				if (!isRoot) {
					if (ImGui::MenuItem("Paste as children"))
						state.scene->Clipboard()->PasteComponents(component);
					DrawTooltip("Paste as children (SceneHeirarchy_ContextMenu)", "CTRL + V");
				}

				// Add component:
				const bool openAddComponentPopup = ImGui::MenuItem("Add Component");

				ImGui::EndPopup();

				// Apply "Add component" action:
				if (openAddComponentPopup) {
					state.view->m_rightClickMenuTarget = nullptr;
					state.view->m_addChildTarget = component;
					ImGui::OpenPopup(state.view->m_addComponentPopupName.c_str());
				}
			}

			inline static void DrawObjectHeirarchy(Component* root, DrawHeirarchyState& state) {
				for (size_t i = 0; i < root->ChildCount(); i++) {
					Component* child = root->GetChild(i);
					const std::string text = [&]() {
						std::stringstream stream;
						stream << "###editor_heirarchy_view_" << ((size_t)state.view) << "_child_tree_node" << ((size_t)child);
						return stream.str();
					}();
					const ComponentSerializer* serializer = state.serializers->FindSerializerOf(child);
					
					bool disabled = (!child->Enabled());
					if (disabled)
						ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);

					// Tree node:
					bool treeNodeExpanded = ImGui::TreeNodeEx(text.c_str(),
						ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding |
						(state.scene->Selection()->Contains(child) ? ImGuiTreeNodeFlags_Selected : 0));
					
					if (serializer != nullptr)
						DrawTooltip(text.c_str(), serializer->TargetName());
					
					DragComponent(child, state);

					// Text and button editors:
					{
						const constexpr bool drawEnableButton = true;
						const constexpr bool drawDeleteButton = false;
						const constexpr bool drawEditButton = true;
						const constexpr float singleButtonWidth = 32.0f;
						const constexpr float totalButtonWidth = singleButtonWidth * (
							(drawEnableButton ? 1.0f : 0.0f) +
							(drawDeleteButton ? 1.0f : 0.0f) +
							(drawEditButton ? 1.0f : 0.0f));

						DrawEditNameField(child, state, totalButtonWidth);
						DrawPopupContextMenu(child, state);

						ImGui::SameLine(ImGui::GetWindowWidth() - totalButtonWidth);
						if (drawEnableButton) DrawEnabledCheckbox(child, state);
						if (drawDeleteButton) DrawDeleteComponentButton(child, state);
						if (drawEditButton && serializer != nullptr) DrawEditComponentButton(child, state);
					}

					// Recursion:
					if (treeNodeExpanded) {
						DrawObjectHeirarchy(child, state);
						ImGui::TreePop();
					}

					if (disabled)
						ImGui::PopStyleVar();
				}
				// __TODO__: Maybe, some way to drag and drop could be incorporated here...
				DrawAddComponentMenu(root, state);
			}
		};

		void SceneHeirarchyView::DrawEditorWindow() {
			Reference<EditorScene> editorScene = GetOrCreateScene();
			std::unique_lock<std::recursive_mutex> lock(editorScene->UpdateLock());
			
			// Make sure we do not hold dead references:
			auto clearIfDestroyedOrFromAnotherContext = [&](Reference<Component>& component) {
				if (component == nullptr) return;
				else if (component->Destroyed() || component->Context() != editorScene->RootObject()->Context())
					component = nullptr;
			};
			clearIfDestroyedOrFromAnotherContext(m_addChildTarget);
			clearIfDestroyedOrFromAnotherContext(m_componentBeingRenamed.reference);
			clearIfDestroyedOrFromAnotherContext(m_rightClickMenuTarget);
			
			// Draw editor window
			Tools::DrawHeirarchyState state;
			{
				state.view = this;
				state.scene = editorScene;
			}
			Tools::DrawObjectHeirarchy(editorScene->RootObject(), state);
			Tools::DrawPopupContextMenu(editorScene->RootObject(), state);

			// Deselect everything if clicked on empty space:
			if (ImGui::IsWindowFocused() && 
				ImGui::IsMouseClicked(ImGuiMouseButton_Left) && 
				ImGui::IsWindowHovered() && 
				(!ImGui::IsAnyItemActive()) && 
				(!Tools::CtrlPressed(state)))
				editorScene->Selection()->DeselectAll();

			// Delete selected elements if delete key is down:
			if (ImGui::IsWindowFocused() && 
				Context()->InputModule()->KeyDown(OS::Input::KeyCode::DELETE_KEY)) {
				const auto selection = editorScene->Selection()->Current();
				for (const auto& component : selection) component->Destroy();
			}

			// CTRL+C/X/V
			if (ImGui::IsWindowFocused() && (!ImGui::IsAnyItemActive()) &&
				(Context()->InputModule()->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL) |
					Context()->InputModule()->KeyPressed(OS::Input::KeyCode::RIGHT_CONTROL))) { 
				if (Context()->InputModule()->KeyDown(OS::Input::KeyCode::C)) {
					if (editorScene->Selection()->Count() > 0)
						editorScene->Clipboard()->CopyComponents(editorScene->Selection()->Current());
				}
				else if (Context()->InputModule()->KeyDown(OS::Input::KeyCode::X)) {
					const auto selection = editorScene->Selection()->Current();
					if (!selection.empty())
						editorScene->Clipboard()->CopyComponents(editorScene->Selection()->Current());
					for (const auto& component : selection) component->Destroy();
				}
				else if (Context()->InputModule()->KeyDown(OS::Input::KeyCode::V)) {
					Component* root;
					if (editorScene->Selection()->Count() == 1) {
						editorScene->Selection()->Iterate([&](Component* component) { root = component; });
						if (root->Parent() != nullptr) root = root->Parent();
					}
					else root = editorScene->RootObject();
					const auto newInstances = editorScene->Clipboard()->PasteComponents(root);
					editorScene->Selection()->DeselectAll();
					for (const auto& component : newInstances) {
						editorScene->TrackComponent(component, true);
						editorScene->Selection()->Select(component);
					}
				}
			}
		}

		namespace {
			static const EditorMainMenuCallback editorMenuCallback(
				"Scene/Heirarchy", "Open Scene hairarchy view (displays and lets edit scene graph)", Callback<EditorContext*>([](EditorContext* context) {
					Object::Instantiate<SceneHeirarchyView>(context);
					}));
			static EditorMainMenuAction::RegistryEntry action;
		}

		namespace {
			class SceneHeirarchyViewSerializer : public virtual EditorStorageSerializer::Of<SceneHeirarchyView> {
			public:
				inline SceneHeirarchyViewSerializer() : Serialization::ItemSerializer("SceneHeirarchyView", "Scene Heirarchy View (Editor Window)") {}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, SceneHeirarchyView* target)const final override {
					EditorWindow::Serializer()->GetFields(recordElement, target);
				}
			};
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::SceneHeirarchyView>(const Callback<TypeId>& report) {
		report(TypeId::Of<Editor::EditorSceneWindow>());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::SceneHeirarchyView>(const Callback<const Object*>& report) {
		static const Editor::SceneHeirarchyViewSerializer serializer;
		report(&serializer);
	}
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneHeirarchyView>() {
		Editor::action = &Editor::editorMenuCallback;
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneHeirarchyView>() {
		Editor::action = nullptr;
	}
}
