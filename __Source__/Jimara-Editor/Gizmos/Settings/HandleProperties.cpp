#include "HandleProperties.h"
#include "TransformHandleSettings.h"
#include "../Gizmo.h"
#include "../../GUI/ImGuiRenderer.h"
#include "../../Environment/EditorStorage.h"


namespace Jimara {
	namespace Editor {
		class HandleProperties::Cache : public virtual ObjectCache<Reference<const Object>>{
		public:
			inline static Reference<HandleProperties> GetFor(EditorContext* context) {
				static Cache cache;
				return cache.GetCachedOrCreate(context, [&]()->Reference<HandleProperties> {
					Reference<HandleProperties> instance = new HandleProperties();
					instance->ReleaseRef();
					context->AddStorageObject(instance);
					return instance;
					});
			}
		};

		Reference<HandleProperties> HandleProperties::Of(EditorContext* context) {
			return Cache::GetFor(context);
		}

		float HandleProperties::HandleSizeFor(const GizmoViewport* viewport, const Vector3& position) {
			const Transform* viewportTransform = viewport->ViewportTransform();
			const Size2 resolution = viewport->Resolution();
			if (viewport->ProjectionMode() == Camera::ProjectionMode::PERSPECTIVE) {
				const float fieldOfView = viewport->FieldOfView();
				const float heightPerDistance = std::tan(Math::Radians(fieldOfView * 0.5f)) * 2.0f;
				if (resolution.y < 1) return 0.0f;
				const float sizePerDistance = m_handleSize.load() * heightPerDistance / static_cast<float>(resolution.y);
				const Vector3 delta = position - viewportTransform->WorldPosition();
				const float distance = Math::Dot(viewportTransform->Forward(), delta);
				return sizePerDistance * distance;
			}
			else {
				if (resolution.y < 1) return 0.0f;
				return m_handleSize.load() * viewport->OrthographicSize() / static_cast<float>(resolution.y);
			}
		}

		float HandleProperties::GizmoGUIPriority() { return TransformHandleSettings::GizmoGUIPriority() * (1.0f - std::numeric_limits<float>::epsilon()); }

		namespace {
			class HandleProperties_Drawer : public virtual Gizmo, public virtual GizmoGUI::Drawer {
			private:
				const Reference<HandleProperties> m_settings;

			public:
				inline HandleProperties_Drawer(Scene::LogicContext* context)
					: Component(context, "HandleProperties_Drawer")
					, GizmoGUI::Drawer(HandleProperties::GizmoGUIPriority())
					, m_settings(HandleProperties::Of(GizmoScene::GetContext(context)->EditorApplicationContext())) {}

			protected:
				inline virtual void OnDrawGizmoGUI()final override {
					float size = m_settings->HandleSize();
					ImGui::SameLine();
					ImGui::Text("|");
					ImGui::SameLine();
					float posX = ImGui::GetCursorPos().x;
					float width = ImGui::GetWindowWidth();
					float remainding = Math::Max(width - posX, 0.0f);
					ImGui::PushItemWidth(Math::Min(remainding, 200.0f));
					if (ImGui::SliderFloat("Handle Size###HandleProperties_Drawer_handle_size", &size, 56.0f, 256.0f, "%.0f"))
						if (size != m_settings->HandleSize())
							m_settings->SetHandleSize(size);
					ImGui::PopItemWidth();
				}
			};

			class HandleProperties_Serializer : public virtual EditorStorageSerializer {
			public:
				inline HandleProperties_Serializer() : ItemSerializer("HandleProperties_Serializer", "Serializer of HandleProperties") {}
				virtual TypeId StorageType()const override { return TypeId::Of<HandleProperties>(); }
				virtual Reference<Object> CreateObject(EditorContext* context)const override { return HandleProperties::Of(context); }
				inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Object* targetAddr)const override {
					HandleProperties* target = dynamic_cast<HandleProperties*>(targetAddr);
					if (target == nullptr) return;
					{
						static const Reference<const Serialization::ItemSerializer::Of<HandleProperties>> serializer =
							Serialization::ValueSerializer<float>::For<HandleProperties>(
								"Handle Size", " Preffered base handle size in pixels",
								[](HandleProperties* target) -> float { return target->HandleSize(); },
								[](const float& value, HandleProperties* target) { target->SetHandleSize(value); });
						recordElement(serializer->Serialize(target));
					}
				}
			};
		}
	}


	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::HandleProperties>(const Callback<const Object*>& report) {
		static const Editor::HandleProperties_Serializer serializer;
		report(&serializer);
		static const Reference<const Editor::Gizmo::ComponentConnection> connection = 
			Editor::Gizmo::ComponentConnection::Targetless<Editor::HandleProperties_Drawer>();
		report(connection);
	}
}
