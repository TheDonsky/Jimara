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
			if (asset->m_resource == this) {
				Resource* self = asset->m_resource;
				asset->m_resource = nullptr;
				{
					std::unique_lock<SpinLock> lock(m_assetLock);
					if (m_asset == asset)
						m_asset = nullptr;
				}
				// Let the asset reclaim the resource if it does not have to be deleted:
				asset->UnloadResourceObject(self);
				return;
			}
		}
		Object::OnOutOfScope();
	}


	const GUID& Asset::Guid()const { return m_guid; }

	void Asset::RefreshExternalDependencies() {
		Reference<Resource> resource = GetLoadedResource();
		if (resource != nullptr) RefreshExternalDependencies(resource);
	}

	namespace {
		static thread_local const uint8_t Asset_THREAD_TOKEN = 0;
	}

	Reference<Resource> Asset::GetLoadedResource()const {
		Reference<Resource> resource;
		if (m_loadingThreadToken.load() == (&Asset_THREAD_TOKEN))
			resource = m_resource;
		else {
			std::unique_lock<std::mutex> lock(m_resourceLock);
			resource = m_resource;
		}
		return resource;
	}

	Reference<Resource> Asset::LoadResource(const Callback<LoadInfo>& reportProgress) {
		// Check if recursive LoadResource call is happening and terminate early
		if (m_loadingThreadToken.load() == (&Asset_THREAD_TOKEN)) return nullptr;

		// Only one thread at a time can 'load'
		std::unique_lock<std::mutex> lock(m_resourceLock);

		// If we already have the resource loaded, we can just return it (Note that the resource uses the same lock, making this safe-ish enough):
		if (m_resource != nullptr && m_resource->RefCount() > 0 && m_resource->m_asset == this)
			return Reference<Resource>(m_resource);

		// If there's no resource loaded, we just load it and establish the connection:
		m_reportProgress = &reportProgress;
		m_loadingThreadToken = (&Asset_THREAD_TOKEN);
		Reference<Resource> resource = LoadResourceObject();
		m_loadingThreadToken = nullptr;
		m_reportProgress = nullptr;
		if (resource != nullptr) {
			assert(ResourceType().CheckType(resource)); // Let's make sure we're internally consistent...
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
