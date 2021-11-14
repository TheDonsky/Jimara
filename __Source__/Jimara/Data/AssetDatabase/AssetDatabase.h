#pragma once
#include "../../Core/Object.h"
#include "../../Core/Synch/SpinLock.h"
#include "../GUID.h"
#include <mutex>

namespace Jimara {
	class Asset;

	class Resource : public virtual Object {
	public:
		Asset* GetAsset()const;

		const GUID& Guid()const;

	protected:
		virtual void OnOutOfScope()const final override;

	private:
		Reference<Asset> m_asset;

		friend class Asset;
	};

	class Asset : public virtual Object {
	public:
		const GUID& Guid()const;

		Reference<Resource> Load();

		template<typename ObjectType>
		inline Reference<ObjectType> LoadAs() {
			Reference<Resource> ref = Load();
			return Reference<ObjectType>(dynamic_cast<ObjectType*>(ref.operator->()));
		}

	protected:
		virtual Reference<Resource> LoadResource() = 0;

	private:
		GUID m_guid;
		SpinLock m_resourceLock;
		Resource* m_resource = nullptr;

		friend class Resource;
	};

	class AssetDatabase : public virtual Object {
	public:
		virtual Reference<Asset> FindAsset(const GUID& id) = 0;
	};
}
