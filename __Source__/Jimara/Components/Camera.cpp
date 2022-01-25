#include "Camera.h"
#include "../Environment/GraphicsContext/LightingModels/ForwardRendering/ForwardLightingModel.h"
#include "../Data/Serialization/Attributes/SliderAttribute.h"
#include "../Data/Serialization/Attributes/ColorAttribute.h"


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
		SetSceneLightingModel(nullptr); // __TODO__: Take this from the scene defaults...
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

	uint32_t Camera::RendererCategory()const { return m_category; }

	void Camera::SetRendererCategory(uint32_t category) {
		m_category = category;
		if (m_renderer != nullptr)
			m_renderer->SetCategory(m_category);
	}

	uint32_t Camera::RendererPriority()const { return m_priority; }

	void Camera::SetRendererPriority(uint32_t priority) {
		m_priority = priority;
		if (m_renderer != nullptr)
			m_renderer->SetCategory(m_priority);
	}

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
		// If we have a null lighting model, we'll set it to default:
		if (model == nullptr)
			model = ForwardLightingModel::Instance();

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
		if (m_lightingModel != nullptr && m_renderer == nullptr) {
			Reference<Viewport> viewport = Object::Instantiate<Viewport>(this);
			m_renderer = m_lightingModel->CreateRenderer(viewport);
			if (m_renderer == nullptr)
				Context()->Log()->Error("Camera::SetSceneLightingModel - Failed to create a renderer!");
		}

		// Add/remove renderer to render stack 
		if (m_renderer != nullptr) {
			SetRendererCategory(RendererCategory());
			SetRendererPriority(RendererPriority());
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


	namespace {
		class CameraSerializer : public virtual ComponentSerializer::Of<Camera> {
		public:
			inline CameraSerializer() : ItemSerializer("Jimara/Camera", "Camera") {}

			inline static const CameraSerializer* Instance() {
				static const CameraSerializer instance;
				return &instance;
			}

			virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, Camera* target)const final override {
				TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>()->GetFields(recordElement, target);
				{
					static const Reference<const FieldSerializer> serializer = Serialization::ValueSerializer<float>::For<Camera>(
						"Field of view", "Field of vew (in degrees) for the perspective projection",
						[](Camera* camera) -> float { return camera->FieldOfView(); },
						[](const float& value, Camera* camera) { camera->SetFieldOfView(value); },
						{ Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 180.0f) });
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const FieldSerializer> serializer = Serialization::ValueSerializer<float>::For<Camera>(
						"Close Plane", "'Close' clipping plane (range: (epsilon) to (positive infinity))",
						[](Camera* camera) -> float { return camera->ClosePlane(); },
						[](const float& value, Camera* camera) { camera->SetClosePlane(value); });
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const FieldSerializer> serializer = Serialization::ValueSerializer<float>::For<Camera>(
						"Far Plane", "'Far' clipping plane (range: (ClosePlane) to (positive infinity))",
						[](Camera* camera) -> float { return camera->FarPlane(); },
						[](const float& value, Camera* camera) { camera->SetFarPlane(value); });
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const FieldSerializer> serializer = Serialization::ValueSerializer<Vector4>::For<Camera>(
						"Clear color", "Clear color for rendering",
						[](Camera* camera) -> Vector4 { return camera->ClearColor(); },
						[](const Vector4& value, Camera* camera) { camera->SetClearColor(value); },
						{ Object::Instantiate<Serialization::ColorAttribute>() });
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const FieldSerializer> serializer = Serialization::ValueSerializer<uint32_t>::For<Camera>(
						"Render Category", "Higher category will render later; refer to Scene::GraphicsContext::Renderer for further details.",
						[](Camera* camera) -> uint32_t { return camera->RendererCategory(); },
						[](const uint32_t& value, Camera* camera) { camera->SetRendererCategory(value); });
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const FieldSerializer> serializer = Serialization::ValueSerializer<uint32_t>::For<Camera>(
						"Render Priority", "Higher priority will render earlier within the same category; refer to Scene::GraphicsContext::Renderer for further details.",
						[](Camera* camera) -> uint32_t { return camera->RendererPriority(); },
						[](const uint32_t& value, Camera* camera) { camera->SetRendererPriority(value); });
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const FieldSerializer> serializer = Serialization::ValueSerializer<LightingModel*>::Create<Camera>(
						"Lighting model", "Lighting model used for rendering",
						Function<LightingModel*, Camera*>([](Camera* camera) -> LightingModel* { return camera->SceneLightingModel(); }),
						Callback<LightingModel*const&, Camera*>([](LightingModel*const& value, Camera* camera) { camera->SetSceneLightingModel(value); }));
					recordElement(serializer->Serialize(target));
				}
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Camera>(const Callback<const Object*>& report) {
		report(CameraSerializer::Instance());
	}
}
