#include "GizmoViewport.h"
#include <Environment/GraphicsContext/LightingModels/ForwardRendering/ForwardLightingModel.h>


namespace Jimara {
	namespace Editor {
		namespace {
			class GizmoSceneViewportT : public virtual LightingModel::ViewportDescriptor {
			public:
				Matrix4 viewMatrix = Math::Identity();
				float fieldOfView = 60.0f;
				std::optional<Vector4> clearColor;

				inline GizmoSceneViewportT(Scene::LogicContext* context) : LightingModel::ViewportDescriptor(context) {};

				inline virtual Matrix4 ViewMatrix()const override { return viewMatrix; }

				inline virtual Matrix4 ProjectionMatrix(float aspect)const override {
					return Math::Perspective(Math::Radians(fieldOfView), aspect, 0.001f, 10000.0f);
				}

				inline virtual std::optional<Vector4> ClearColor()const override { return clearColor; }
			};

			class GizmoSceneViewportRootTransform : public virtual Component {
			public:
				inline GizmoSceneViewportRootTransform(Scene::LogicContext* context) : Component(context, "GizmoSceneViewportRootTransform") {}
			};

			inline static Reference<Graphics::TextureView> CreateTargetTexture(Graphics::GraphicsDevice* device, Size2 size) {
				const Reference<Graphics::Texture> texture = device->CreateMultisampledTexture(
					Graphics::Texture::TextureType::TEXTURE_2D,
					Graphics::Texture::PixelFormat::B8G8R8A8_SRGB,
					Size3(size, 1), 1, Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
				if (texture == nullptr) {
					device->Log()->Error("GizmoViewport::CreateTargetTexture - Failed to create target texture! [File:", __FILE__, "'; Line: ", __LINE__, "]");
					return nullptr;
				}
				Reference<Graphics::TextureView> targetTexture = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
				if (targetTexture == nullptr)
					device->Log()->Error("GizmoViewport::CreateTargetTexture - Failed to create target texture view! [File:", __FILE__, "'; Line: ", __LINE__, "]");
				return targetTexture;
			}

			inline static bool UpdateTargetTexture(Graphics::GraphicsDevice* device, Size2 size, Reference<Graphics::TextureView>& targetTexture) {
				if (targetTexture == nullptr || targetTexture->TargetTexture()->Size() != Size3(size, 1)) {
					targetTexture = CreateTargetTexture(device, size);
					return true;
				}
				else return false;
			}

			class GizmoViewportRenderer : public virtual Scene::GraphicsContext::Renderer {
			private:
				const Reference<Graphics::GraphicsDevice> m_device;
				const Reference<Scene::GraphicsContext::Renderer> m_targetRenderer;
				const Reference<Scene::GraphicsContext::Renderer> m_gizmoRenderer;
				Reference<Graphics::TextureView> m_intermediateTexture;

			public:
				inline GizmoViewportRenderer(LightingModel::ViewportDescriptor* targetViewport, LightingModel::ViewportDescriptor* gizmoViewport) 
					: m_device(targetViewport->Context()->Graphics()->Device())
					, m_targetRenderer(ForwardLightingModel::Instance()->CreateRenderer(targetViewport))
					, m_gizmoRenderer(ForwardLightingModel::Instance()->CreateRenderer(gizmoViewport)) {
					if (m_targetRenderer == nullptr) 
						targetViewport->Context()->Log()->Error("GizmoViewport::GizmoViewportRenderer - Failed to create renderer for target viewport!");
					if (m_gizmoRenderer == nullptr) 
						targetViewport->Context()->Log()->Error("GizmoViewport::GizmoViewportRenderer - Failed to create renderer for gizmo viewport!");
				}

				inline virtual void Render(Graphics::Pipeline::CommandBufferInfo commandBufferInfo, Graphics::TextureView* targetView) override {
					if (targetView == nullptr) return;
					m_targetRenderer->Render(commandBufferInfo, targetView);

					Graphics::Texture* const targetTexture = targetView->TargetTexture();
					UpdateTargetTexture(m_device, targetTexture->Size(), m_intermediateTexture);
					if (m_intermediateTexture == nullptr) {
						m_device->Log()->Error("GizmoViewport::Render - Failed to create/update overlay texture! [File:", __FILE__, "'; Line: ", __LINE__, "]");
						return;
					}

					m_gizmoRenderer->Render(commandBufferInfo, targetView);
				}

				inline virtual void GetDependencies(Callback<JobSystem::Job*> report) override {
					if (m_targetRenderer != nullptr) m_targetRenderer->GetDependencies(report);
					if (m_gizmoRenderer != nullptr) m_gizmoRenderer->GetDependencies(report);
				}
			};
		}

		GizmoViewport::GizmoViewport(Scene::LogicContext* targetContext, Scene::LogicContext* gizmoContext) 
			: m_targetContext(targetContext), m_gizmoContext(gizmoContext)
			, m_targetViewport(Object::Instantiate<GizmoSceneViewportT>(targetContext))
			, m_gizmoViewport(Object::Instantiate<GizmoSceneViewportT>(gizmoContext)) {
			assert(targetContext != nullptr);
			assert(gizmoContext != nullptr);
			m_renderer = Object::Instantiate<GizmoViewportRenderer>(m_targetViewport, m_gizmoViewport);
			m_gizmoContext->Graphics()->Renderers().AddRenderer(m_renderer);
			m_gizmoContext->Graphics()->OnGraphicsSynch() += Callback(&GizmoViewport::Update, this);
		}

		GizmoViewport::~GizmoViewport() {
			m_gizmoContext->Graphics()->Renderers().RemoveRenderer(m_renderer);
			m_gizmoContext->Graphics()->OnGraphicsSynch() -= Callback(&GizmoViewport::Update, this);
		}

		Transform* GizmoViewport::ViewportTransform() {
			if (m_transform == nullptr || m_transform->Destroyed()) {
				if (m_rootComponent == nullptr || m_rootComponent->Destroyed())
					m_rootComponent = Object::Instantiate<GizmoSceneViewportRootTransform>(m_gizmoContext);
				m_transform = Object::Instantiate<Transform>(m_rootComponent, "GizmoViewport Transform");
				m_transform->SetLocalPosition(Vector3(2.0f));
				m_transform->LookAt(Vector3(0.0f));
			}
			return m_transform;
		}

		Size2 GizmoViewport::Resolution()const {
			Reference<Graphics::TextureView> targetView = m_gizmoContext->Graphics()->Renderers().TargetTexture();
			return (targetView == nullptr) ? Size2(0) : Size2(targetView->TargetTexture()->Size());
		}

		void GizmoViewport::SetResolution(const Size2& resolution) {
			Reference<Graphics::TextureView> targetView = m_gizmoContext->Graphics()->Renderers().TargetTexture();
			if (UpdateTargetTexture(m_gizmoContext->Graphics()->Device(), resolution, targetView))
				m_gizmoContext->Graphics()->Renderers().SetTargetTexture(targetView);
		}

		void GizmoViewport::Update() {
			GizmoSceneViewportT* targetViewport = dynamic_cast<GizmoSceneViewportT*>(m_targetViewport.operator->());
			GizmoSceneViewportT* gizmoViewport = dynamic_cast<GizmoSceneViewportT*>(m_gizmoContext.operator->());
			targetViewport->clearColor = m_clearColor;
			targetViewport->fieldOfView = targetViewport->fieldOfView = FieldOfView();
			targetViewport->viewMatrix = targetViewport->viewMatrix = Math::Inverse(ViewportTransform()->WorldMatrix());
		}
	}
}
