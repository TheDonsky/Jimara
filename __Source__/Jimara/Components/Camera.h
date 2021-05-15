#pragma once
#include "Component.h"
#include "Transform.h"


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
		Camera(Component* parent, const std::string& name = "Camera", float fieldOfView = 64.0f, float closePlane = 0.001f, float farPlane = 10000.0f);

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

		/// <summary>
		/// Projection matrix
		/// </summary>
		/// <param name="aspect"> Target aspect ratio (width / height) </param>
		/// <returns> Projection matrix </returns>
		Matrix4 ProjectionMatrix(float aspect)const;

	private:
		// Field of view
		volatile float m_fieldOfView;

		// Close plane
		volatile float m_closePlane;

		// Far plane
		volatile float m_farPlane;
	};
}
