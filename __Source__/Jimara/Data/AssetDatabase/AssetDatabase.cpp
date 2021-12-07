#include "AssetDatabase.h"
#include <cassert>



namespace Jimara {
	Asset* Resource::GetAsset()const { return m_asset; }

	void Resource::OnOutOfScope()const {
		Reference<Asset> asset;
		{
			std::unique_lock<SpinLock> lock(m_assetLock);
			asset = m_asset;
		}

		if (asset != nullptr) {
			// Lock to prevent overlap with Asset::Load():
			std::unique_lock<std::mutex> lock(asset->m_resourceLock);
			
			// If there has been an overlap with Asset::Load(), we should cancel destruction:
			if (RefCount() > 0) return;
			
			// Otherwise, we are free to destroy all connection between the resource and the asset:
			Resource* resource = asset->m_resource;
			if (resource == this) {
				asset->m_resource = nullptr;
				std::unique_lock<SpinLock> lock(m_assetLock);
				if (m_asset == asset)
					m_asset = nullptr;

				// Let the asset reclaim the resource if it does not have to be deleted:
				asset->UnloadResource(resource);
				return;
			}
		}
		Object::OnOutOfScope();
	}


	const GUID& Asset::Guid()const { return m_guid; }

	Reference<Resource> Asset::GetLoaded()const {
		Reference<Resource> resource;
		{
			std::unique_lock<std::mutex> lock(m_resourceLock);
			resource = m_resource;
		}
		return resource;
	}

	Reference<Resource> Asset::Load() {
		// Only one thread at a time can 'load'
		std::unique_lock<std::mutex> lock(m_resourceLock);

		// If we already have the resource loaded, we can just return it (Note that the resource uses the same lock, making this safe-ish enough):
		if (m_resource != nullptr && m_resource->RefCount() > 0 && m_resource->m_asset == this)
			return Reference<Resource>(m_resource);

		// If there's no resource loaded, we just load it and establish the connection:
		Reference<Resource> resource = LoadResource();
		if (resource != nullptr) {
			assert(resource->m_asset == nullptr); // Rudimentary defence against misuse...
			{
				std::unique_lock<SpinLock> lock(resource->m_assetLock);
				resource->m_asset = this;
			}
			m_resource = resource;
		}
		return resource;
	}

	Asset::Asset(const GUID& guid) : m_guid(guid) {}
}
