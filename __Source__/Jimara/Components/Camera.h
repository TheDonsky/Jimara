#pragma once
namespace Jimara { class Camera; }
#include "Component.h"
#include "Transform.h"
#include "../Environment/GraphicsContext/LightingModels/LightingModel.h"


namespace Jimara {
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
		/// Projection matrix
		/// </summary>
		/// <param name="aspect"> Target aspect ratio (width / height) </param>
		/// <returns> Projection matrix </returns>
		Matrix4 ProjectionMatrix(float aspect)const;

		/// <summary> Lighting model, used for rendering </summary>
		LightingModel* SceneLightingModel()const;

		/// <summary>
		/// Sets lighting model
		/// </summary>
		/// <param name="model"> Lighting model to use (nullptr will disable camera) </param>
		void SetSceneLightingModel(LightingModel* model);

		/// <summary> Renderer, based on the camera and the LightingModel </summary>
		Graphics::ImageRenderer* Renderer()const;


	private:
		// Field of view
		volatile float m_fieldOfView;

		// Close plane
		volatile float m_closePlane;

		// Far plane
		volatile float m_farPlane;

		// Clear color
		Vector4 m_clearColor;

		// If false, the camera will become permanently disabled
		volatile bool m_isAlive;

		// Lighting model, used by the camera
		Reference<LightingModel> m_lightingModel;

		// Renderer, based on the camera and the LightingModel
		Reference<Graphics::ImageRenderer> m_renderer;

		// Destroys the renderer
		void DisposeRenderer();

		// Permanently disables the camera
		void DestroyCamera(Component*);
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<Camera>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
}
