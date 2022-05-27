#include "TransformHandleSettings.h"
#include "../Gizmo.h"
#include "../../GUI/ImGuiRenderer.h"
#include "../../GUI/Utils/DrawTooltip.h"
#include <IconFontCppHeaders/IconsFontAwesome5.h>


namespace Jimara {
	namespace Editor {
		class TransformHandleSettings::Cache : public virtual ObjectCache<Reference<const Object>> {
		public:
			inline static Reference<TransformHandleSettings> GetFor(const Object* key) {
				static Cache cache;
				return cache.GetCachedOrCreate(key, false, []()->Reference<TransformHandleSettings> {
					Reference<TransformHandleSettings> instance = new TransformHandleSettings();
					instance->ReleaseRef();
					return instance;
					});
			}
		};

		Reference<TransformHandleSettings> TransformHandleSettings::Of(GizmoScene::Context* context) {
			return TransformHandleSettings::Cache::GetFor(context);
		}


		namespace {
			class TransformHandleSettings_Drawer : public virtual Gizmo, public virtual GizmoGUI::Drawer {
			private:
				const Reference<TransformHandleSettings> m_settings;

			public:
				inline TransformHandleSettings_Drawer(Scene::LogicContext* context)
					: Component(context, "TransformHandleSettings_Drawer")
					, m_settings(TransformHandleSettings::Of(GizmoScene::GetContext(context))) {}

			protected:
				inline virtual void OnDrawGizmoGUI()final override {
					auto toggleWithButton = [&](TransformHandleSettings::HandleType type, auto buttonText, auto tooltip) {
						const bool wasEnabled = m_settings->HandleMode() == type;
						if (wasEnabled) ImGui::BeginDisabled();
						if (ImGui::Button(buttonText)) m_settings->SetHandleMode(type);
						DrawTooltip(buttonText, tooltip);
						if (wasEnabled) ImGui::EndDisabled();
					};
					toggleWithButton(TransformHandleSettings::HandleType::MOVE, 
						ICON_FA_ARROWS_ALT "###transform_handles_move_mode_on", 
						"Move");
					ImGui::SameLine();
					toggleWithButton(TransformHandleSettings::HandleType::ROTATE, 
						ICON_FA_SYNC "###transform_handles_rotation_mode_on",
						"Rotate");
					ImGui::SameLine();
					toggleWithButton(TransformHandleSettings::HandleType::SCALE, 
						ICON_FA_EXPAND "###transform_handles_scale_mode_on",
						"Scale");

					auto toggleValues = [&](auto valueA, auto valueB, auto getValue, auto setValue, auto textA, auto textB, auto tooltipA, auto tooltipB) {
						if (getValue() == valueA) {
							if (ImGui::Button(textA))
								setValue(valueB);
							DrawTooltip(textA, tooltipA);
						}
						else {
							if (ImGui::Button(textB))
								setValue(valueA);
							DrawTooltip(textB, tooltipB);
						}
						return true;
					};

					ImGui::SameLine();
					ImGui::Text("|");
					ImGui::SameLine();
					toggleValues(
						TransformHandleSettings::AxisSpace::LOCAL, TransformHandleSettings::AxisSpace::WORLD,
						[&]() { return m_settings->HandleOrientation(); }, [&](auto value) { m_settings->SetHandleOrientation(value); },
						ICON_FA_BULLSEYE " LOCAL", ICON_FA_GLOBE " WORLD",
						"Handle orientation ([Local] -> World space)", "Handle orientation ([World] -> Local space)");

					ImGui::SameLine();
					toggleValues(
						TransformHandleSettings::PivotMode::AVERAGE, TransformHandleSettings::PivotMode::INDIVIDUAL,
						[&]() { return m_settings->PivotPosition(); }, [&](auto value) { m_settings->SetPivotPosition(value); },
						ICON_FA_COMPRESS " CENTER", ICON_FA_DOT_CIRCLE " PIVOT",
						"Scale/Rotate around ([selection center] -> individual origins)", "Scale/Rotate around ([individual origins] -> selection center)");
				}
			};

			static const constexpr Gizmo::ComponentConnection TransformHandleSettings_Drawer_CONNECTION = Gizmo::ComponentConnection::Targetless<TransformHandleSettings_Drawer>();
		}
	}


	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::TransformHandleSettings>() {
		Editor::Gizmo::AddConnection(Editor::TransformHandleSettings_Drawer_CONNECTION);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::TransformHandleSettings>(){
		Editor::Gizmo::RemoveConnection(Editor::TransformHandleSettings_Drawer_CONNECTION);
	}
}
