#pragma once
#include "Collider.h"


namespace Jimara {
	/// <summary> This will make sure, Component is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::SphereCollider);

	/// <summary>
	/// Sphere collider component
	/// </summary>
	class SphereCollider : public SingleMaterialCollider {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		/// <param name="radius"> Sphere radius </param>
		/// <param name="material"> Physics material to use </param>
		SphereCollider(Component* parent, const std::string_view& name = "SphereCollider", float radius = 0.5f, Physics::PhysicsMaterial* material = nullptr);

		/// <summary> Collider radius </summary>
		float Radius()const;

		/// <summary>
		/// Sets the sphere radius
		/// </summary>
		/// <param name="value"> Radius to use </param>
		void SetRadius(float value);

		/// <summary> Physics material, used by the collider (nullptr means the default material) </summary>
		virtual Physics::PhysicsMaterial* Material()const override;

		/// <summary>
		/// Updates physics material used by the collider (nullptr will result in some default material)
		/// </summary>
		/// <param name="material"> Material to use </param>
		virtual void SetMaterial(Physics::PhysicsMaterial* material)override;

	protected:
		/// <summary>
		/// Creates and maintains Physics::PhysicsSphereCollider
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

		// Sphere radius
		std::atomic<float> m_radius;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<SphereCollider>(const Callback<TypeId>& report) { report(TypeId::Of<SingleMaterialCollider>()); }
	template<> void TypeIdDetails::GetTypeAttributesOf<SphereCollider>(const Callback<const Object*>& report);
}
