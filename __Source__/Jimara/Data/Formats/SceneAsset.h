#pragma once
#include "../TypeRegistration/TypeRegistartion.h"
#include "../AssetDatabase/AssetDatabase.h"
#include "../ComponentHeirarchySpowner.h"


namespace Jimara {
	class SceneAsset;

	// Registration
	JIMARA_REGISTER_TYPE(Jimara::SceneAsset);
	
	// TypeId detail callbacks
	template<> void TypeIdDetails::OnRegisterType<SceneAsset>();
	template<> void TypeIdDetails::OnUnregisterType<SceneAsset>();
	template<> struct TypeIdDetails::TypeDetails<SceneAsset> {
	private:
		static void OnRegisterType();
		static void OnUnregisterType();

		friend void TypeIdDetails::OnRegisterType<SceneAsset>();
		friend void TypeIdDetails::OnUnregisterType<SceneAsset>();
	};
	template<> inline void TypeIdDetails::GetParentTypesOf<SceneAsset>(const Callback<TypeId>& report) {
		report(TypeId::Of<ModifiableAsset::Of<ComponentHeirarchySpowner>>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<SceneAsset>(const Callback<const Object*>&) {}

#pragma warning(disable: 4250)
	class SceneAsset : public virtual ModifiableAsset::Of<ComponentHeirarchySpowner> {
	protected:
		virtual Reference<ComponentHeirarchySpowner> LoadItem() override;

		virtual void Store(ComponentHeirarchySpowner* resource) override;

	private:
		class Importer;

		SpinLock m_importerLock;
		const Importer* m_importer;

		SceneAsset(const GUID& guid, const Importer* importer);

		friend struct TypeIdDetails::TypeDetails<SceneAsset>;
	};
#pragma warning(default: 4250)
}
