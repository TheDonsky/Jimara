#pragma once
#include "../Transform.h"
#include "../../Data/ComponentHierarchySpowner.h"
#include "../../Environment/Interfaces/BoundedObject.h"


namespace Jimara {
	/// <summary> Let the system know the class exists </summary>
	JIMARA_REGISTER_TYPE(Jimara::Subscene);

	/// <summary>
	/// Subscene spawner (Similar, but not quite like a prefab instance, you can not change or view the subtree from the Editor)
	/// </summary>
	class JIMARA_API Subscene : public virtual Component, public virtual BoundedObject {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Spawner name </param>
		/// <param name="content"> Initial content to spawn </param>
		Subscene(Component* parent, const std::string_view& name = "Subscene", ComponentHierarchySpowner* content = nullptr);

		/// <summary> Virtual destructor </summary>
		virtual ~Subscene();

		/// <summary> Spawned content </summary>
		ComponentHierarchySpowner* Content()const;

		/// <summary>
		/// Replaces existing content with the new one
		/// </summary>
		/// <param name="content"> Content to spawn </param>
		void SetContent(ComponentHierarchySpowner* content);

		/// <summary> Reloads/Recreates content form ComponentHeirarchySpowner </summary>
		void Reload();

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		/// <summary> Retrieves MeshRenderer boundaries in world-space </summary>
		virtual AABB GetBoundaries()const override;

		/// <summary>
		/// Retrieves the subscene that spawned the hierarchy of the instance
		/// </summary>
		/// <param name="instance"> Any component from the hierarchy, spawned by the subscene </param>
		/// <returns> Spowner Subscene </returns>
		static Subscene* GetSubscene(Component* instance);

	private:
		// Current content
		Reference<ComponentHierarchySpowner> m_content;

		// Last known position, rotation and scale:
		Vector3 m_lastPosition = Vector3(0.0f);
		Vector3 m_lastEulerAngles = Vector3(0.0f);
		Vector3 m_lastScale = Vector3(1.0f);

		// Spawned hierarchy
		Reference<Component> m_spownedHierarchy;

		// Helper utilities
		struct Helpers;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<Subscene>(const Callback<TypeId>& report) { 
		report(TypeId::Of<Component>()); 
		report(TypeId::Of<BoundedObject>());
	}
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<Subscene>(const Callback<const Object*>& report);
}
