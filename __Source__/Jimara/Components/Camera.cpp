#include "Camera.h"
#include "../Environment/Rendering/LightingModels/ForwardRendering/ForwardPlusLightingModel.h"
#include "../Data/Serialization/Attributes/SliderAttribute.h"
#include "../Data/Serialization/Attributes/ColorAttribute.h"
#include "../Data/Serialization/Attributes/EnumAttribute.h"
#include "../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	struct Camera::Helpers {
		class Viewport : public virtual ViewportDescriptor {
		private:
			Matrix4 m_viewMatrix = Math::MatrixFromEulerAngles(Vector3(0.0f));
			Camera::ProjectionMode m_projectionMode = Camera::ProjectionMode::PERSPECTIVE;
			float m_fieldOfView = 64.0f;
			float m_orthographicSize = 8.0f;
			float m_closePlane = 0.0001f;
			float m_farPlane = 100000.0f;
			float m_aspectRatio = 1.0f;
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
						m_viewport->m_viewMatrix = camera->ViewMatrix();
					}

					// Update projection matrix arguments:
					{
						m_viewport->m_projectionMode = camera->Mode();
						m_viewport->m_fieldOfView = camera->FieldOfView();
						m_viewport->m_orthographicSize = camera->OrthographicSize();
						m_viewport->m_closePlane = camera->ClosePlane();
						const Vector2 resolution = (camera->m_renderStack != nullptr) ? camera->m_renderStack->Resolution() : Size2(1u);
						m_viewport->m_aspectRatio = (resolution.x / Math::Max(resolution.y, 1.0f));
						const float farPlane = camera->FarPlane();
						m_viewport->m_farPlane = ((resolution.x * resolution.y) > std::numeric_limits<float>::epsilon())
							? farPlane : Math::Min(farPlane, m_viewport->m_closePlane * 2.0f);
					}

					// Update clear color:
					{
						m_viewport->m_clearColor = camera->ClearColor();
					}
				}

				inline virtual void CollectDependencies(Callback<Job*>) override {}
			} job;

			inline Viewport(Camera* camera) 
				: RendererFrustrumDescriptor(RendererFrustrumFlags::PRIMARY)
				, ViewportDescriptor(camera->Context()), cameraPtr(camera), job(this) {
				Context()->Graphics()->SynchPointJobs().Add(&job);
			}

			inline ~Viewport() {
				Context()->Graphics()->SynchPointJobs().Remove(&job);
			}

			inline virtual Matrix4 ViewMatrix()const override { return m_viewMatrix; }

			inline virtual Matrix4 ProjectionMatrix()const override {
				return (m_projectionMode == Camera::ProjectionMode::PERSPECTIVE)
					? Math::Perspective(m_fieldOfView, m_aspectRatio, m_closePlane, m_farPlane)
					: Math::Orthographic(m_orthographicSize, m_aspectRatio, m_closePlane, m_farPlane);
			}

			inline virtual Vector4 ClearColor()const override { return m_clearColor; }
		};
	};

	Camera::Camera(Component* parent, const std::string_view& name)
		: Component(parent, name)
		, m_viewport(Object::Instantiate<Helpers::Viewport>(this))
		, m_renderStack(RenderStack::Main(parent->Context())) {
		SetSceneLightingModel(nullptr); // __TODO__: Take this from the scene defaults...
	}

	Camera::~Camera() {
		SetSceneLightingModel(nullptr);
	}

	Camera::ProjectionMode Camera::Mode()const { return m_projectionMode; }

	void Camera::SetMode(ProjectionMode mode) { 
		m_projectionMode = (mode == ProjectionMode::ORTHOGRAPHIC) ? ProjectionMode::ORTHOGRAPHIC : ProjectionMode::PERSPECTIVE;
	}

	float Camera::FieldOfView()const { return m_fieldOfView; }

	void Camera::SetFieldOfView(float value) {
		m_fieldOfView = max(std::numeric_limits<float>::epsilon(), min(value, 180.0f - std::numeric_limits<float>::epsilon()));
	}

	float Camera::OrthographicSize()const { return m_orthographicSize; }

	void Camera::SetOrthographicSize(float value) { m_orthographicSize = value; }

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
			m_renderer->SetPriority(m_priority);
	}

	Matrix4 Camera::ProjectionMatrix(float aspect)const { 
		return (m_projectionMode == ProjectionMode::PERSPECTIVE)
			? Math::Perspective(m_fieldOfView, aspect, m_closePlane, m_farPlane)
			: Math::Orthographic(m_orthographicSize, aspect, m_closePlane, m_farPlane);
	}

	Matrix4 Camera::ProjectionMatrix()const {
		const Vector2 resolution = (m_renderStack == nullptr) ? Size2(1u) : m_renderStack->Resolution();
		return ProjectionMatrix(resolution.x / Math::Max(resolution.y, 1.0f));
	}

	Matrix4 Camera::ViewMatrix()const {
		const Transform* transform = GetTransform();
		if (transform == nullptr) return Math::Identity();
		else return Math::Inverse(transform->WorldMatrix());
	}

	namespace {
		inline static void DestroyRenderer(RenderStack* renderStack, Reference<RenderStack::Renderer>& renderer) {
			if (renderer == nullptr) return;
			renderStack->RemoveRenderer(renderer);
			renderer = nullptr;
		}
	}

	LayerMask Camera::Layers()const { return m_layers; }

	void Camera::RenderLayers(const LayerMask& layers) {
		if (layers == m_layers) return;
		DestroyRenderer(m_renderStack, m_renderer);
		m_layers = layers;
		SetSceneLightingModel(SceneLightingModel());
	}

	Graphics::RenderPass::Flags Camera::RendererFlags()const { return m_rendererFlags; }

	void Camera::SetRendererFlags(Graphics::RenderPass::Flags flags) {
		if (m_rendererFlags == flags) return;
		DestroyRenderer(m_renderStack, m_renderer);
		m_rendererFlags = flags;
		SetSceneLightingModel(SceneLightingModel());
	}

	LightingModel* Camera::SceneLightingModel()const { return m_lightingModel; }

	void Camera::SetSceneLightingModel(LightingModel* model) {
		// If we have a null lighting model, we'll set it to default:
		if (model == nullptr)
			model = ForwardPlusLightingModel::Instance();

		// Change model if need be...
		{
			const bool sameModel = (m_lightingModel == model);
			if ((!sameModel) || Destroyed()) {
				DestroyRenderer(m_renderStack, m_renderer);
				m_lightingModel = model;
				if (Destroyed() || sameModel) return;
			}
		}

		// Create renderer if possible...
		if (m_lightingModel != nullptr && m_renderer == nullptr) {
			m_renderer = m_lightingModel->CreateRenderer(m_viewport, m_layers, m_rendererFlags);
			if (m_renderer == nullptr)
				Context()->Log()->Error("Camera::SetSceneLightingModel - Failed to create a renderer!");
		}

		// Add/remove renderer to render stack 
		if (m_renderer != nullptr) {
			SetRendererCategory(RendererCategory());
			SetRendererPriority(RendererPriority());
			if (ActiveInHierarchy())
				m_renderStack->AddRenderer(m_renderer);
			else m_renderStack->RemoveRenderer(m_renderer);
		}
	}

	const ViewportDescriptor* Camera::ViewportDescriptor()const { return m_viewport; }

	void Camera::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Mode, SetMode, "Projection Mode", "Camera's projection mode",
				Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<ProjectionMode>>>(false,
					"PERSPECTIVE", ProjectionMode::PERSPECTIVE,
					"ORTHOGRAPHIC", ProjectionMode::ORTHOGRAPHIC));
			if (m_projectionMode == ProjectionMode::PERSPECTIVE)
				JIMARA_SERIALIZE_FIELD_GET_SET(FieldOfView, SetFieldOfView,
					"Field of view", "Field of vew (in degrees) for the perspective projection",
					Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 180.0f));
			else JIMARA_SERIALIZE_FIELD_GET_SET(OrthographicSize, SetOrthographicSize,
				"Orthographic Size", "Vertical size of the region, visible in orthographic mode");
			JIMARA_SERIALIZE_FIELD_GET_SET(ClosePlane, SetClosePlane, "Close Plane", "'Close' clipping plane (range: (epsilon) to (positive infinity))");
			JIMARA_SERIALIZE_FIELD_GET_SET(FarPlane, SetFarPlane, "Far Plane", "'Far' clipping plane (range: (ClosePlane) to (positive infinity))");
			JIMARA_SERIALIZE_FIELD_GET_SET(RendererFlags, SetRendererFlags, "Renderer Flags", "Flags for the underlying renderer",
				Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<Graphics::RenderPass::Flags>>>(true,
					"CLEAR_COLOR", Graphics::RenderPass::Flags::CLEAR_COLOR,
					"CLEAR_DEPTH", Graphics::RenderPass::Flags::CLEAR_DEPTH,
					"RESOLVE_COLOR", Graphics::RenderPass::Flags::RESOLVE_COLOR,
					"RESOLVE_DEPTH", Graphics::RenderPass::Flags::RESOLVE_DEPTH
					));
			JIMARA_SERIALIZE_FIELD_GET_SET(ClearColor, SetClearColor, "Clear color", "Clear color for rendering", Object::Instantiate<Serialization::ColorAttribute>());
			JIMARA_SERIALIZE_FIELD_GET_SET(RendererCategory, SetRendererCategory, "Render Category", "Higher category will render later; refer to Scene::GraphicsContext::Renderer for further details.");
			JIMARA_SERIALIZE_FIELD_GET_SET(RendererPriority, SetRendererPriority, "Render Priority", "Higher priority will render earlier within the same category; refer to Scene::GraphicsContext::Renderer for further details.");
			JIMARA_SERIALIZE_FIELD_GET_SET(SceneLightingModel, SetSceneLightingModel, "Lighting model", "Lighting model used for rendering");
			{
				LayerMask layers = Layers();
				JIMARA_SERIALIZE_FIELD(layers, "Layers", "Only the layers in this list will be rendered", Layers::LayerMaskAttribute::Instance());
				RenderLayers(layers);
			}
		};
	}

	void Camera::OnComponentInitialized() {
		SetSceneLightingModel(SceneLightingModel());
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

	void Camera::OnOutOfScope()const {
		Helpers::Viewport* viewport = dynamic_cast<Helpers::Viewport*>(m_viewport.operator->());
		{
			std::unique_lock<SpinLock> lock(viewport->cameraLock);
			if (RefCount() > 0) return;
			viewport->cameraPtr = nullptr;
		}
		viewport->Context()->Graphics()->SynchPointJobs().Remove(&viewport->job);
		Component::OnOutOfScope();
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Camera>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Camera>(
			"Camera", "Jimara/Camera", "Camera for in-game scene rendering");
		report(factory);
	}
}
