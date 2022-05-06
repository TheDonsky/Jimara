#include "GizmoViewport.h"
#include <Environment/Rendering/LightingModels/ForwardRendering/ForwardLightingModel.h>
#include <Components/Lights/DirectionalLight.h>


namespace Jimara {
	namespace Editor {
		namespace {
			class GizmoSceneViewportT : public virtual ViewportDescriptor {
			public:
				Matrix4 viewMatrix = Math::Identity();
				float fieldOfView = 60.0f;
				Vector4 clearColor = Vector4(0.0f);

				inline GizmoSceneViewportT(Scene::LogicContext* context) : ViewportDescriptor(context) {};

				inline virtual Matrix4 ViewMatrix()const override { return viewMatrix; }

				inline virtual Matrix4 ProjectionMatrix(float aspect)const override {
					return Math::Perspective(Math::Radians(fieldOfView), aspect, 0.001f, 10000.0f);
				}

				inline virtual Vector4 ClearColor()const override { return clearColor; }
			};

			class GizmoSceneViewportRootTransform : public virtual Component {
			public:
				inline GizmoSceneViewportRootTransform(Scene::LogicContext* context) : Component(context, "GizmoSceneViewportRootTransform") {}
			};
		}

		GizmoViewport::GizmoViewport(Scene::LogicContext* targetContext, Scene::LogicContext* gizmoContext) 
			: m_targetContext(targetContext), m_gizmoContext(gizmoContext)
			, m_renderStack(Object::Instantiate<RenderStack>(targetContext))
			, m_targetViewport(Object::Instantiate<GizmoSceneViewportT>(targetContext))
			, m_gizmoViewport(Object::Instantiate<GizmoSceneViewportT>(gizmoContext)) {
			assert(targetContext != nullptr);
			assert(gizmoContext != nullptr);
			{
				const Reference<RenderStack::Renderer> targetRenderer(ForwardLightingModel::Instance()->CreateRenderer(
					m_targetViewport, GraphicsLayerMask::All(), Graphics::RenderPass::Flags::CLEAR_COLOR | Graphics::RenderPass::Flags::CLEAR_DEPTH));
				if (targetRenderer == nullptr)
					m_targetViewport->Context()->Log()->Error("GizmoViewport::GizmoViewport - Failed to create renderer for target viewport!");
				else {
					targetRenderer->SetCategory(0);
					m_renderStack->AddRenderer(targetRenderer);
				}
			}
			{
				const Reference<RenderStack::Renderer> gizmoRenderer(ForwardLightingModel::Instance()->CreateRenderer(
					m_gizmoViewport, GraphicsLayerMask(static_cast<GraphicsLayer>(GizmoLayers::SELECTION_WORLD_SPACE)),
					Graphics::RenderPass::Flags::NONE));
				if (gizmoRenderer == nullptr)
					m_gizmoViewport->Context()->Log()->Error(
						"GizmoViewport::GizmoViewportRenderer - Failed to create SELECTION_WORLD_SPACE renderer for gizmo viewport!");
				else {
					gizmoRenderer->SetCategory(1);
					m_renderStack->AddRenderer(gizmoRenderer);
				}
			}
			{
				const Reference<RenderStack::Renderer> gizmoRenderer(ForwardLightingModel::Instance()->CreateRenderer(
					m_gizmoViewport, GraphicsLayerMask(static_cast<GraphicsLayer>(GizmoLayers::SELECTION_OVERLAY)),
					Graphics::RenderPass::Flags::CLEAR_DEPTH));
				if (gizmoRenderer == nullptr)
					m_gizmoViewport->Context()->Log()->Error(
						"GizmoViewport::GizmoViewportRenderer - Failed to create SELECTION_OVERLAY renderer for gizmo viewport!");
				else {
					gizmoRenderer->SetCategory(2);
					m_renderStack->AddRenderer(gizmoRenderer);
				}
			}
			{
				const Reference<RenderStack::Renderer> gizmoRenderer(ForwardLightingModel::Instance()->CreateRenderer(
					m_gizmoViewport, GraphicsLayerMask(static_cast<GraphicsLayer>(GizmoLayers::HANDLE)),
					Graphics::RenderPass::Flags::CLEAR_DEPTH | Graphics::RenderPass::Flags::RESOLVE_COLOR));
				if (gizmoRenderer == nullptr)
					m_gizmoViewport->Context()->Log()->Error(
						"GizmoViewport::GizmoViewportRenderer - Failed to create HANDLE renderer for gizmo viewport!");
				else {
					gizmoRenderer->SetCategory(3);
					m_renderStack->AddRenderer(gizmoRenderer);
				}
			}
			m_gizmoContext->Graphics()->OnGraphicsSynch() += Callback(&GizmoViewport::Update, this);
		}

		GizmoViewport::~GizmoViewport() {
			m_gizmoContext->Graphics()->OnGraphicsSynch() -= Callback(&GizmoViewport::Update, this);
		}

		Transform* GizmoViewport::ViewportTransform()const {
			if (m_transform == nullptr || m_transform->Destroyed()) {
				if (m_rootComponent == nullptr || m_rootComponent->Destroyed())
					m_rootComponent = Object::Instantiate<GizmoSceneViewportRootTransform>(m_gizmoContext);
				m_transform = Object::Instantiate<Transform>(m_rootComponent, "GizmoViewport Transform");
				m_transform->SetLocalPosition(Vector3(2.0f));
				m_transform->LookAt(Vector3(0.0f));
				Object::Instantiate<DirectionalLight>(m_transform, "GizmoViewport light");
			}
			return m_transform;
		}

		Size2 GizmoViewport::Resolution()const {
			return m_renderStack->Resolution();
		}

		void GizmoViewport::SetResolution(const Size2& resolution) {
			m_renderStack->SetResolution(resolution);
		}

		void GizmoViewport::Update() {
			GizmoSceneViewportT* targetViewport = dynamic_cast<GizmoSceneViewportT*>(m_targetViewport.operator->());
			GizmoSceneViewportT* gizmoViewport = dynamic_cast<GizmoSceneViewportT*>(m_gizmoViewport.operator->());
			targetViewport->clearColor = m_clearColor;
			targetViewport->fieldOfView = gizmoViewport->fieldOfView = FieldOfView();
			targetViewport->viewMatrix = gizmoViewport->viewMatrix = Math::Inverse(ViewportTransform()->WorldMatrix());
		}
	}
}
