#include "HDRILightGizmo.h"
#include <Components/Lights/HDRILight.h>
#include <Environment/Rendering/ImageBasedLighting/HDRISkyboxRenderer.h>


namespace Jimara {
	namespace Editor {
		struct HDRILightGizmo::Helpers {
			static void OnGraphicsSynch(HDRILightGizmo* self) {
				if (self->m_renderer == nullptr)
					return;
				HDRILight* light = self->Target<HDRILight>();
				if (light == nullptr || (!light->ActiveInHeirarchy()))
					return Helpers::Clear(self);
				HDRISkyboxRenderer* renderer = dynamic_cast<HDRISkyboxRenderer*>(self->m_renderer.operator->());
				renderer->SetColorMultiplier(Vector4(light->Color() * light->Intensity(), 1.0f));
				renderer->SetEnvironmentMap(light->Texture());
			} 

			static void Clear(HDRILightGizmo* self) {
				if (self->m_renderer == nullptr)
					return;
				self->GizmoContext()->Viewport()->ViewportRenderStack()->RemoveRenderer(self->m_renderer);
				self->Context()->Graphics()->OnGraphicsSynch() -= Callback(Helpers::OnGraphicsSynch, self);
				self->m_renderer = nullptr;
			}
		};

		HDRILightGizmo::HDRILightGizmo(Scene::LogicContext* context)
			: Component(context, "HDRILightGizmo") {}

		HDRILightGizmo::~HDRILightGizmo() {
			Helpers::Clear(this);
		}

		void HDRILightGizmo::Update() {
			HDRILight* light = Target<HDRILight>();
			if (light == nullptr || (!light->ActiveInHeirarchy()))
				return Helpers::Clear(this);
			else if (m_renderer != nullptr)
				return;
			m_renderer = HDRISkyboxRenderer::Create(GizmoContext()->Viewport()->GizmoSceneViewport());
			if (m_renderer == nullptr)
				return Context()->Log()->Error(
					"HDRILightGizmo::Update - Could not create a renderer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			m_renderer->SetCategory(0u);
			m_renderer->SetPriority(~uint32_t(0u) - 1u);
			GizmoContext()->Viewport()->ViewportRenderStack()->AddRenderer(m_renderer);
			Context()->Graphics()->OnGraphicsSynch() += Callback(Helpers::OnGraphicsSynch, this);
		}

		namespace {
			static const constexpr Gizmo::ComponentConnection HDRILightGizmo_Connection =
				Gizmo::ComponentConnection::Make<HDRILightGizmo, HDRILight>(
					Gizmo::FilterFlag::CREATE_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED |
					Gizmo::FilterFlag::CREATE_CHILD_GIZMOS_IF_SELECTED |
					Gizmo::FilterFlag::CREATE_PARENT_GIZMOS_IF_SELECTED);
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::HDRILightGizmo>() {
		Editor::Gizmo::AddConnection(Editor::HDRILightGizmo_Connection);
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::HDRILightGizmo>() {
		Editor::Gizmo::RemoveConnection(Editor::HDRILightGizmo_Connection);
	}
}
