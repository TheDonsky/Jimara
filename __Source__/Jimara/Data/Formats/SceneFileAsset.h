#pragma once
#include "../../Core/TypeRegistration/TypeRegistration.h"
#include "../AssetDatabase/AssetDatabase.h"
#include "../ComponentHierarchySpowner.h"


namespace Jimara {
	class SceneFileAsset;

	// Registration
	JIMARA_REGISTER_TYPE(Jimara::SceneFileAsset);
	
	// TypeId detail callbacks
	template<> void TypeIdDetails::OnRegisterType<SceneFileAsset>();
	template<> void TypeIdDetails::OnUnregisterType<SceneFileAsset>();
	template<> struct JIMARA_API TypeIdDetails::TypeDetails<SceneFileAsset> {
	private:
		static void OnRegisterType();
		static void OnUnregisterType();

		friend void TypeIdDetails::OnRegisterType<SceneFileAsset>();
		friend void TypeIdDetails::OnUnregisterType<SceneFileAsset>();
	};
	template<> inline void TypeIdDetails::GetParentTypesOf<SceneFileAsset>(const Callback<TypeId>& report) {
		report(TypeId::Of<ModifiableAsset::Of<ComponentHierarchySpowner>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<SceneFileAsset>(const Callback<const Object*>&) {}

#pragma warning(disable: 4250)
	/// <summary>
	/// File system asset for scenes
	/// </summary>
	class SceneFileAsset : public virtual ModifiableAsset::Of<EditableComponentHierarchySpowner> {
	public:
		/// <summary> Scene files do have external dependencies </summary>
		inline virtual bool HasRecursiveDependencies()const final override { return true; }

	protected:
		/// <summary>
		/// Loads scene tree data from the file
		/// </summary>
		/// <returns> Hierarchy spowner </returns>
		virtual Reference<EditableComponentHierarchySpowner> LoadItem() override;

		/// <summary>
		/// Stores scene tree data to the file
		/// </summary>
		/// <param name="resource"> Resource, that has previously been loaded with LoadItem() call and now has been changed (alegedly) </param>
		virtual void Store(EditableComponentHierarchySpowner* resource) override;

	private:
		// Internal asset importer class
		class Importer;

		// Lock for importer reference
		SpinLock m_importerLock;

		// Importer reference (Alive only while the FileSystemDB is alive and file exists; beyond that, Load/Store operations will fail miserably)
		Importer* m_importer;

		// Constructor
		SceneFileAsset(const GUID& guid, Importer* importer);

		// Type id details need access to internals
		friend struct TypeIdDetails::TypeDetails<SceneFileAsset>;
	};
#pragma warning(default: 4250)
}
