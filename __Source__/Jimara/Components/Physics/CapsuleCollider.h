#pragma once
#include "Collider.h"


namespace Jimara {
	/// <summary> This will make sure, Component is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::CapsuleCollider);

	/// <summary>
	/// Capsule collider component
	/// </summary>
	class CapsuleCollider : public SingleMaterialCollider {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		/// <param name="radius"> Capsule radius </param>
		/// <param name="height"> Capsule mid-section height </param>
		/// <param name="material"> Physics material to use  </param>
		CapsuleCollider(Component* parent, const std::string_view& name = "Capsule", float radius = 0.5f, float height = 1.0f, Physics::PhysicsMaterial* material = nullptr);

		/// <summary> Component serializer </summary>
		virtual Reference<const ComponentSerializer> GetSerializer()const override;

		/// <summary> Collision capsule radius </summary>
		float Radius()const;

		/// <summary>
		/// Sets capsule radius
		/// </summary>
		/// <param name="value"> New radius to use </param>
		void SetRadius(float value);

		/// <summary> Capsule mid-section height </summary>
		float Height()const;

		/// <summary>
		/// Updates capsule mid-section height
		/// </summary>
		/// <param name="value"> New height to use </param>
		void SetHeight(float value);

		/// <summary> Capsule orientation </summary>
		Physics::CapsuleShape::Alignment Alignment()const;

		/// <summary>
		/// Sets capsule orientation
		/// </summary>
		/// <param name="alignment"> Orientation to use </param>
		void SetAlignment(Physics::CapsuleShape::Alignment alignment);

		/// <summary> Physics material, used by the collider (nullptr means the default material) </summary>
		virtual Physics::PhysicsMaterial* Material()const override;

		/// <summary>
		/// Updates physics material used by the collider (nullptr will result in some default material)
		/// </summary>
		/// <param name="material"> Material to use </param>
		virtual void SetMaterial(Physics::PhysicsMaterial* material)override;

	protected:
		/// <summary>
		/// Creates and maintains Physics::PhysicsCapsuleCollider
		/// </summary>
		/// <param name="old"> The last collider, created by this callback </param>
		/// <param name="body"> Body to attach the collider to </param>
		/// <param name="scale"> Collider scale </param>
		/// <param name="listener"> Listener to use with this collider (always the same, so no need to check for the one tied to 'old') </param>
		/// <returns> Physics::PhysicsCollider to be used by the collider </returns>
		virtual Reference<Physics::PhysicsCollider> GetPhysicsCollider(Physics::PhysicsCollider* old, Physics::PhysicsBody* body, Vector3 scale, Physics::PhysicsCollider::EventListener* listener) override;

	private:
		// Used material
		Reference<Physics::PhysicsMaterial> m_material;

		// Underlying capsule shape
		Physics::CapsuleShape m_capsule;

		// Type registration callbacks and friendship with type registrator
		JIMARA_DEFINE_TYPE_REGISTRATION_CALLBACKS;
		friend class BuiltInTypeRegistrator;
	};
}
