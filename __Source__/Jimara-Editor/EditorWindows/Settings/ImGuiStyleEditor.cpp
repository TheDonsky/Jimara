#include "ImGuiStyleEditor.h"
#include "../../GUI/Utils/DrawSerializedObject.h"
#include <Data/Serialization/Attributes/ColorAttribute.h>
#include <Data/Serialization/Attributes/SliderAttribute.h>
#include <Data/Serialization/Attributes/EnumAttribute.h>
#include <Data/Serialization/Helpers/SerializeToJson.h>
#include <OS/IO/FileDialogues.h>
#include <OS/IO/MMappedFile.h>
#include <IconFontCppHeaders/IconsFontAwesome4.h>
#include <fstream>


namespace Jimara {
	namespace Editor {
		namespace {
			static EventInstance<EditorContext*> eraseImGuiStyleUndoActionStack;

			class ImGuiStyleUndoAction : public virtual UndoStack::Action {
			private:
				const nlohmann::json m_oldData;
				Reference<EditorContext> m_context;

				inline void Invalidate(EditorContext* context) { 
					if (m_context != m_context) return;
					eraseImGuiStyleUndoActionStack.operator Jimara::Event<Jimara::Editor::EditorContext *> &() -= Callback(&ImGuiStyleUndoAction::Invalidate, this);
					m_context = nullptr;
				}

			public:
				inline static nlohmann::json CreateSnapshot() {
					ImGuiStyle& style = ImGui::GetStyle();
					bool error = false;
					return Serialization::SerializeToJson(
						ImGuiStyleEditor::StyleSerializer()->Serialize(style), nullptr, error,
						[](const auto&, bool& err) { err = true; return nlohmann::json(); });
				}

				inline static void LoadSnapshot(const nlohmann::json& snapshot) {
					ImGuiStyle& style = ImGui::GetStyle();
					Serialization::DeserializeFromJson(
						ImGuiStyleEditor::StyleSerializer()->Serialize(style), snapshot, nullptr,
						[](const auto&, const auto&) { return false; });
				}

				inline ImGuiStyleUndoAction(EditorContext* context, nlohmann::json& oldData) : m_context(context), m_oldData(oldData) {
					eraseImGuiStyleUndoActionStack.operator Jimara::Event<Jimara::Editor::EditorContext *> &() += Callback(&ImGuiStyleUndoAction::Invalidate, this);
				}

				inline virtual ~ImGuiStyleUndoAction() {
					Invalidate(m_context);
				}

				inline virtual bool Invalidated()const { return m_context == nullptr; }

				inline virtual void Undo() { if (!Invalidated()) LoadSnapshot(m_oldData); }
			};

#pragma warning (disable: 26812)
			class ImGuiStyleSerializer : public virtual Serialization::SerializerList::From<ImGuiStyle> {
			private:
				typedef Reference<const Serialization::ItemSerializer::Of<ImGuiStyle>> FieldSerializer;

				inline static FieldSerializer CreateColorSerializer(
					const std::string_view& name, const std::string_view& hint, ImGuiCol_ color) {
					static_assert(sizeof(int) <= sizeof(void*));
					void* colorAsAddr = (void*)((size_t)color);
					Vector4(*getFn)(void*, ImGuiStyle*) = [](void* colorAsAddr, ImGuiStyle* style) {
						const ImVec4& color = style->Colors[(size_t)colorAsAddr];
						return Vector4(color.x, color.y, color.z, color.w);
					};
					void(*setFn)(void*, const Vector4&, ImGuiStyle*) = [](void* colorAsAddr, const Vector4& value, ImGuiStyle* style) {
						style->Colors[(size_t)colorAsAddr] = ImVec4(value.r, value.g, value.b, value.a);
					};
					return Serialization::ValueSerializer<Vector4>::Create<ImGuiStyle>(
						name, hint,
						Function<Vector4, ImGuiStyle*>(getFn, colorAsAddr),
						Callback<const Vector4&, ImGuiStyle*>(setFn, colorAsAddr),
						{ Object::Instantiate<Serialization::ColorAttribute>() });
				}

				class Group : public virtual Serialization::SerializerList::From<ImGuiStyle> {
				private:
					const std::vector<FieldSerializer> m_fields;

				public:
					inline Group(const std::string_view& name, const std::string_view& hint, const std::vector<FieldSerializer>& serializers) 
						: Serialization::ItemSerializer(name, hint), m_fields(serializers) {}

					inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, ImGuiStyle* style)const final override {
						for (size_t i = 0; i < m_fields.size(); i++) 
							recordElement(m_fields[i]->Serialize(style));
					}

					inline static FieldSerializer Create(const std::string_view& name, const std::string_view& hint, const std::vector<FieldSerializer>& serializers) {
						return Object::Instantiate<Group>(name, hint, serializers);
					}
				};

			public:
				inline ImGuiStyleSerializer() : Serialization::ItemSerializer("ImGui style", "You can edit ImGui style with this") {}

				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, ImGuiStyle* style)const final override {
					static const std::vector<FieldSerializer> colorSerializers = {
						Group::Create("Alpha", "Alpha and DisabledAlpha", {
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"Alpha", "Global alpha applies to everything in Dear ImGui",
								[](ImGuiStyle* style) -> float { return style->Alpha; },
								[](const float& value, ImGuiStyle* style) { style->Alpha = value; },
								{ Object::Instantiate<Serialization::SliderAttribute<float>>(0.1f, 1.0f) }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"DisabledAlpha", "Additional alpha multiplier applied by BeginDisabled(). Multiply over current value of Alpha",
								[](ImGuiStyle* style) -> float { return style->DisabledAlpha; },
								[](const float& value, ImGuiStyle* style) { style->DisabledAlpha = value; },
								{ Object::Instantiate<Serialization::SliderAttribute<float>>(0.1f, 1.0f) })
						}),
						Group::Create("Window", "Window related settings", {
							Serialization::ValueSerializer<Vector2>::For<ImGuiStyle>(
								"WindowPadding", "Padding within a window",
								[](ImGuiStyle* style) -> Vector2 { return Vector2(style->WindowPadding.x, style->WindowPadding.y); },
								[](const Vector2& value, ImGuiStyle* style) { style->WindowPadding = ImVec2(value.x, value.y); }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"WindowRounding", "Radius of window corners rounding. Set to 0.0f to have rectangular windows. Large values tend to lead to variety of artifacts and are not recommended",
								[](ImGuiStyle* style) -> float { return style->WindowRounding; },
								[](const float& value, ImGuiStyle* style) { style->WindowRounding = value; }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"WindowBorderSize", "Thickness of border around windows. Generally set to 0.0f or 1.0f. (Other values are not well tested and more CPU/GPU costly)",
								[](ImGuiStyle* style) -> float { return style->WindowBorderSize; },
								[](const float& value, ImGuiStyle* style) { style->WindowBorderSize = value; }),
							Serialization::ValueSerializer<Vector2>::For<ImGuiStyle>(
								"WindowMinSize", "Minimum window size. This is a global setting. If you want to constraint individual windows, use SetNextWindowSizeConstraints()",
								[](ImGuiStyle* style) -> Vector2 { return Vector2(style->WindowMinSize.x, style->WindowMinSize.y); },
								[](const Vector2& value, ImGuiStyle* style) { style->WindowMinSize = ImVec2(value.x, value.y); }),
							Serialization::ValueSerializer<Vector2>::For<ImGuiStyle>(
								"WindowTitleAlign", "Alignment for title bar text. Defaults to (0.0f,0.5f) for left-aligned,vertically centered",
								[](ImGuiStyle* style) -> Vector2 { return Vector2(style->WindowTitleAlign.x, style->WindowTitleAlign.y); },
								[](const Vector2& value, ImGuiStyle* style) { style->WindowTitleAlign = ImVec2(value.x, value.y); }),
							Serialization::ValueSerializer<ImGuiDir>::For<ImGuiStyle>(
								"WindowMenuButtonPosition", "Side of the collapsing/docking button in the title bar (None/Left/Right). Defaults to ImGuiDir_Left",
								[](ImGuiStyle* style) -> ImGuiDir { return style->WindowMenuButtonPosition; },
								[](const ImGuiDir& value, ImGuiStyle* style) { style->WindowMenuButtonPosition = value; },
								{ Object::Instantiate<Serialization::EnumAttribute<ImGuiDir>>(std::vector<Serialization::EnumAttribute<ImGuiDir>::Choice>{
									Serialization::EnumAttribute<ImGuiDir>::Choice("ImGuiDir_None", ImGuiDir_None),
									Serialization::EnumAttribute<ImGuiDir>::Choice("ImGuiDir_Left", ImGuiDir_Left),
									Serialization::EnumAttribute<ImGuiDir>::Choice("ImGuiDir_Right", ImGuiDir_Right)
									}, false) }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"ChildRounding", "Radius of child window corners rounding. Set to 0.0f to have rectangular windows",
								[](ImGuiStyle* style) -> float { return style->ChildRounding; },
								[](const float& value, ImGuiStyle* style) { style->ChildRounding = value; }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"ChildBorderSize", "Thickness of border around child windows. Generally set to 0.0f or 1.0f. (Other values are not well tested and more CPU/GPU costly)",
								[](ImGuiStyle* style) -> float { return style->ChildBorderSize; },
								[](const float& value, ImGuiStyle* style) { style->ChildBorderSize = value; })
						}),
						Group::Create("Popup", "Popup related settings", {
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"PopupRounding", "Radius of popup window corners rounding. (Note that tooltip windows use WindowRounding)",
								[](ImGuiStyle* style) -> float { return style->PopupRounding; },
								[](const float& value, ImGuiStyle* style) { style->PopupRounding = value; }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"PopupBorderSize", "Thickness of border around popup/tooltip windows. Generally set to 0.0f or 1.0f. (Other values are not well tested and more CPU/GPU costly)",
								[](ImGuiStyle* style) -> float { return style->PopupBorderSize; },
								[](const float& value, ImGuiStyle* style) { style->PopupBorderSize = value; })
						}),
						Group::Create("Frame", "Frame related settings", {
							Serialization::ValueSerializer<Vector2>::For<ImGuiStyle>(
								"FramePadding", "Padding within a framed rectangle (used by most widgets)",
								[](ImGuiStyle* style) -> Vector2 { return Vector2(style->FramePadding.x, style->FramePadding.y); },
								[](const Vector2& value, ImGuiStyle* style) { style->FramePadding = ImVec2(value.x, value.y); }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"FrameRounding", "Radius of frame corners rounding. Set to 0.0f to have rectangular frame (used by most widgets)",
								[](ImGuiStyle* style) -> float { return style->FrameRounding; },
								[](const float& value, ImGuiStyle* style) { style->FrameRounding = value; }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"FrameBorderSize", "Thickness of border around frames. Generally set to 0.0f or 1.0f. (Other values are not well tested and more CPU/GPU costly)",
								[](ImGuiStyle* style) -> float { return style->FrameBorderSize; },
								[](const float& value, ImGuiStyle* style) { style->FrameBorderSize = value; })
						}),
						Group::Create("Spacings", "Settings for some spacings", {
							Serialization::ValueSerializer<Vector2>::For<ImGuiStyle>(
								"ItemSpacing", "Horizontal and vertical spacing between widgets/lines",
								[](ImGuiStyle* style) -> Vector2 { return Vector2(style->ItemSpacing.x, style->ItemSpacing.y); },
								[](const Vector2& value, ImGuiStyle* style) { style->ItemSpacing = ImVec2(value.x, value.y); }),
							Serialization::ValueSerializer<Vector2>::For<ImGuiStyle>(
								"ItemInnerSpacing", "Horizontal and vertical spacing between within elements of a composed widget (e.g. a slider and its label)",
								[](ImGuiStyle* style) -> Vector2 { return Vector2(style->ItemInnerSpacing.x, style->ItemInnerSpacing.y); },
								[](const Vector2& value, ImGuiStyle* style) { style->ItemInnerSpacing = ImVec2(value.x, value.y); }),
							Serialization::ValueSerializer<Vector2>::For<ImGuiStyle>(
								"CellPadding", "Padding within a table cell",
								[](ImGuiStyle* style) -> Vector2 { return Vector2(style->CellPadding.x, style->CellPadding.y); },
								[](const Vector2& value, ImGuiStyle* style) { style->CellPadding = ImVec2(value.x, value.y); }),
							Serialization::ValueSerializer<Vector2>::For<ImGuiStyle>(
								"TouchExtraPadding", "Expand reactive bounding box for touch-based system where touch position is not accurate enough. Unfortunately we don't sort widgets so priority on overlap will always be given to the first widget. So don't grow this too much!",
								[](ImGuiStyle* style) -> Vector2 { return Vector2(style->TouchExtraPadding.x, style->TouchExtraPadding.y); },
								[](const Vector2& value, ImGuiStyle* style) { style->TouchExtraPadding = ImVec2(value.x, value.y); }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"IndentSpacing", "Horizontal indentation when e.g. entering a tree node. Generally == (FontSize + FramePadding.x*2)",
								[](ImGuiStyle* style) -> float { return style->IndentSpacing; },
								[](const float& value, ImGuiStyle* style) { style->IndentSpacing = value; }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"ColumnsMinSpacing", "Minimum horizontal spacing between two columns. Preferably > (FramePadding.x + 1)",
								[](ImGuiStyle* style) -> float { return style->ColumnsMinSpacing; },
								[](const float& value, ImGuiStyle* style) { style->ColumnsMinSpacing = value; })
						}),
						Group::Create("Grabby stuff", "Settings for scroll bar and sliders", {
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"ScrollbarSize", "Width of the vertical scrollbar, Height of the horizontal scrollbar",
								[](ImGuiStyle* style) -> float { return style->ScrollbarSize; },
								[](const float& value, ImGuiStyle* style) { style->ScrollbarSize = value; }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"ScrollbarRounding", "Radius of grab corners for scrollbar",
								[](ImGuiStyle* style) -> float { return style->ScrollbarRounding; },
								[](const float& value, ImGuiStyle* style) { style->ScrollbarRounding = value; }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"GrabMinSize", "Minimum width/height of a grab box for slider/scrollbar",
								[](ImGuiStyle* style) -> float { return style->GrabMinSize; },
								[](const float& value, ImGuiStyle* style) { style->GrabMinSize = value; }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"GrabRounding", "Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs",
								[](ImGuiStyle* style) -> float { return style->GrabRounding; },
								[](const float& value, ImGuiStyle* style) { style->GrabRounding = value; }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"LogSliderDeadzone", "The size in pixels of the dead-zone around zero on logarithmic sliders that cross zero",
								[](ImGuiStyle* style) -> float { return style->LogSliderDeadzone; },
								[](const float& value, ImGuiStyle* style) { style->LogSliderDeadzone = value; })
						}),
						Group::Create("Tab", "Tab settings", {
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"TabRounding", "Radius of upper corners of a tab. Set to 0.0f to have rectangular tabs",
								[](ImGuiStyle* style) -> float { return style->TabRounding; },
								[](const float& value, ImGuiStyle* style) { style->TabRounding = value; }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"TabBorderSize", "Thickness of border around tabs",
								[](ImGuiStyle* style) -> float { return style->TabBorderSize; },
								[](const float& value, ImGuiStyle* style) { style->TabBorderSize = value; }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"TabMinWidthForCloseButton", "Minimum width for close button to appears on an unselected tab when hovered. Set to 0.0f to always show when hovering, set to FLT_MAX to never show close button unless selected",
								[](ImGuiStyle* style) -> float { return style->TabMinWidthForCloseButton; },
								[](const float& value, ImGuiStyle* style) { style->TabMinWidthForCloseButton = value; })
						}),
						Group::Create("More alignment & padding stuff", "More alignment stuff", {
							Serialization::ValueSerializer<ImGuiDir>::For<ImGuiStyle>(
								"ColorButtonPosition", "Side of the color button in the ColorEdit4 widget (left/right). Defaults to ImGuiDir_Right",
								[](ImGuiStyle* style) -> ImGuiDir { return style->ColorButtonPosition; },
								[](const ImGuiDir& value, ImGuiStyle* style) { style->ColorButtonPosition = value; },
								{ Object::Instantiate<Serialization::EnumAttribute<ImGuiDir>>(std::vector<Serialization::EnumAttribute<ImGuiDir>::Choice>{
									Serialization::EnumAttribute<ImGuiDir>::Choice("ImGuiDir_Left", ImGuiDir_Left),
									Serialization::EnumAttribute<ImGuiDir>::Choice("ImGuiDir_Right", ImGuiDir_Right)
									}, false) }),
							Serialization::ValueSerializer<Vector2>::For<ImGuiStyle>(
								"ButtonTextAlign", "Alignment of button text when button is larger than text. Defaults to (0.5f, 0.5f) (centered)",
								[](ImGuiStyle* style) -> Vector2 { return Vector2(style->ButtonTextAlign.x, style->ButtonTextAlign.y); },
								[](const Vector2& value, ImGuiStyle* style) { style->ButtonTextAlign = ImVec2(value.x, value.y); }),
							Serialization::ValueSerializer<Vector2>::For<ImGuiStyle>(
								"SelectableTextAlign", "Alignment of selectable text. Defaults to (0.0f, 0.0f) (top-left aligned). It's generally important to keep this left-aligned if you want to lay multiple items on a same line",
								[](ImGuiStyle* style) -> Vector2 { return Vector2(style->SelectableTextAlign.x, style->SelectableTextAlign.y); },
								[](const Vector2& value, ImGuiStyle* style) { style->SelectableTextAlign = ImVec2(value.x, value.y); }),
							Serialization::ValueSerializer<Vector2>::For<ImGuiStyle>(
								"DisplayWindowPadding", "Window position are clamped to be visible within the display area or monitors by at least this amount. Only applies to regular windows",
								[](ImGuiStyle* style) -> Vector2 { return Vector2(style->DisplayWindowPadding.x, style->DisplayWindowPadding.y); },
								[](const Vector2& value, ImGuiStyle* style) { style->DisplayWindowPadding = ImVec2(value.x, value.y); }),
							Serialization::ValueSerializer<Vector2>::For<ImGuiStyle>(
								"DisplaySafeAreaPadding", "If you cannot see the edges of your screen (e.g. on a TV) increase the safe area padding. Apply to popups/tooltips as well regular windows. NB: Prefer configuring your TV sets correctly!",
								[](ImGuiStyle* style) -> Vector2 { return Vector2(style->DisplaySafeAreaPadding.x, style->DisplaySafeAreaPadding.y); },
								[](const Vector2& value, ImGuiStyle* style) { style->DisplaySafeAreaPadding = ImVec2(value.x, value.y); })
						}),
						Serialization::ValueSerializer<float>::For<ImGuiStyle>(
							"MouseCursorScale", "Scale software rendered mouse cursor (when io.MouseDrawCursor is enabled). We apply per-monitor DPI scaling over this scale. May be removed later",
							[](ImGuiStyle* style) -> float { return style->MouseCursorScale; },
							[](const float& value, ImGuiStyle* style) { style->MouseCursorScale = value; }),
						Group::Create("Quality", "Quality settings", {
							Serialization::ValueSerializer<bool>::For<ImGuiStyle>(
								"AntiAliasedLines", "Enable anti-aliased lines/borders. Disable if you are really tight on CPU/GPU. Latched at the beginning of the frame (copied to ImDrawList)",
								[](ImGuiStyle* style) -> bool { return style->AntiAliasedLines; },
								[](const bool& value, ImGuiStyle* style) { style->AntiAliasedLines = value; }),
							Serialization::ValueSerializer<bool>::For<ImGuiStyle>(
								"AntiAliasedLinesUseTex", "Enable anti-aliased lines/borders using textures where possible. Require backend to render with bilinear filtering. Latched at the beginning of the frame (copied to ImDrawList)",
								[](ImGuiStyle* style) -> bool { return style->AntiAliasedLinesUseTex; },
								[](const bool& value, ImGuiStyle* style) { style->AntiAliasedLinesUseTex = value; }),
							Serialization::ValueSerializer<bool>::For<ImGuiStyle>(
								"AntiAliasedFill", "Enable anti-aliased edges around filled shapes (rounded rectangles, circles, etc.). Disable if you are really tight on CPU/GPU. Latched at the beginning of the frame (copied to ImDrawList)",
								[](ImGuiStyle* style) -> bool { return style->AntiAliasedFill; },
								[](const bool& value, ImGuiStyle* style) { style->AntiAliasedFill = value; }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"CurveTessellationTol", "Tessellation tolerance when using PathBezierCurveTo() without a specific number of segments. Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality",
								[](ImGuiStyle* style) -> float { return style->CurveTessellationTol; },
								[](const float& value, ImGuiStyle* style) { style->CurveTessellationTol = value; }),
							Serialization::ValueSerializer<float>::For<ImGuiStyle>(
								"CircleTessellationMaxError", "Maximum error (in pixels) allowed when using AddCircle()/AddCircleFilled() or drawing rounded corner rectangles with no explicit segment count specified. Decrease for higher quality but more geometry",
								[](ImGuiStyle* style) -> float { return style->CircleTessellationMaxError; },
								[](const float& value, ImGuiStyle* style) { style->CircleTessellationMaxError = value; })
						}),
						Group::Create("Colors", "ImGui Colors", {
							CreateColorSerializer("ImGuiCol_Text", "", ImGuiCol_Text),
							CreateColorSerializer("ImGuiCol_TextDisabled", "", ImGuiCol_TextDisabled),
							CreateColorSerializer("ImGuiCol_WindowBg", "", ImGuiCol_WindowBg),
							CreateColorSerializer("ImGuiCol_ChildBg", "", ImGuiCol_ChildBg),
							CreateColorSerializer("ImGuiCol_PopupBg", "", ImGuiCol_PopupBg),
							CreateColorSerializer("ImGuiCol_Border", "", ImGuiCol_Border),
							CreateColorSerializer("ImGuiCol_BorderShadow", "", ImGuiCol_BorderShadow),
							Group::Create("Frame", "Frame settings", {
								CreateColorSerializer("ImGuiCol_FrameBg", "", ImGuiCol_FrameBg),
								CreateColorSerializer("ImGuiCol_FrameBgHovered", "", ImGuiCol_FrameBgHovered),
								CreateColorSerializer("ImGuiCol_FrameBgActive", "", ImGuiCol_FrameBgActive)
							}),
							Group::Create("Title", "Title settings", {
								CreateColorSerializer("ImGuiCol_TitleBg", "", ImGuiCol_TitleBg),
								CreateColorSerializer("ImGuiCol_TitleBgActive", "", ImGuiCol_TitleBgActive),
								CreateColorSerializer("ImGuiCol_TitleBgCollapsed", "", ImGuiCol_TitleBgCollapsed)
							}),
							CreateColorSerializer("ImGuiCol_MenuBarBg", "", ImGuiCol_MenuBarBg),
							Group::Create("Scrollbar", "Scrollbar settings", {
								CreateColorSerializer("ImGuiCol_ScrollbarBg", "", ImGuiCol_ScrollbarBg),
								CreateColorSerializer("ImGuiCol_ScrollbarGrab", "", ImGuiCol_ScrollbarGrab),
								CreateColorSerializer("ImGuiCol_ScrollbarGrabHovered", "", ImGuiCol_ScrollbarGrabHovered),
								CreateColorSerializer("ImGuiCol_ScrollbarGrabActive", "", ImGuiCol_ScrollbarGrabActive)
							}),
							CreateColorSerializer("ImGuiCol_CheckMark", "", ImGuiCol_CheckMark),
							CreateColorSerializer("ImGuiCol_SliderGrab", "", ImGuiCol_SliderGrab),
							CreateColorSerializer("ImGuiCol_SliderGrabActive", "", ImGuiCol_SliderGrabActive),
							Group::Create("Button", "Button settings", {
								CreateColorSerializer("ImGuiCol_Button", "", ImGuiCol_Button),
								CreateColorSerializer("ImGuiCol_ButtonHovered", "", ImGuiCol_ButtonHovered),
								CreateColorSerializer("ImGuiCol_ButtonActive", "", ImGuiCol_ButtonActive)
							}),
							Group::Create("Header", "Header settings", {
								CreateColorSerializer("ImGuiCol_Header", "", ImGuiCol_Header),
								CreateColorSerializer("ImGuiCol_HeaderHovered", "", ImGuiCol_HeaderHovered),
								CreateColorSerializer("ImGuiCol_HeaderActive", "", ImGuiCol_HeaderActive)
							}),
							Group::Create("Separator", "Separator settings", {
								CreateColorSerializer("ImGuiCol_Separator", "", ImGuiCol_Separator),
								CreateColorSerializer("ImGuiCol_SeparatorHovered", "", ImGuiCol_SeparatorHovered),
								CreateColorSerializer("ImGuiCol_SeparatorActive", "", ImGuiCol_SeparatorActive)
							}),
							Group::Create("ResizeGrip", "ResizeGrip settings", {
								CreateColorSerializer("ImGuiCol_ResizeGrip", "", ImGuiCol_ResizeGrip),
								CreateColorSerializer("ImGuiCol_ResizeGripHovered", "", ImGuiCol_ResizeGripHovered),
								CreateColorSerializer("ImGuiCol_ResizeGripActive", "", ImGuiCol_ResizeGripActive)
							}),
							Group::Create("Tab", "Tab settings", {
								CreateColorSerializer("ImGuiCol_Tab", "", ImGuiCol_Tab),
								CreateColorSerializer("ImGuiCol_TabHovered", "", ImGuiCol_TabHovered),
								CreateColorSerializer("ImGuiCol_TabActive", "", ImGuiCol_TabActive),
								CreateColorSerializer("ImGuiCol_TabUnfocused", "", ImGuiCol_TabUnfocused),
								CreateColorSerializer("ImGuiCol_TabUnfocusedActive", "", ImGuiCol_TabUnfocusedActive)
							}),
							CreateColorSerializer("ImGuiCol_DockingPreview", "", ImGuiCol_DockingPreview),
							CreateColorSerializer("ImGuiCol_DockingEmptyBg", "", ImGuiCol_DockingEmptyBg),
							Group::Create("Plot", "Plot settings", {
								CreateColorSerializer("ImGuiCol_PlotLines", "", ImGuiCol_PlotLines),
								CreateColorSerializer("ImGuiCol_PlotLinesHovered", "", ImGuiCol_PlotLinesHovered),
								CreateColorSerializer("ImGuiCol_PlotHistogram", "", ImGuiCol_PlotHistogram),
								CreateColorSerializer("ImGuiCol_PlotHistogramHovered", "", ImGuiCol_PlotHistogramHovered)
							}),
							Group::Create("Table", "Table settings", {
								CreateColorSerializer("ImGuiCol_TableHeaderBg", "", ImGuiCol_TableHeaderBg),
								CreateColorSerializer("ImGuiCol_TableBorderStrong", "", ImGuiCol_TableBorderStrong),
								CreateColorSerializer("ImGuiCol_TableBorderLight", "", ImGuiCol_TableBorderLight),
								CreateColorSerializer("ImGuiCol_TableRowBg", "", ImGuiCol_TableRowBg),
								CreateColorSerializer("ImGuiCol_TableRowBgAlt", "", ImGuiCol_TableRowBgAlt)
							}),
							CreateColorSerializer("ImGuiCol_TextSelectedBg", "", ImGuiCol_TextSelectedBg),
							CreateColorSerializer("ImGuiCol_DragDropTarget", "", ImGuiCol_DragDropTarget),
							CreateColorSerializer("ImGuiCol_NavHighlight", "", ImGuiCol_NavHighlight),
							CreateColorSerializer("ImGuiCol_NavWindowingHighlight", "", ImGuiCol_NavWindowingHighlight),
							CreateColorSerializer("ImGuiCol_NavWindowingDimBg", "", ImGuiCol_NavWindowingDimBg),
							CreateColorSerializer("ImGuiCol_ModalWindowDimBg", "", ImGuiCol_ModalWindowDimBg)
						})
					};
					for (size_t i = 0; i < colorSerializers.size(); i++)
						recordElement(colorSerializers[i]->Serialize(style));
				}
			};
#pragma warning (default: 26812)

			inline static void DrawSaveButton(EditorContext* context) {
				if (!ImGui::Button(ICON_FA_FLOPPY_O " Save")) return;
				const std::optional<OS::Path> dialogueResult = OS::SaveDialogue("Save ImGui style", "", { OS::FileDialogueFilter("Json", { "*.json" }) });
				if (!dialogueResult.has_value()) return;
				OS::Path path = dialogueResult.value().extension().native().length() > 0 ? dialogueResult.value() : OS::Path(((std::string)dialogueResult.value()) + ".json");
				std::ofstream stream(path);
				if (!stream.good()) {
					context->Log()->Error("Failed to open file '", path, "'!");
					return;
				}
				stream << ImGuiStyleUndoAction::CreateSnapshot().dump(1, '\t') << std::endl;
				stream.close();
				context->Log()->Info("ImGui style saved to '", path, "'");
			}

			inline static void DrawLoadButton(EditorContext* context) {
				if (!ImGui::Button(ICON_FA_FOLDER " Load")) return;
				std::vector<OS::Path> path = OS::OpenDialogue("Load ImGui style", "", { OS::FileDialogueFilter("Json", { "*.json" }) });
				if (path.empty()) return;
				const Reference<OS::MMappedFile> mapping = OS::MMappedFile::Create(path[0], context->Log());
				if (mapping == nullptr) {
					context->Log()->Error("Failed to open file '", path[0], "'!");
					return;
				}
				try {
					MemoryBlock block(*mapping);
					nlohmann::json snapshot = nlohmann::json::parse(std::string_view(reinterpret_cast<const char*>(block.Data()), block.Size()));
					ImGuiStyleUndoAction::LoadSnapshot(snapshot);
					eraseImGuiStyleUndoActionStack(context);
					context->Log()->Info("ImGui style loaded from '", path[0], "'");
				}
				catch (nlohmann::json::parse_error& err) {
					context->Log()->Error("EditorScene::Load - Could not parse file: \"", path[0], "\"! [Error: <", err.what(), ">]");
				}
			}
		}

		ImGuiStyleEditor::ImGuiStyleEditor(EditorContext* context) : EditorWindow(context, "UI Style Editor") {}

		const Serialization::ItemSerializer::Of<ImGuiStyle>* ImGuiStyleEditor::StyleSerializer() {
			static const ImGuiStyleSerializer instance;
			return &instance;
		}

		void ImGuiStyleEditor::DrawEditorWindow() {
			ImGuiStyle& style = ImGui::GetStyle();
			{
				DrawSaveButton(EditorWindowContext());
				ImGui::SameLine();
				DrawLoadButton(EditorWindowContext());
				ImGui::Separator();
			}
			static thread_local std::optional<nlohmann::json> initialSnapshot;
			nlohmann::json snapshot = ImGuiStyleUndoAction::CreateSnapshot();
			bool changeFinished = DrawSerializedObject(StyleSerializer()->Serialize(style), (size_t)this, EditorWindowContext()->Log(), [&](const Serialization::SerializedObject&) {
				EditorWindowContext()->Log()->Error("ImGuiStyleEditor::DrawEditorWindow - StyleSerializer does not have any object pointers!");
				return false;
				});
			if ((!initialSnapshot.has_value()) && (snapshot != ImGuiStyleUndoAction::CreateSnapshot()))
				initialSnapshot = std::move(snapshot);
			if (changeFinished && initialSnapshot.has_value()) {
				Reference<UndoStack::Action> undoAction = Object::Instantiate<ImGuiStyleUndoAction>(EditorWindowContext(), initialSnapshot.value());
				EditorWindowContext()->AddUndoAction(undoAction);
				initialSnapshot = std::optional<nlohmann::json>();
			}
		}

		namespace {
			static const EditorMainMenuCallback editorMenuCallback(
				"Window/Options/ImGui Style", "Customize ImGui style for the Editor", Callback<EditorContext*>([](EditorContext* context) {
					Object::Instantiate<ImGuiStyleEditor>(context);
					}));
			static EditorMainMenuAction::RegistryEntry action;
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::ImGuiStyleEditor>(const Callback<TypeId>& report) {
		report(TypeId::Of<Editor::EditorWindow>());
	}
	template<> void TypeIdDetails::OnRegisterType<Editor::ImGuiStyleEditor>() {
		Editor::action = &Editor::editorMenuCallback;
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::ImGuiStyleEditor>() {
		Editor::action = nullptr;
	}
}
