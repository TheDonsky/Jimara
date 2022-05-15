#include "TransformHandleSettings.h"
#include "../Gizmo.h"
#include "../../GUI/ImGuiRenderer.h"
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
					auto toggleWithButton = [&](TransformHandleSettings::HandleType type, auto... buttonText) {
						const bool wasEnabled = m_settings->HandleMode() == type;
						if (wasEnabled) ImGui::BeginDisabled();
						if (ImGui::Button(buttonText...)) m_settings->SetHandleMode(type);
						if (wasEnabled) ImGui::EndDisabled();
					};
					toggleWithButton(TransformHandleSettings::HandleType::MOVE, ICON_FA_ARROWS_ALT "###transform_handles_move_mode_on");
					ImGui::SameLine();
					toggleWithButton(TransformHandleSettings::HandleType::ROTATE, ICON_FA_SYNC "###transform_handles_rotation_mode_on");
					ImGui::SameLine();
					toggleWithButton(TransformHandleSettings::HandleType::SCALE, ICON_FA_EXPAND "###transform_handles_scale_mode_on");
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
