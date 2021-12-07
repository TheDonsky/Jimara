#include "AssetDatabase.h"
#include <cassert>



namespace Jimara {
	Asset* Resource::GetAsset()const { return m_asset; }

	void Resource::OnOutOfScope()const {
		if (m_asset != nullptr) {
			// Make sure the asset can not go out of scope inside this scope:
			Reference<Asset> asset(m_asset);

			// Lock to prevent overlap with Asset::Load():
			std::unique_lock<std::mutex> lock(m_asset->m_resourceLock);
			
			// If there has been an overlap with Asset::Load(), we should cancel destruction:
			if (RefCount() > 0) return;
			
			// Otherwise, we are free to destroy all connection between the resource and the asset:
			else if (m_asset->m_resource == this) {
				m_asset->m_resource = nullptr;
				m_asset = nullptr;
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
			resource->m_asset = this;
			m_resource = resource;
		}
		return resource;
	}

	Asset::Asset(const GUID& guid) : m_guid(guid) {}
}
