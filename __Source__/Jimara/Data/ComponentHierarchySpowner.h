#pragma once
#include "../Environment/Scene/Scene.h"

namespace Jimara {
	/// <summary>
	/// Resource, that can spown a component subtree on demand
	/// </summary>
	class ComponentHierarchySpowner : public virtual Resource {
	public:
		/// <summary>
		/// Spowns a component subtree
		/// Note: Spowners are expected to have resources preloaded and ready to go, 
		///		so this is relatively safe to call from the main update thread.
		/// </summary>
		/// <param name="parent"> Parent component for the subtree </param>
		/// <returns> "Root-level" component of the spowned subtree (or nullptr if failed) </returns>
		virtual Reference<Component> SpownHierarchy(Component* parent) = 0;
	};

	// TypeIdDetails for ComponentHierarchySpowner
	template<> inline void TypeIdDetails::GetParentTypesOf<ComponentHierarchySpowner>(const Callback<TypeId>& report) { report(TypeId::Of<Resource>()); }

	/// <summary>
	/// EditableComponentHierarchySpowner that can be updated
	/// </summary>
	class EditableComponentHierarchySpowner : public virtual ComponentHierarchySpowner {
	public:
		/// <summary>
		/// Updates internal structures, so that the next time we want to spown something, 
		/// we use new Hierarchy instead
		/// </summary>
		/// <param name="parent"> Root object of the Component subtree </param>
		virtual void StoreHierarchyData(Component* parent) = 0;

		/// <summary>
		/// Updates internal structures, so that the next time we want to spown something, 
		/// we use new Hierarchy instead and also invokes corresponding ModifiableAsset::StoreResource() 
		/// call to permanently store the changes when and if possible
		/// </summary>
		/// <param name="parent"> Root object of the Component subtree </param>
		inline void StoreHierarchyAndAssetData(Component* parent) {
			Reference<Resource> self(this);
			StoreHierarchyData(parent);
			ModifiableAsset* asset = dynamic_cast<ModifiableAsset*>(GetAsset());
			if (asset != nullptr)
				asset->StoreResource();
		}
	};

	// TypeIdDetails for EditableComponentHierarchySpowner
	template<> inline void TypeIdDetails::GetParentTypesOf<EditableComponentHierarchySpowner>(const Callback<TypeId>& report) { report(TypeId::Of<ComponentHierarchySpowner>()); }
}
