#include "AssetDatabase.h"
#include <cassert>



namespace Jimara {
	Asset* Resource::GetAsset()const { return m_asset; }

	const GUID& Resource::Guid()const {
		assert(m_asset != nullptr);
		return m_asset->Guid();
	}

	void Resource::OnOutOfScope()const {
		if (m_asset != nullptr) {
			Reference<Asset> asset(m_asset);
			std::unique_lock<SpinLock> lock(m_asset->m_resourceLock);
			if (RefCount() > 0) return;
			else if (m_asset->m_resource == this) {
				m_asset->m_resource = nullptr;
				m_asset = nullptr;
			}
		}
		Object::OnOutOfScope();
	}


	const GUID& Asset::Guid()const { return m_guid; }

	Reference<Resource> Asset::Load() {
		{
			std::unique_lock<SpinLock> lock(m_resourceLock);
			if (m_resource != nullptr && m_resource->RefCount() > 0)
				return Reference<Resource>(m_resource);
		}
		Reference<Resource> resource = LoadResource();
		if (resource != nullptr) {
			assert(resource->m_asset == nullptr); // Rudimentary defence against misuse...
			std::unique_lock<SpinLock> lock(m_resourceLock);
			resource->m_asset = this;
			m_resource = resource;
		}
		return resource;
	}
}
