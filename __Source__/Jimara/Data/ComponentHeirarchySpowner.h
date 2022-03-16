#pragma once
#include "../Environment/Scene/Scene.h"

namespace Jimara {
	/// <summary>
	/// Resource, that can spown a component subtree on demand
	/// </summary>
	class ComponentHeirarchySpowner : public virtual Resource {
	public:
		/// <summary>
		/// Spowns a component subtree
		/// Note: Spowners are expected to have resources preloaded and ready to go, 
		///		so this is relatively safe to call from the main update thread.
		/// </summary>
		/// <param name="parent"> Parent component for the subtree </param>
		/// <returns> "Root-level" component of the spowned subtree (or nullptr if failed) </returns>
		virtual Reference<Component> SpownHeirarchy(Component* parent) = 0;
	};

	// TypeIdDetails for ComponentHeirarchySpowner
	template<> inline void TypeIdDetails::GetParentTypesOf<ComponentHeirarchySpowner>(const Callback<TypeId>& report) { report(TypeId::Of<Resource>()); }

	/// <summary>
	/// EditableComponentHeirarchySpowner that can be updated
	/// </summary>
	class EditableComponentHeirarchySpowner : public virtual ComponentHeirarchySpowner {
	public:
		/// <summary>
		/// Updates internal structures, so that the next time we want to spown something, 
		/// we use new heirarchy instead
		/// </summary>
		/// <param name="parent"> Root object of the Component subtree </param>
		virtual void StoreHeirarchyData(Component* parent) = 0;

		/// <summary>
		/// Updates internal structures, so that the next time we want to spown something, 
		/// we use new heirarchy instead and also invokes corresponding ModifiableAsset::StoreResource() 
		/// call to permanently store the changes when and if possible
		/// </summary>
		/// <param name="parent"> Root object of the Component subtree </param>
		inline void StoreHeirarchyAndAssetData(Component* parent) {
			Reference<Resource> self(this);
			StoreHeirarchyData(parent);
			ModifiableAsset* asset = dynamic_cast<ModifiableAsset*>(GetAsset());
			if (asset != nullptr)
				asset->StoreResource();
		}
	};

	// TypeIdDetails for EditableComponentHeirarchySpowner
	template<> inline void TypeIdDetails::GetParentTypesOf<EditableComponentHeirarchySpowner>(const Callback<TypeId>& report) { report(TypeId::Of<ComponentHeirarchySpowner>()); }
}
