#pragma once
#include "Collider.h"
#include "../../Environment/Interfaces/BoundedObject.h"


namespace Jimara {
	/// <summary> This will make sure, Component is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::BoxCollider);

	/// <summary>
	/// Box collider component
	/// </summary>
	class JIMARA_API BoxCollider : public virtual SingleMaterialCollider, public virtual BoundedObject {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		/// <param name="size"> Collision box size </param>
		/// <param name="material"> Physics material to use </param>
		BoxCollider(Component* parent, const std::string_view& name = "BoxCollider", const Vector3& size = Vector3(1.0f), Physics::PhysicsMaterial* material = nullptr);

		/// <summary> Collision box size </summary>
		Vector3 Size()const;

		/// <summary>
		/// Sets collisio box size
		/// </summary>
		/// <param name="size"> New size to use </param>
		void SetSize(const Vector3& size);

		/// <summary> Physics material, used by the collider (nullptr means the default material) </summary>
		virtual Physics::PhysicsMaterial* Material()const override;

		/// <summary>
		/// Updates physics material used by the collider (nullptr will result in some default material)
		/// </summary>
		/// <param name="material"> Material to use </param>
		virtual void SetMaterial(Physics::PhysicsMaterial* material)override;

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		/// <summary>
		/// Reports actions associated with the component.
		/// </summary>
		/// <param name="report"> Actions will be reported through this callback </param>
		virtual void GetSerializedActions(Callback<Serialization::SerializedCallback> report)override;
		
		/// <summary> Retrieves MeshRenderer boundaries in world-space </summary>
		virtual AABB GetBoundaries()const override;

	protected:
		/// <summary>
		/// Creates and maintains Physics::PhysicsBoxCollider
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
		
		// Box size
		Vector3 m_size;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<BoxCollider>(const Callback<TypeId>& report) { 
		report(TypeId::Of<SingleMaterialCollider>());
		report(TypeId::Of<BoundedObject>());
	}
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<BoxCollider>(const Callback<const Object*>& report);
}
