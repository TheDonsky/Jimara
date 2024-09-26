#include "GizmoViewport.h"
#include "Settings/HandleProperties.h"
#include <Jimara/Environment/Rendering/LightingModels/ForwardRendering/ForwardPlusLightingModel.h>
#include <Jimara/Components/Lights/DirectionalLight.h>


namespace Jimara {
	namespace Editor {
		namespace {
			class GizmoSceneViewport_ClearPass : public virtual RenderStack::Renderer {
			private:
				const Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>> m_clearColor;

			public:
				inline GizmoSceneViewport_ClearPass(Graphics::GraphicsDevice* device)
					: m_clearColor(Graphics::SharedTextureSamplerBinding(Vector4(0.0f), device)) {
					SetCategory(0u);
					SetPriority(~uint32_t(0u));
				}

				inline virtual ~GizmoSceneViewport_ClearPass() {}

				virtual void Render(Graphics::InFlightBufferInfo commandBufferInfo, RenderImages* images) final override {
					images->GetImage(RenderImages::MainColor())->Resolve()->TargetTexture()->Blit(
						commandBufferInfo, m_clearColor->BoundObject()->TargetView()->TargetTexture());
				}
			};

			class GizmoSceneViewportT : public virtual ViewportDescriptor {
			public:
				Matrix4 viewMatrix = Math::Identity();
				Camera::ProjectionMode projectionMode = Camera::ProjectionMode::PERSPECTIVE;
				float fieldOfView = 60.0f;
				float orthographicSize = 8.0f;
				Vector4 clearColor = Vector4(0.0f);
				float aspectRatio = 1.0f;

				inline GizmoSceneViewportT(Scene::LogicContext* context) : ViewportDescriptor(context) {};

				inline virtual ~GizmoSceneViewportT() {}

				inline virtual Matrix4 ViewMatrix()const override { return viewMatrix; }

				inline virtual Matrix4 ProjectionMatrix()const override {
					const constexpr float closePlane = 0.01f;
					const constexpr float farPlane = 10000.0f;
					return (projectionMode == Camera::ProjectionMode::PERSPECTIVE)
						? Math::Perspective(fieldOfView, aspectRatio, closePlane, farPlane)
						: Math::Orthographic(orthographicSize, aspectRatio, closePlane, farPlane);
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
			, m_renderStack(Object::Instantiate<RenderStack>(targetContext, Size2(0u), Graphics::Texture::Multisampling::SAMPLE_COUNT_1))
			, m_targetViewport(Object::Instantiate<GizmoSceneViewportT>(targetContext))
			, m_gizmoViewport(Object::Instantiate<GizmoSceneViewportT>(gizmoContext)) {
			assert(targetContext != nullptr);
			assert(gizmoContext != nullptr);
			{
				const Reference<RenderStack::Renderer> clearPass = Object::Instantiate<GizmoSceneViewport_ClearPass>(targetContext->Graphics()->Device());
				m_renderStack->AddRenderer(clearPass);
			}
			{
				const Reference<RenderStack::Renderer> targetRenderer(ForwardPlusLightingModel::Instance()->CreateRenderer(
					m_targetViewport, LayerMask::All(), Graphics::RenderPass::Flags::CLEAR_DEPTH));
				if (targetRenderer == nullptr)
					m_targetViewport->Context()->Log()->Error("GizmoViewport::GizmoViewport - Failed to create renderer for target viewport!");
				else {
					targetRenderer->SetCategory(0);
					m_renderStack->AddRenderer(targetRenderer);
				}
			}
			{
				const Reference<RenderStack::Renderer> gizmoRenderer(ForwardPlusLightingModel::Instance()->CreateRenderer(
					m_gizmoViewport, LayerMask(
						static_cast<Layer>(GizmoLayers::WORLD_SPACE),
						static_cast<Layer>(GizmoLayers::SELECTION_WORLD_SPACE)),
					Graphics::RenderPass::Flags::NONE));
				if (gizmoRenderer == nullptr)
					m_gizmoViewport->Context()->Log()->Error(
						"GizmoViewport::GizmoViewportRenderer - Failed to create WORLD_SPACE/SELECTION_WORLD_SPACE renderer for gizmo viewport!");
				else {
					gizmoRenderer->SetCategory(1);
					m_renderStack->AddRenderer(gizmoRenderer);
				}
			}
			{
				const Reference<RenderStack::Renderer> gizmoRenderer(ForwardPlusLightingModel::Instance()->CreateRenderer(
					m_gizmoViewport, LayerMask(
						static_cast<Layer>(GizmoLayers::OVERLAY),
						static_cast<Layer>(GizmoLayers::SELECTION_OVERLAY)),
					Graphics::RenderPass::Flags::CLEAR_DEPTH));
				if (gizmoRenderer == nullptr)
					m_gizmoViewport->Context()->Log()->Error(
						"GizmoViewport::GizmoViewportRenderer - Failed to create OVERLAY/SELECTION_OVERLAY renderer for gizmo viewport!");
				else {
					gizmoRenderer->SetCategory(2);
					m_renderStack->AddRenderer(gizmoRenderer);
				}
			}
			{
				const Reference<RenderStack::Renderer> gizmoRenderer(ForwardPlusLightingModel::Instance()->CreateRenderer(
					m_gizmoViewport, LayerMask(static_cast<Layer>(GizmoLayers::HANDLE)),
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
			std::unique_lock<std::recursive_mutex> lock(m_targetContext->UpdateLock());
			m_gizmoContext->Graphics()->OnGraphicsSynch() -= Callback(&GizmoViewport::Update, this);
			m_renderStack->SetResolution(Size2(0));
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

		float GizmoViewport::GizmoSizeAt(const Vector3& location)const {
			if (m_handleProperties == nullptr) 
				m_handleProperties = HandleProperties::Of(GizmoScene::GetContext(m_gizmoContext)->EditorApplicationContext());
			return dynamic_cast<HandleProperties*>(m_handleProperties.operator->())->HandleSizeFor(this, location);
		}

		void GizmoViewport::Update() {
			GizmoSceneViewportT* targetViewport = dynamic_cast<GizmoSceneViewportT*>(m_targetViewport.operator->());
			GizmoSceneViewportT* gizmoViewport = dynamic_cast<GizmoSceneViewportT*>(m_gizmoViewport.operator->());
			targetViewport->clearColor = m_clearColor;
			targetViewport->projectionMode = gizmoViewport->projectionMode = m_projectionMode;
			targetViewport->fieldOfView = gizmoViewport->fieldOfView = FieldOfView();
			targetViewport->orthographicSize = gizmoViewport->orthographicSize = m_orthographicSize;
			targetViewport->viewMatrix = gizmoViewport->viewMatrix = Math::Inverse(ViewportTransform()->WorldMatrix());
			const Vector2 size = m_renderStack->Resolution();
			targetViewport->aspectRatio = gizmoViewport->aspectRatio = (size.x / Math::Max(size.y, 1.0f));
		}
	}
}
