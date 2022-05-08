#pragma once
namespace Jimara { class Camera; }
#include "Component.h"
#include "Transform.h"
#include "../Environment/Rendering/LightingModels/LightingModel.h"


namespace Jimara {
	/// <summary> This will make sure, Camera is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::Camera);

	/// <summary>
	/// Camera, for defining the viewport
	/// </summary>
	class Camera : public virtual Component {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Name of the camera component </param>
		/// <param name="fieldOfView"> Field of view (range: (epsilon) to (180 - epsilon) degrees) </param>
		/// <param name="closePlane"> 'Close' clipping plane (range: (epsilon) to (positive infinity)) </param>
		/// <param name="farPlane"> 'Far' clipping plane (range: (closePlane) to (positive infinity)) </param>
		/// <param name="clearColor"> Clear color </param>
		Camera(Component* parent, const std::string_view& name = "Camera"
			, float fieldOfView = 64.0f, float closePlane = 0.001f, float farPlane = 10000.0f
			, const Vector4& clearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f));

		/// <summary> Virtual destructor </summary>
		virtual ~Camera();

		/// <summary> Field of view </summary>
		float FieldOfView()const;

		/// <summary>
		/// Updates field of view
		/// </summary>
		/// <param name="value"> New field of view (range: (epsilon) to (180 - epsilon) degrees) </param>
		void SetFieldOfView(float value);

		/// <summary> 'Close' clipping plane (range: (epsilon) to (positive infinity)) </summary>
		float ClosePlane()const;

		/// <summary>
		/// Updates close plane (will change far plane when it becomes incompatible with current configuration)
		/// </summary>
		/// <param name="value"> New distance </param>
		void SetClosePlane(float value);

		/// <summary> 'Far' clipping plane (range: (ClosePlane) to (positive infinity)) </summary>
		float FarPlane()const;

		/// <summary>
		/// Updates far plane
		/// </summary>
		/// <param name="value"> New distance </param>
		void SetFarPlane(float value);

		/// <summary> Clear color </summary>
		Vector4 ClearColor()const;

		/// <summary>
		/// Updates clear color
		/// </summary>
		/// <param name="color"> new value </param>
		void SetClearColor(const Vector4& color);

		/// <summary> 
		/// Renderer category for render stack 
		/// Note: Higher category will render later; refer to Scene::GraphicsContext::Renderer for further details.
		/// </summary>
		uint32_t RendererCategory()const;

		/// <summary>
		/// Sets renderer category for render stack
		/// Note: Higher category will render later; refer to Scene::GraphicsContext::Renderer for further details.
		/// </summary>
		/// <param name="category"> Category to set </param>
		void SetRendererCategory(uint32_t category);

		/// <summary> 
		/// Renderer priority for render stack 
		/// Note: Higher priority will render earlier within the same category; refer to Scene::GraphicsContext::Renderer for further details.
		/// </summary>
		uint32_t RendererPriority()const;

		/// <summary>
		/// Sets renderer priority for render stack
		/// Note: Higher priority will render earlier within the same category; refer to Scene::GraphicsContext::Renderer for further details.
		/// </summary>
		/// <param name="priority"> Priority to set </param>
		void SetRendererPriority(uint32_t priority);

		/// <summary>
		/// Projection matrix
		/// </summary>
		/// <param name="aspect"> Target aspect ratio (width / height) </param>
		/// <returns> Projection matrix </returns>
		Matrix4 ProjectionMatrix(float aspect)const;

		/// <summary> Layer mask for the underlying renderer </summary>
		GraphicsLayerMask Layers()const;

		/// <summary>
		/// Sets layer mask for the underlying renderer
		/// </summary>
		/// <param name="layers"></param>
		void RenderLayers(const GraphicsLayerMask& layers);

		/// <summary> Clear/Resolve flags for the underlying renderer </summary>
		Graphics::RenderPass::Flags RendererFlags()const;

		/// <summary>
		/// Sets clear/resolve flags for the underlying renderer
		/// </summary>
		/// <param name="flags"> Flags to use </param>
		void SetRendererFlags(Graphics::RenderPass::Flags flags);

		/// <summary> Lighting model used for rendering </summary>
		LightingModel* SceneLightingModel()const;

		/// <summary>
		/// Sets lighting model
		/// </summary>
		/// <param name="model"> Lighting model to use (nullptr will be interpreted as a default model) </param>
		void SetSceneLightingModel(LightingModel* model);

		/// <summary> Viewport descriptor of this camera </summary>
		const Jimara::ViewportDescriptor* ViewportDescriptor()const;

	protected:
		/// <summary> Invoked, whenever the component becomes recognized by engine </summary>
		virtual void OnComponentInitialized()override;

		/// <summary> Invoked, whenever the component becomes active in herarchy </summary>
		virtual void OnComponentEnabled()override;

		/// <summary> Invoked, whenever the component stops being active in herarchy </summary>
		virtual void OnComponentDisabled()override;

		/// <summary> Invoked, whenever the component gets destroyed </summary>
		virtual void OnComponentDestroyed()override;

		/// <summary> Invoked when the camera goes out of scope </summary>
		virtual void OnOutOfScope()const final override;

	private:
		// Field of view
		volatile float m_fieldOfView;

		// Close plane
		volatile float m_closePlane;

		// Far plane
		volatile float m_farPlane;

		// Clear color
		Vector4 m_clearColor;

		// Renderer category for render stack (higher category will render later)
		volatile uint32_t m_category = 0;

		// Renderer priority for render stack (higher priority will render earlier within the same category)
		volatile uint32_t m_priority = 0;

		// Render mask
		GraphicsLayerMask m_layers = GraphicsLayerMask::All();

		// Clear/Resolve flags
		Graphics::RenderPass::Flags m_rendererFlags =
			Graphics::RenderPass::Flags::CLEAR_COLOR | Graphics::RenderPass::Flags::CLEAR_DEPTH | Graphics::RenderPass::Flags::RESOLVE_COLOR;

		// Lighting model, used by the camera
		Reference<LightingModel> m_lightingModel;

		// Viewport
		const Reference<Jimara::ViewportDescriptor> m_viewport;

		// Render stack, this camera renders to
		Reference<RenderStack> m_renderStack;

		// Renderer, based on the camera and the LightingModel
		Reference<RenderStack::Renderer> m_renderer;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<Camera>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> void TypeIdDetails::GetTypeAttributesOf<Camera>(const Callback<const Object*>& report);
}
