#include "HDRILightGizmo.h"
#include <Jimara/Components/Lights/HDRILight.h>
#include <Jimara/Environment/Rendering/ImageBasedLighting/HDRISkyboxRenderer.h>
#include <Jimara/Environment/Rendering/ImageBasedLighting/HDRIEnvironment.h>


namespace Jimara {
	namespace Editor {
		struct HDRILightGizmo::Helpers {
			struct Renderer : public virtual RenderStack::Renderer {
				const Reference<HDRISkyboxRenderer> skyboxRenderer;

				inline Renderer(HDRISkyboxRenderer* renderer) : skyboxRenderer(renderer) {}
				inline ~Renderer() {}

				inline virtual void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) final override {
					skyboxRenderer->Render(commandBufferInfo, images);
				}
			};

			static void OnGraphicsSynch(HDRILightGizmo* self) {
				if (self->m_renderer == nullptr)
					return;
				HDRILight* light = self->Target<HDRILight>();
				if (light == nullptr || (!light->ActiveInHeirarchy()))
					return Helpers::Clear(self);
				Renderer* renderer = dynamic_cast<Renderer*>(self->m_renderer.operator->());
				renderer->skyboxRenderer->SetColorMultiplier(Vector4(light->Color() * light->Intensity(), 1.0f));
				const HDRIEnvironment* environment = light->Texture();
				renderer->skyboxRenderer->SetEnvironmentMap(
					environment == nullptr ? nullptr : environment->HDRI());
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
			const Reference<HDRISkyboxRenderer> skyboxRenderer = HDRISkyboxRenderer::Create(GizmoContext()->Viewport()->GizmoSceneViewport());
			if (skyboxRenderer == nullptr)
				return Context()->Log()->Error(
					"HDRILightGizmo::Update - Could not create a renderer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			m_renderer = Object::Instantiate<Helpers::Renderer>(skyboxRenderer);
			m_renderer->SetCategory(0u);
			m_renderer->SetPriority(~uint32_t(0u) - 1u);
			GizmoContext()->Viewport()->ViewportRenderStack()->AddRenderer(m_renderer);
			Context()->Graphics()->OnGraphicsSynch() += Callback(Helpers::OnGraphicsSynch, this);
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::HDRILightGizmo>(const Callback<const Object*>& report) {
		static const Reference<const Editor::Gizmo::ComponentConnection> connection = 
			Editor::Gizmo::ComponentConnection::Make<Editor::HDRILightGizmo, HDRILight>(
				Editor::Gizmo::FilterFlag::CREATE_IF_SELECTED |
				Editor::Gizmo::FilterFlag::CREATE_IF_NOT_SELECTED |
				Editor::Gizmo::FilterFlag::CREATE_CHILD_GIZMOS_IF_SELECTED |
				Editor::Gizmo::FilterFlag::CREATE_PARENT_GIZMOS_IF_SELECTED |
				Editor::Gizmo::FilterFlag::CREATE_ONE_FOR_ALL_TARGETS);
		report(connection);
	}
}
