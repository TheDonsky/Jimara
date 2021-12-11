#include "Transform.h"
#include "../Data/Serialization/Attributes/EulerAnglesAttribute.h"


namespace Jimara {
	Transform::Transform(Component* parent, const std::string_view& name, const Vector3& localPosition, const Vector3& localEulerAngles, const Vector3& localScale)
		: Component(parent, name)
		, m_localPosition(localPosition), m_localEulerAngles(localEulerAngles), m_localScale(localScale)
		, m_matrixDirty(true), m_rotationMatrix(1.0f), m_transformationMatrix(Matrix4(1.0f)) {}

	namespace {
		class TransformSerializer : public virtual ComponentSerializer::Of<Transform> {
		public:
			inline TransformSerializer()
				: ItemSerializer("Jimara/Transform", "Transform component") {}

			inline virtual void SerializeTarget(const Callback<Serialization::SerializedObject>& recordElement, Transform* target)const override {
				TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>()->SerializeComponent(recordElement, target);
				
				static const Reference<const Serialization::Vector3Serializer> positionSerializer = Serialization::Vector3Serializer::Create<Transform>(
					"Position", "Relative position in parent space",
					[](Transform* target) -> Vector3 { return target->LocalPosition(); },
					[](const Vector3& value, Transform* target) { target->SetLocalPosition(value); });
				recordElement(Serialization::SerializedObject(positionSerializer, target));

				static const Reference<const Serialization::Vector3Serializer> rotationSerializer = Serialization::Vector3Serializer::Create<Transform>(
					"Rotation", "Relative euler angles in parent space",
					[](Transform* target) { return target->LocalEulerAngles(); },
					[](const Vector3& value, Transform* target) { target->SetLocalEulerAngles(value); },
					{ Object::Instantiate<Serialization::EulerAnglesAttribute>() } );
				recordElement(Serialization::SerializedObject(rotationSerializer, target));

				static const Reference<const Serialization::Vector3Serializer> scaleSerializer = Serialization::Vector3Serializer::Create<Transform>(
					"Scale", "Relative scale in parent space",
					[](Transform* target) { return target->LocalScale(); },
					[](const Vector3& value, Transform* target) { target->SetLocalScale(value); });
				recordElement(Serialization::SerializedObject(scaleSerializer, target));
			}

			inline static const ComponentSerializer* Instance() {
				static const TransformSerializer instance;
				return &instance;
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Transform>(const Callback<const Object*>& report) { report(TransformSerializer::Instance()); }

	Vector3 Transform::LocalPosition()const { return m_localPosition; }

	void Transform::SetLocalPosition(const Vector3& value) { 
		m_localPosition = value; 
		m_matrixDirty = true; 
	}

	Vector3 Transform::WorldPosition()const {
		return WorldMatrix()[3];
	}

	void Transform::SetWorldPosition(const Vector3& value) {
		const Transform* parent = GetComponentInParents<Transform>(false);
		if (parent == nullptr) SetLocalPosition(value);
		else SetLocalPosition(Math::Inverse(parent->WorldMatrix()) * Vector4(value, 1));
	}


	Vector3 Transform::LocalEulerAngles()const { return m_localEulerAngles; }

	void Transform::SetLocalEulerAngles(const Vector3& value) {
		m_localEulerAngles = value;
		m_matrixDirty = true;
	}

	Vector3 Transform::WorldEulerAngles()const {
		const Transform* parent = GetComponentInParents<Transform>(false);
		if (parent == nullptr) return m_localEulerAngles;
		else return Math::EulerAnglesFromMatrix(parent->WorldRotationMatrix() * LocalRotationMatrix());
	}

	void Transform::SetWorldEulerAngles(const Vector3& value) {
		const Transform* parent = GetComponentInParents<Transform>(false);
		if (parent == nullptr) SetLocalEulerAngles(value);
		else SetLocalEulerAngles(Math::EulerAnglesFromMatrix(Math::Inverse(parent->WorldRotationMatrix()) * Math::MatrixFromEulerAngles(value)));
	}


	Vector3 Transform::LocalScale()const { return m_localScale; }

	void Transform::SetLocalScale(const Vector3& value) {
		m_localScale = value;
		m_matrixDirty = true;
	}

	Vector3 Transform::LossyScale()const {
		const Matrix4 worldRotation = WorldRotationMatrix();
		const Matrix4 worldMatrix = WorldMatrix();
		return Math::LossyScale(WorldMatrix(), WorldRotationMatrix());
	}


	const Matrix4& Transform::LocalMatrix()const {
		UpdateMatrices();
		return m_transformationMatrix;
	}

	const Matrix4& Transform::LocalRotationMatrix()const {
		UpdateMatrices();
		return m_rotationMatrix;
	}

	Matrix4 Transform::WorldMatrix()const {
		Matrix4 result = LocalMatrix();
		const Transform* ptr = GetComponentInParents<Transform>(false);
		while (ptr != nullptr) {
			result = ptr->LocalMatrix() * result;
			ptr = ptr->GetComponentInParents<Transform>(false);
		}
		return result;
	}

	Matrix4 Transform::WorldRotationMatrix()const {
		Matrix4 result = LocalRotationMatrix();
		const Transform* ptr = GetComponentInParents<Transform>(false);
		while (ptr != nullptr) {
			result = ptr->LocalRotationMatrix() * result;
			ptr = ptr->GetComponentInParents<Transform>(false);
		}
		return result;
	}


	Vector3 Transform::LocalToParentSpaceDirection(const Vector3& localDirection)const {
		return LocalRotationMatrix() * Vector4(localDirection, 1.0f);
	}

	Vector3 Transform::LocalForward()const {
		UpdateMatrices();
		return m_rotationMatrix[2];
	}

	Vector3 Transform::LocalRight()const {
		UpdateMatrices();
		return m_rotationMatrix[0];
	}

	Vector3 Transform::LocalUp()const {
		UpdateMatrices();
		return m_rotationMatrix[1];
	}

	Vector3 Transform::LocalToWorldDirection(const Vector3& localDirection)const {
		return WorldRotationMatrix() * Vector4(localDirection, 1.0f);
	}

	Vector3 Transform::Forward()const {
		return WorldRotationMatrix()[2];
	}

	Vector3 Transform::Right()const {
		return WorldRotationMatrix()[0];
	}

	Vector3 Transform::Up()const {
		return WorldRotationMatrix()[1];
	}

	Vector3 Transform::LocalToParentSpacePosition(const Vector3& localPosition)const {
		return LocalMatrix() * Vector4(localPosition, 1.0f);
	}

	Vector3 Transform::LocalToWorldPosition(const Vector3& localPosition)const {
		return WorldMatrix() * Vector4(localPosition, 1.0f);
	}

	void Transform::LookAt(const Vector3& target, const Vector3& up) {
		LookTowards(target - WorldPosition(), up);
	}

	void Transform::LookTowards(const Vector3& direction, const Vector3& up) {
		SetWorldEulerAngles(Math::EulerAnglesFromMatrix(Math::LookTowards(direction, up)));
	}

	void Transform::LookAtLocal(const Vector3& target, const Vector3& up) {
		LookTowardsLocal(target - m_localPosition, up);
	}

	void Transform::LookTowardsLocal(const Vector3& direction, const Vector3& up) {
		SetLocalEulerAngles(Math::EulerAnglesFromMatrix(Math::LookTowards(direction, up)));
	}




	void Transform::UpdateMatrices()const {
		if (m_matrixDirty) {
			std::unique_lock<SpinLock> lock(m_matrixLock);
			if (m_matrixDirty) {
				m_rotationMatrix = Math::MatrixFromEulerAngles(m_localEulerAngles);
				m_transformationMatrix = m_rotationMatrix;
				{
					m_transformationMatrix[0] *= m_localScale.x;
					m_transformationMatrix[1] *= m_localScale.y;
					m_transformationMatrix[2] *= m_localScale.z;
				}
				m_transformationMatrix[3] = Vector4(m_localPosition, 1.0f);
				m_matrixDirty = false;
			}
		}
	}
}
