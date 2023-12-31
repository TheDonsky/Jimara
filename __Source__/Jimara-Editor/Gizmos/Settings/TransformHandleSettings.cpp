#include "TransformHandleSettings.h"
#include "../Gizmo.h"
#include "../../GUI/ImGuiRenderer.h"
#include "../../GUI/Utils/DrawTooltip.h"
#include "../../Environment/EditorStorage.h"
#include <Data/Serialization/Attributes/EnumAttribute.h>
#include <IconFontCppHeaders/IconsFontAwesome5.h>


namespace Jimara {
	namespace Editor {
		class TransformHandleSettings::Cache : public virtual ObjectCache<Reference<const Object>> {
		public:
			inline static Reference<TransformHandleSettings> GetFor(EditorContext* context) {
				static Cache cache;
				return cache.GetCachedOrCreate(context, [&]()->Reference<TransformHandleSettings> {
					Reference<TransformHandleSettings> instance = new TransformHandleSettings();
					instance->ReleaseRef();
					context->AddStorageObject(instance);
					return instance;
					});
			}
		};

		Reference<TransformHandleSettings> TransformHandleSettings::Of(EditorContext* context) {
			return TransformHandleSettings::Cache::GetFor(context);
		}


		namespace {
			class TransformHandleSettings_Drawer : public virtual Gizmo, public virtual GizmoGUI::Drawer {
			private:
				const Reference<TransformHandleSettings> m_settings;

			public:
				inline TransformHandleSettings_Drawer(Scene::LogicContext* context)
					: Component(context, "TransformHandleSettings_Drawer")
					, GizmoGUI::Drawer(TransformHandleSettings::GizmoGUIPriority())
					, m_settings(TransformHandleSettings::Of(GizmoScene::GetContext(context))) {}

			protected:
				inline virtual void OnDrawGizmoGUI()final override {
					const bool ctrlPressed =
						Context()->Input()->KeyPressed(OS::Input::KeyCode::LEFT_CONTROL) ||
						Context()->Input()->KeyPressed(OS::Input::KeyCode::RIGHT_CONTROL);
					auto toggleWithButton = [&](TransformHandleSettings::HandleType type, auto buttonText, auto tooltip, OS::Input::KeyCode hotKey) {
						const bool wasEnabled = m_settings->HandleMode() == type;
						if (wasEnabled) ImGui::BeginDisabled();
						if (ImGui::Button(buttonText) || (Context()->Input()->KeyDown(hotKey) && (!ctrlPressed)))
							m_settings->SetHandleMode(type);
						DrawTooltip(buttonText, tooltip);
						if (wasEnabled) ImGui::EndDisabled();
					};
					toggleWithButton(TransformHandleSettings::HandleType::MOVE, 
						ICON_FA_ARROWS_ALT "###transform_handles_move_mode_on", 
						"Move (G)", OS::Input::KeyCode::G);
					ImGui::SameLine();
					toggleWithButton(TransformHandleSettings::HandleType::ROTATE, 
						ICON_FA_SYNC "###transform_handles_rotation_mode_on",
						"Rotate (R)", OS::Input::KeyCode::R);
					ImGui::SameLine();
					toggleWithButton(TransformHandleSettings::HandleType::SCALE, 
						ICON_FA_EXPAND "###transform_handles_scale_mode_on",
						"Scale (S)", OS::Input::KeyCode::S);

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

			class TransformHandleSettings_Serializer : public virtual EditorStorageSerializer {
			public:
				inline TransformHandleSettings_Serializer() : ItemSerializer("TransformHandleSettings_Serializer", "Serializer of TransformHandleSettings") {}
				virtual TypeId StorageType()const override { return TypeId::Of<TransformHandleSettings>(); }
				virtual Reference<Object> CreateObject(EditorContext* context)const override { return TransformHandleSettings::Of(context); }
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Object* targetAddr)const override {
					TransformHandleSettings* target = dynamic_cast<TransformHandleSettings*>(targetAddr);
					if (target == nullptr) return;
					{
						static const Reference<const Serialization::ItemSerializer::Of<TransformHandleSettings>> serializer =
							Serialization::ValueSerializer<uint8_t>::For<TransformHandleSettings>(
								"Handle Mode", "Active handle type (MOVE/ROTATE/SCLE)",
								[](TransformHandleSettings* target) -> uint8_t { return static_cast<uint8_t>(target->HandleMode()); },
								[](const uint8_t& value, TransformHandleSettings* target) { target->SetHandleMode(static_cast<TransformHandleSettings::HandleType>(value)); },
								{ Object::Instantiate<Serialization::EnumAttribute<uint8_t>>(std::vector<Serialization::EnumAttribute<uint8_t>::Choice>({
									Serialization::EnumAttribute<uint8_t>::Choice("MOVE", static_cast<uint8_t>(TransformHandleSettings::HandleType::MOVE)),
									Serialization::EnumAttribute<uint8_t>::Choice("ROTATE", static_cast<uint8_t>(TransformHandleSettings::HandleType::ROTATE)),
									Serialization::EnumAttribute<uint8_t>::Choice("SCALE", static_cast<uint8_t>(TransformHandleSettings::HandleType::SCALE))
									}), false) });
						recordElement(serializer->Serialize(target));
					}
					{
						static const Reference<const Serialization::ItemSerializer::Of<TransformHandleSettings>> serializer =
							Serialization::ValueSerializer<uint8_t>::For<TransformHandleSettings>(
								"Handle Orientation", "Tells, whether to place the handles in world or local space (LOCAL/WORLD)",
								[](TransformHandleSettings* target) -> uint8_t { return static_cast<uint8_t>(target->HandleOrientation()); },
								[](const uint8_t& value, TransformHandleSettings* target) { target->SetHandleOrientation(static_cast<TransformHandleSettings::AxisSpace>(value)); },
								{ Object::Instantiate<Serialization::EnumAttribute<uint8_t>>(std::vector<Serialization::EnumAttribute<uint8_t>::Choice>({
									Serialization::EnumAttribute<uint8_t>::Choice("LOCAL", static_cast<uint8_t>(TransformHandleSettings::AxisSpace::LOCAL)),
									Serialization::EnumAttribute<uint8_t>::Choice("WORLD", static_cast<uint8_t>(TransformHandleSettings::AxisSpace::WORLD))
									}), false) });
						recordElement(serializer->Serialize(target));
					}
					{
						static const Reference<const Serialization::ItemSerializer::Of<TransformHandleSettings>> serializer =
							Serialization::ValueSerializer<uint8_t>::For<TransformHandleSettings>(
								"Pivot Position", "Tells, what to use as the pivot point during scale/rotation (AVERAGE/INDIVIDUAL)",
								[](TransformHandleSettings* target) -> uint8_t { return static_cast<uint8_t>(target->PivotPosition()); },
								[](const uint8_t& value, TransformHandleSettings* target) { target->SetPivotPosition(static_cast<TransformHandleSettings::PivotMode>(value)); },
								{ Object::Instantiate<Serialization::EnumAttribute<uint8_t>>(std::vector<Serialization::EnumAttribute<uint8_t>::Choice>({
									Serialization::EnumAttribute<uint8_t>::Choice("AVERAGE", static_cast<uint8_t>(TransformHandleSettings::PivotMode::AVERAGE)),
									Serialization::EnumAttribute<uint8_t>::Choice("INDIVIDUAL", static_cast<uint8_t>(TransformHandleSettings::PivotMode::INDIVIDUAL))
									}), false) });
						recordElement(serializer->Serialize(target));
					}
				}
			};
		}
	}


	// Registration callbacks
	template<> void TypeIdDetails::OnRegisterType<Editor::TransformHandleSettings>() {
		Editor::Gizmo::AddConnection(Editor::TransformHandleSettings_Drawer_CONNECTION);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::TransformHandleSettings>(){
		Editor::Gizmo::RemoveConnection(Editor::TransformHandleSettings_Drawer_CONNECTION);
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::TransformHandleSettings>(const Callback<const Object*>& report) {
		static const Editor::TransformHandleSettings_Serializer serializer;
		report(&serializer);
	}
}
