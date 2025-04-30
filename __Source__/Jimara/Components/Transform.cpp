#include "Transform.h"
#include "../Data/Serialization/Attributes/EulerAnglesAttribute.h"
#include "../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	Transform::Transform(Component* parent, const std::string_view& name, const Vector3& localPosition, const Vector3& localEulerAngles, const Vector3& localScale)
		: Component(parent, name)
		, m_localPosition(localPosition), m_localEulerAngles(localEulerAngles), m_localScale(localScale)
		, m_frameCachedWorldMatrix(Math::Identity()), m_lastCachedFrameIndex(parent->Context()->FrameIndex() - 1u) {}

	Transform::Transform(SceneContext* context, const std::string_view& name) 
		: Component(context, name) {}

	template<> void TypeIdDetails::GetTypeAttributesOf<Transform>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<Transform>(
			"Transform", "Jimara/Transform", "Transform Component");
		report(factory);
	}

	Vector3 Transform::LocalPosition()const { return m_localPosition; }

	void Transform::SetLocalPosition(const Vector3& value) { 
		m_localPosition = value;
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
	}

	Vector3 Transform::LossyScale()const {
		const Matrix4 worldRotation = WorldRotationMatrix();
		const Matrix4 worldMatrix = WorldMatrix();
		return Math::LossyScale(WorldMatrix(), WorldRotationMatrix());
	}


	Matrix4 Transform::LocalMatrix()const {
		Matrix4 matrix = LocalRotationMatrix();
		matrix[0u] *= m_localScale.x;
		matrix[1u] *= m_localScale.y;
		matrix[2u] *= m_localScale.z;
		matrix[3] = Vector4(m_localPosition, 1.0f);
		return matrix;
	}

	Matrix4 Transform::LocalRotationMatrix()const {
		return Math::MatrixFromEulerAngles(m_localEulerAngles);
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
		return Math::MatrixFromEulerAngles(m_localEulerAngles)[2u];
	}

	Vector3 Transform::LocalRight()const {
		return Math::MatrixFromEulerAngles(m_localEulerAngles)[0u];
	}

	Vector3 Transform::LocalUp()const {
		return Math::MatrixFromEulerAngles(m_localEulerAngles)[1u];
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

	const Matrix4& Transform::FrameCachedWorldMatrix()const {
		struct Calculator {
			inline static const Matrix4& Calculate(const Transform* self, uint64_t frameId) {
				if (self->m_lastCachedFrameIndex.load() != frameId) {
					const Transform* parent = self->GetComponentInParents<Transform>(false);
					self->m_frameCachedWorldMatrix = (parent == nullptr) ? self->LocalMatrix() :
						(Calculate(parent, frameId) * self->LocalMatrix());
					self->m_lastCachedFrameIndex = frameId;
				}
				return self->m_frameCachedWorldMatrix;
			}
		};
		return Calculator::Calculate(this, Context()->FrameIndex());
	}

	void Transform::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(LocalPosition, SetLocalPosition, "Position", "Relative position in parent space");
			JIMARA_SERIALIZE_FIELD_GET_SET(LocalEulerAngles, SetLocalEulerAngles,
				"Rotation", "Relative euler angles in parent space", Object::Instantiate<Serialization::EulerAnglesAttribute>());
			JIMARA_SERIALIZE_FIELD_GET_SET(LocalScale, SetLocalScale, "Scale", "Relative scale in parent space");
		};
	}

	void Transform::GetSerializedActions(Callback<Serialization::SerializedCallback> report) {
		Component::GetSerializedActions(report);

		// Local Position
		{
			static const auto serializer = Serialization::DefaultSerializer<Vector3>::Create(
				"Local Position", "Local position will be set to this");
			report(Serialization::SerializedCallback::Create<const Vector3&>::From(
				"SetPosition", Callback<const Vector3&>(&Transform::SetLocalPosition, this), serializer));
		}

		// Local Euler angles
		{
			static const auto serializer = Serialization::DefaultSerializer<Vector3>::Create(
				"Local Euler Angles", "Local euler angles will be set to this");
			report(Serialization::SerializedCallback::Create<const Vector3&>::From(
				"SetRotation", Callback<const Vector3&>(&Transform::SetLocalEulerAngles, this), serializer));
		}

		// Local scale
		{
			static const auto serializer = Serialization::DefaultSerializer<Vector3>::Create(
				"Local Scale", "Local scale will be set to this");
			report(Serialization::SerializedCallback::Create<const Vector3&>::From(
				"SetScale", Callback<const Vector3&>(&Transform::SetLocalScale, this), serializer));
		}

		// World-Space Position
		{
			static const auto serializer = Serialization::DefaultSerializer<Vector3>::Create(
				"World Position", "World-Space position will be set to this");
			report(Serialization::SerializedCallback::Create<const Vector3&>::From(
				"SetWorldPosition", Callback<const Vector3&>(&Transform::SetWorldPosition, this), serializer));
		}

		// World-Space Euler angles
		{
			static const auto serializer = Serialization::DefaultSerializer<Vector3>::Create(
				"World Euler Angles", "World-Space euler angles will be set to this");
			report(Serialization::SerializedCallback::Create<const Vector3&>::From(
				"SetWorldRotation", Callback<const Vector3&>(&Transform::SetWorldEulerAngles, this), serializer));
		}
	}
}
