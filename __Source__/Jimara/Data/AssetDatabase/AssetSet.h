#pragma once
#include "AssetDatabase.h"

namespace Jimara {
	/// <summary>
	/// A simple asset collection with insert/search and nothing else
	/// </summary>
	class AssetSet final : public virtual AssetDatabase {
	public:
		/// <summary>
		/// Finds an asset within the database
		/// Note: The asset may or may not be loaded once returned;
		/// </summary>
		/// <param name="id"> Asset identifier </param>
		/// <returns> Asset reference, if found </returns>
		inline virtual Reference<Asset> FindAsset(const GUID& id) final override {
			std::unique_lock<SpinLock> lock(m_lock);
			decltype(m_assets)::const_iterator it = m_assets.find(id);
			if (it == m_assets.end()) return nullptr;
			else return it->second;
		}

		/// <summary>
		/// Inserts an asset in the database
		/// Note: Overrides any asset already present in the DB if the GUID repeats
		/// </summary>
		/// <param name="asset"> Asset to insert </param>
		inline void InsertAsset(Asset* asset) {
			if (asset == nullptr) return;
			std::unique_lock<SpinLock> lock(m_lock);
			m_assets[asset->Guid()] = asset;
		}

	private:
		// Collection lock
		SpinLock m_lock;

		// Collection
		std::unordered_map<GUID, Reference<Asset>> m_assets;
	};
}
