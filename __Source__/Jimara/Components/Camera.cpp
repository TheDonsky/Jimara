#include "Camera.h"


namespace Jimara {
	Camera::Camera(Component* parent, const std::string& name, float fieldOfView, float closePlane, float farPlane)
		: Component(parent, name) {
		SetFieldOfView(fieldOfView);
		SetClosePlane(closePlane);
		SetFarPlane(farPlane);
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

	void Camera::SetFarPlane(float value) { m_farPlane = max(m_closePlane, value); }

	Matrix4 Camera::ProjectionMatrix(float aspect)const { 
		Matrix4 projection = glm::perspective(glm::radians(m_fieldOfView), aspect, m_closePlane, m_farPlane);
		projection[2] *= -1.0f;
		return projection;
	}
}
