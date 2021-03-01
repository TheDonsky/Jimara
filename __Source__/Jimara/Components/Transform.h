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


	private:
		Vector3 m_localPosition;
		Vector3 m_localEulerAngles;
		Vector3 m_localScale;
		
		mutable std::atomic<bool> m_matrixDirty;
		mutable Matrix4 m_trasnformationMatrix;
	};
}
