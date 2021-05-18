#include "Camera.h"


namespace Jimara {
	namespace {
		class CameraRenderer : public virtual Graphics::ImageRenderer {
		private:
			class Viewport : public virtual LightingModel::ViewportDescriptor {
			private:
				const Reference<Camera> m_camera;

			public:
				inline Viewport(Camera* camera) : LightingModel::ViewportDescriptor(camera->Context()->Graphics()), m_camera(camera) {}

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
			} m_viewport;
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

	Camera::Camera(Component* parent, const std::string& name, float fieldOfView, float closePlane, float farPlane, const Vector4& clearColor)
		: Component(parent, name), m_isAlive(true) {
		SetFieldOfView(fieldOfView);
		SetClosePlane(closePlane);
		SetFarPlane(farPlane);
		SetClearColor(clearColor);
		SetSceneLightingModel(Context()->Graphics()->DefaultLightingModel());
		OnDestroyed() += Callback(&Camera::DestroyCamera, this);
	}

	Camera::~Camera() {
		OnDestroyed() -= Callback(&Camera::DestroyCamera, this);
		DestroyCamera(this);
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
		Matrix4 projection = glm::perspective(glm::radians(m_fieldOfView), aspect, m_closePlane, m_farPlane);
		projection[2] *= -1.0f;
		return projection;
	}

	LightingModel* Camera::SceneLightingModel()const { return m_lightingModel; }

	void Camera::SetSceneLightingModel(LightingModel* model) {
		if (m_lightingModel == model) return;
		DisposeRenderer();
		m_lightingModel = model;
		if ((!m_isAlive) || (m_lightingModel == nullptr)) return;
		m_renderer = Object::Instantiate<CameraRenderer>(this);
		// __TODO__: Add the renderer to global registries and what not
	}

	Graphics::ImageRenderer* Camera::Renderer()const { return m_renderer; }

	void Camera::DisposeRenderer() {
		// __TODO__: Remove the renderer from global registries and what not
		m_renderer = nullptr;
	}

	void Camera::DestroyCamera(Component*) {
		m_isAlive = false;
		DisposeRenderer();
	}
}
