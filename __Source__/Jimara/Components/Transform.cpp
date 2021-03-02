#include "Transform.h"


namespace Jimara {
	Transform::Transform(Component* parent, const std::string& name, const Vector3& localPosition, const Vector3& localEulerAngles, const Vector3& localScale)
		: Component(parent, name)
		, m_localPosition(localPosition), m_localEulerAngles(localEulerAngles), m_localScale(localScale)
		, m_matrixDirty(true), m_rotationMatrix(1.0f), m_trasnformationMatrix(Matrix4(1.0f)) {}


	Vector3 Transform::LocalPosition()const { return m_localPosition; }

	void Transform::SetLocalPosition(const Vector3& value) { 
		m_localPosition = value; 
		m_matrixDirty = true; 
	}

	Vector3 Transform::WorldPosition()const {
		const Matrix4 worldMatrix = WorldMatrix();
		return Vector3(worldMatrix[0][3], worldMatrix[1][3], worldMatrix[2][3]);
	}

	void Transform::SetWorldPosition(const Vector3& value) {
		Context()->Log()->Fatal("Transform::SetWorldPosition - Not yet implemented!");
		m_matrixDirty = true;
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
		m_matrixDirty = true;
	}


	Vector3 Transform::LocalScale()const { return m_localScale; }

	void Transform::SetLocalScale(const Vector3& value) {
		m_localScale = value;
		m_matrixDirty = true;
	}


	const Matrix4& Transform::LocalMatrix()const {
		UpdateMatrices();
		return m_trasnformationMatrix;
	}

	const Matrix4& Transform::LocalRotationMatrix()const {
		UpdateMatrices();
		return m_rotationMatrix;
	}

	Matrix4 Transform::WorldMatrix()const {
		const Transform* parent = GetComponentInParents<Transform>(false);
		if (parent != nullptr) return parent->WorldMatrix() * LocalMatrix();
		else return LocalMatrix();
	}

	Matrix4 Transform::WorldRotationMatrix()const {
		const Transform* parent = GetComponentInParents<Transform>(false);
		if (parent != nullptr) return parent->WorldRotationMatrix() * LocalRotationMatrix();
		else return LocalRotationMatrix();
	}

	Vector3 Transform::LocalToParentSpaceDirection(const Vector3& localDirection)const {
		return Vector4(localDirection, 1.0f) * LocalRotationMatrix();
	}

	Vector3 Transform::LocalForward()const {
		UpdateMatrices();
		return Vector3(m_rotationMatrix[0][2], m_rotationMatrix[1][2], m_rotationMatrix[2][2]);
	}

	Vector3 Transform::LocalRight()const {
		UpdateMatrices();
		return Vector3(m_rotationMatrix[0][0], m_rotationMatrix[1][0], m_rotationMatrix[2][0]);
	}

	Vector3 Transform::LocalUp()const {
		UpdateMatrices();
		return Vector3(m_rotationMatrix[0][1], m_rotationMatrix[1][1], m_rotationMatrix[2][1]);
	}

	Vector3 Transform::LocalToWorldDirection(const Vector3& localDirection)const {
		return Vector4(localDirection, 1.0f) * WorldRotationMatrix();
	}

	Vector3 Transform::Forward()const {
		const Matrix4 rotation = WorldRotationMatrix();
		return Vector3(rotation[0][2], rotation[1][2], rotation[2][2]);
	}

	Vector3 Transform::Right()const {
		const Matrix4 rotation = WorldRotationMatrix();
		return Vector3(rotation[0][0], rotation[1][0], rotation[2][0]);
	}

	Vector3 Transform::Up()const {
		const Matrix4 rotation = WorldRotationMatrix();
		return Vector3(rotation[0][1], rotation[1][1], rotation[2][1]);
	}

	Vector3 Transform::LocalToParentSpacePosition(const Vector3& localDirection)const {
		return Vector4(localDirection, 1.0f) * LocalMatrix();
	}

	Vector3 Transform::LocalToWorldPosition(const Vector3& localDirection)const {
		return Vector4(localDirection, 1.0f) * WorldMatrix();
	}




	void Transform::UpdateMatrices()const {
		if (m_matrixDirty) {
			m_rotationMatrix = MatrixFromEulerAngles(m_localEulerAngles);

			m_trasnformationMatrix = m_rotationMatrix;
			{
				m_trasnformationMatrix[0][0] *= m_localScale.x;
				m_trasnformationMatrix[1][0] *= m_localScale.x;
				m_trasnformationMatrix[2][0] *= m_localScale.x;

				m_trasnformationMatrix[0][1] *= m_localScale.y;
				m_trasnformationMatrix[1][1] *= m_localScale.y;
				m_trasnformationMatrix[2][1] *= m_localScale.y;

				m_trasnformationMatrix[0][2] *= m_localScale.z;
				m_trasnformationMatrix[1][2] *= m_localScale.z;
				m_trasnformationMatrix[2][2] *= m_localScale.z;
			}
			{
				m_trasnformationMatrix[0][3] = m_localPosition.x;
				m_trasnformationMatrix[1][3] = m_localPosition.y;
				m_trasnformationMatrix[2][3] = m_localPosition.z;
			}
			m_matrixDirty = false;
		}
	}
}
