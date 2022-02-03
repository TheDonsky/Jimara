#include "Camera.h"
#include "../Environment/GraphicsContext/LightingModels/ForwardRendering/ForwardLightingModel.h"
#include "../Data/Serialization/Attributes/SliderAttribute.h"
#include "../Data/Serialization/Attributes/ColorAttribute.h"


namespace Jimara {
	namespace {
		class Viewport : public virtual LightingModel::ViewportDescriptor {
		private:
			Matrix4 m_viewMatrix = Math::MatrixFromEulerAngles(Vector3(0.0f));
			float m_fieldOfView = 64.0f;
			float m_closePlane = 0.0001f;
			float m_farPlane = 100000.0f;
			Vector4 m_clearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);

		public:
			mutable SpinLock cameraLock;
			std::atomic<Camera*> cameraPtr;
			
			Reference<Camera> GetCamera()const {
				std::unique_lock<SpinLock> lock(cameraLock);
				Reference<Camera> ptr = cameraPtr.load();
				return ptr;
			}

			class UpdateJob : public JobSystem::Job {
			private:
				Viewport* const m_viewport;

			public:
				inline UpdateJob(Viewport* viewport) : m_viewport(viewport) {}

			protected:
				inline virtual void Execute() override {
					Reference<Camera> camera = m_viewport->GetCamera();
					if (camera == nullptr) return;

					// Update view matrix:
					{
						Reference<Transform> transform = camera->GetTransfrom();
						if (transform == nullptr) m_viewport->m_viewMatrix = Math::MatrixFromEulerAngles(Vector3(0.0f));
						else  m_viewport->m_viewMatrix = Math::Inverse(transform->WorldMatrix());
					}

					// Update projection matrix arguments:
					{
						m_viewport->m_fieldOfView = camera->FieldOfView();
						m_viewport->m_closePlane = camera->ClosePlane();
						m_viewport->m_farPlane = camera->FarPlane();
					}

					// Update clear color:
					{
						m_viewport->m_clearColor = camera->ClearColor();
					}
				}

				inline virtual void CollectDependencies(Callback<Job*>) override {}
			} job;

			inline Viewport(Camera* camera) : LightingModel::ViewportDescriptor(camera->Context()), cameraPtr(camera), job(this) {
				Context()->Graphics()->SynchPointJobs().Add(&job);
			}

			inline ~Viewport() {
				Context()->Graphics()->SynchPointJobs().Remove(&job);
			}

			inline virtual Matrix4 ViewMatrix()const override { return m_viewMatrix; }

			inline virtual Matrix4 ProjectionMatrix(float aspect)const override {
				return Math::Perspective(Math::Radians(m_fieldOfView), aspect, m_closePlane, m_farPlane);
			}

			inline virtual Vector4 ClearColor()const override { return m_clearColor; }
		};
	}

	Camera::Camera(Component* parent, const std::string_view& name, float fieldOfView, float closePlane, float farPlane, const Vector4& clearColor)
		: Component(parent, name)
		, m_viewport(Object::Instantiate<Viewport>(this)) {
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
			m_renderer = m_lightingModel->CreateRenderer(m_viewport);
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

	const LightingModel::ViewportDescriptor* Camera::ViewportDescriptor()const { return m_viewport; }

	void Camera::OnComponentEnabled() {
		SetSceneLightingModel(SceneLightingModel());
	}

	void Camera::OnComponentDisabled() {
		SetSceneLightingModel(SceneLightingModel());
	}

	void Camera::OnComponentDestroyed() {
		SetSceneLightingModel(SceneLightingModel());
	}

	void Camera::OnOutOfScope()const {
		Viewport* viewport = dynamic_cast<Viewport*>(m_viewport.operator->());
		{
			std::unique_lock<SpinLock> lock(viewport->cameraLock);
			if (RefCount() > 0) return;
			viewport->cameraPtr = nullptr;
		}
		viewport->Context()->Graphics()->SynchPointJobs().Remove(&viewport->job);
		Component::OnOutOfScope();
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
