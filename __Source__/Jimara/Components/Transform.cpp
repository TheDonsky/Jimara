#include "Transform.h"


namespace Jimara {
	Transform::Transform(Component* parent, const std::string& name, const Vector3& localPosition, const Vector3& localEulerAngles, const Vector3& localScale)
		: Component(parent, name)
		, m_localPosition(localPosition), m_localEulerAngles(localEulerAngles), m_localScale(localScale)
		, m_matrixDirty(true), m_trasnformationMatrix(Matrix4(1.0f)) {}


	Vector3 Transform::LocalPosition()const { return m_localPosition; }

	void Transform::SetLocalPosition(const Vector3& value) { 
		m_localPosition = value; 
		m_matrixDirty = true; 
	}

	Vector3 Transform::WorldPosition()const {
		Context()->Log()->Fatal("Transform::WorldPosition - Not yet implemented!");
		return Vector3();
	}

	void Transform::SetWorldPosition(const Vector3& value) {
		Context()->Log()->Fatal("Transform::SetWorldPosition - Not yet implemented!");
	}


	Vector3 Transform::LocalEulerAngles()const { return m_localEulerAngles; }

	void Transform::SetLocalEulerAngles(const Vector3& value) {
		m_localEulerAngles = value;
		m_matrixDirty = true;
	}

	Vector3 Transform::WorldEulerAngles()const {
		Context()->Log()->Fatal("Transform::WorldEulerAngles - Not yet implemented!");
		return Vector3();
	}

	void Transform::SetWorldEulerAngles(const Vector3& value) {
		Context()->Log()->Fatal("Transform::SetWorldEulerAngles - Not yet implemented!");
	}


	Vector3 Transform::LocalScale()const { return m_localScale; }

	void Transform::SetLocalScale(const Vector3& value) {
		m_localScale = value;
		m_matrixDirty = true;
	}
}
