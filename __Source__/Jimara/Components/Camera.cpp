#include "Camera.h"
#include "../Environment/GraphicsContext/LightingModels/ForwardRendering/ForwardLightingModel.h"


namespace Jimara {
	namespace {
		class Viewport : public virtual LightingModel::ViewportDescriptor {
		private:
			const Reference<Camera> m_camera;

		public:
			inline Viewport(Camera* camera) : LightingModel::ViewportDescriptor(camera->Context()), m_camera(camera) {}

			inline virtual Matrix4 ViewMatrix()const override {
				Reference<Transform> transform = m_camera->GetTransfrom();
				if (transform == nullptr) return Math::MatrixFromEulerAngles(Vector3(0.0f));
				else return Math::Inverse(transform->WorldMatrix());
			}

			inline virtual Matrix4 ProjectionMatrix(float aspect)const override {
				return m_camera->ProjectionMatrix(aspect);
			}

			inline virtual Vector4 ClearColor()const override {
				return m_camera->ClearColor();
			}
		};

		class CameraRenderer : public virtual Graphics::ImageRenderer {
		private:
			Viewport m_viewport;
			Reference<Graphics::ImageRenderer> m_renderer;

		public:
			inline CameraRenderer(Camera* camera) : m_viewport(camera) {
				m_renderer = camera->SceneLightingModel()->CreateRenderer(&m_viewport);
				if (m_renderer == nullptr) camera->Context()->Log()->Fatal("Camera failed to create a renderer!");
			}

			inline virtual ~CameraRenderer() { m_renderer = nullptr; }

			inline virtual Reference<Object> CreateEngineData(Graphics::RenderEngineInfo* engineInfo) override { 
				return (m_renderer == nullptr ? nullptr : m_renderer->CreateEngineData(engineInfo)); 
			}

			inline virtual void Render(Object* engineData, Graphics::Pipeline::CommandBufferInfo bufferInfo) {
				if (m_renderer != nullptr) m_renderer->Render(engineData, bufferInfo);
			}
		};
	}

	Camera::Camera(Component* parent, const std::string_view& name, float fieldOfView, float closePlane, float farPlane, const Vector4& clearColor)
		: Component(parent, name) {
		SetFieldOfView(fieldOfView);
		SetClosePlane(closePlane);
		SetFarPlane(farPlane);
		SetClearColor(clearColor);
		SetSceneLightingModel(ForwardLightingModel::Instance()); // __TODO__: Take this from the scene defaults...
	}

	Camera::~Camera() {
		SetSceneLightingModel(nullptr);
	}

	float Camera::FieldOfView()const { return m_fieldOfView; }

	void Camera::SetFieldOfView(float value) {
		m_fieldOfView = max(std::numeric_limits<float>::epsilon(), min(value, 180.0f - std::numeric_limits<float>::epsilon()));
	}

	float Camera::ClosePlane()const { return m_closePlane; }

	void Camera::SetClosePlane(float value) {
		m_closePlane = max(std::numeric_limits<float>::epsilon(), value);
		SetFarPlane(m_farPlane);
	}
	
	float Camera::FarPlane()const { return m_farPlane; }

	void Camera::SetFarPlane(float value) { m_farPlane = max((float)m_closePlane, value); }

	Vector4 Camera::ClearColor()const { return m_clearColor; }

	void Camera::SetClearColor(const Vector4& color) { m_clearColor = color; }

	Matrix4 Camera::ProjectionMatrix(float aspect)const { 
		return Math::Perspective(Math::Radians(m_fieldOfView), aspect, m_closePlane, m_farPlane);
	}

	LightingModel* Camera::SceneLightingModel()const { return m_lightingModel; }

	namespace {
		inline static void DestroyRenderer(Camera* camera, Reference<Scene::GraphicsContext::Renderer>& renderer) {
			if (renderer == nullptr) return;
			camera->Context()->Graphics()->Renderers().RemoveRenderer(renderer);
			renderer = nullptr;
		}
	}

	void Camera::SetSceneLightingModel(LightingModel* model) {
		// Change model if need be...
		{
			const bool sameModel = (m_lightingModel == model);
			if ((!sameModel) || Destroyed()) {
				DestroyRenderer(this, m_renderer);
				m_lightingModel = model;
				if (Destroyed() || sameModel) return;
			}
		}

		// Create renderer if possible...
		if (m_lightingModel != nullptr) {
			Reference<Viewport> viewport = Object::Instantiate<Viewport>(this);
			m_renderer = m_lightingModel->CreateRenderer(viewport);
			if (m_renderer == nullptr)
				Context()->Log()->Error("Camera::SetSceneLightingModel - Failed to create a renderer!");
		}

		// Add/remove renderer to render stack 
		if (m_renderer != nullptr) {
			if (ActiveInHeirarchy())
				Context()->Graphics()->Renderers().AddRenderer(m_renderer);
			else Context()->Graphics()->Renderers().RemoveRenderer(m_renderer);
		}
	}

	void Camera::OnComponentEnabled() {
		SetSceneLightingModel(SceneLightingModel());
	}

	void Camera::OnComponentDisabled() {
		SetSceneLightingModel(SceneLightingModel());
	}

	void Camera::OnComponentDestroyed() {
		SetSceneLightingModel(SceneLightingModel());
	}
}
