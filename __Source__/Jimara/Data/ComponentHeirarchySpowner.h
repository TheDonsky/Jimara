#pragma once
#include "../Environment/Scene/Scene.h"

namespace Jimara {
	/// <summary>
	/// Resource, that can spown a component subtree on demand
	/// </summary>
	class ComponentHeirarchySpowner : public virtual Resource {
	public:
		/// <summary> Information about resource loading progress </summary>
		struct ProgressInfo {
			/// <summary> Number of resources to load </summary>
			size_t numResources;

			/// <summary> Number of resources already loaded </summary>
			size_t numLoaded;

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="total"> numResources </param>
			/// <param name="loaded"> numLoaded </param>
			inline ProgressInfo(size_t total = 0, size_t loaded = 0) : numResources(total), numLoaded(loaded) {}
		};

		/// <summary>
		/// Spowns a component subtree
		/// Note: Can be invoked synchronously, or asynchronously from an arbitrary non-update thread
		/// </summary>
		/// <param name="parent"> Parent component for the subtree </param>
		/// <param name="reportProgress"> 
		///		This callback will be used to report resource loading progress (useful for spowning from external threads) 
		/// </param>
		/// <param name="spownAsynchronous"> 
		///		If true, ComponentHeirarchySpowner will treat current thread as an "external" thread that is different from the main update thread 
		///		and will allow itself liberty to schedule substeps on the main update queue (ei ExecuteAfterUpdate), and wait for their completion.
		///		Notes: 
		///			0. Setting this flag may result in a deadlock if called from the main update thread, as well as in case the update loop is not running at all;
		///			1. In case of an external thread, this flag is optional, since all component spowning is expected to happen atomically
		///			under the main update lock, but this one can make the framerate smoother by preventing any kind of a conjestion;
		///			2. Not all implementations are forced to use update queue, this flag simply gives them a permission to do so.
		/// </param>
		/// <returns> "Root-level" component of the spowned subtree (or nullptr if failed) </returns>
		virtual Reference<Component> SpownHeirarchy(
			Component* parent, 
			Callback<ProgressInfo> reportProgress = Callback(Unused<ProgressInfo>), 
			bool spownAsynchronous = false) = 0;
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
