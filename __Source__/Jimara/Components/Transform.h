#pragma once
#include "Component.h"
#include "../Math/Math.h"


namespace Jimara { 
	class Transform : public virtual Component {
	public:
		Transform(Component* parent, const std::string& name
			, const Vector3& localPosition = Vector3(0.0f, 0.0f, 0.0f)
			, const Vector3& localEulerAngles = Vector3(0.0f, 0.0f, 0.0f)
			, const Vector3& localScale = Vector3(1.0f, 1.0f, 1.0f));


		Vector3 LocalPosition()const;

		void SetLocalPosition(const Vector3& value);

		Vector3 WorldPosition()const;

		void SetWorldPosition(const Vector3& value);


		Vector3 LocalEulerAngles()const;

		void SetLocalEulerAngles(const Vector3& value);

		Vector3 WorldEulerAngles()const;

		void SetWorldEulerAngles(const Vector3& value);


		Vector3 LocalScale()const;
		
		void SetLocalScale(const Vector3& value);


		const Matrix4& LocalMatrix()const;

		const Matrix4& LocalRotationMatrix()const;

		Matrix4 WorldMatrix()const;

		Matrix4 WorldRotationMatrix()const;


		Vector3 LocalToParentSpaceDirection(const Vector3& localDirection)const;

		Vector3 LocalForward()const;

		Vector3 LocalRight()const;

		Vector3 LocalUp()const;

		Vector3 LocalToWorldDirection(const Vector3& localDirection)const;

		Vector3 Forward()const;

		Vector3 Right()const;

		Vector3 Up()const;


		Vector3 LocalToParentSpacePosition(const Vector3& localDirection)const;

		Vector3 LocalToWorldPosition(const Vector3& localDirection)const;




	private:
		Vector3 m_localPosition;
		Vector3 m_localEulerAngles;
		Vector3 m_localScale;
		
		mutable std::atomic<bool> m_matrixDirty;
		mutable Matrix4 m_rotationMatrix;
		mutable Matrix4 m_trasnformationMatrix;

		void UpdateMatrices()const;
	};
}
