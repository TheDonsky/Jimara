#pragma once
#include "../../Core/Object.h"
#include "../../Core/Synch/SpinLock.h"
#include "../GUID.h"
#include <mutex>

namespace Jimara {
	class Asset;

	/// <summary>
	/// A Resource is any runtime object, loaded from the AssetDatabase 
	/// (could be a texture/mesh/animation/sound/material or whatever)
	/// </summary>
	class Resource : public virtual Object {
	public:
		/// <summary> 
		/// Asset, the resource is loaded from
		/// Note: Resource may not be tied to an asset, existing independently as "Runtime resources"
		/// </summary>
		Asset* GetAsset()const;

		/// <summary> 
		/// Checks if the Resource is tied to an asset of some kind associated with it
		/// Note: Resource may not be tied to an asset, existing independently as "Runtime resources"
		/// </summary>
		inline bool HasAsset()const { return GetAsset() != nullptr; }

	protected:
		/// <summary>
		/// Destroyes the asset connection, once the Resource goes out of scope
		/// Note: this can not be further overriden, mainly for safety
		/// </summary>
		virtual void OnOutOfScope()const final override;

	private:
		// Asset, the resource is loaded from
		mutable Reference<Asset> m_asset;

		// Lock for m_asset
		mutable SpinLock m_assetLock;

		// Asset needs to have access to the fields
		friend class Asset;
	};

	/// <summary>
	/// Arbitrary Asset from the database.
	/// Notes: 
	///		0. This one does not hold the object itself and just provides a way to load the underlying resources;
	///		1. All Assets are expected to exist inside the database (even if not necessary), but it would be unwise to have all of the underlying resources loaded;
	///		2. Depending on the resource and urgency, one should be encuraged to invoke Load() asynchronously wherever possible to avoid stutter.
	/// </summary>
	class Asset : public virtual Object {
	public:
		/// <summary> Unique asset identifier </summary>
		const GUID& Guid()const;

		/// <summary> 
		/// Gets the resource if already loaded.
		/// Note: Use Load()/LoadAs() to actually load the resource; 
		///		this is for the fast access only when you would want to use asynchronous loads if not already present, 
		///		or just to query if the asset is loaded or not.
		/// </summary>
		/// <returns> Underlying asset if loaded </returns>
		Reference<Resource> GetLoaded()const;

		/// <summary>
		/// Gets the resource if already loaded.
		/// Notes: 
		///		0. Use Load()/LoadAs() to actually load the resource;
		///		1. This is for the fast access only when you would want to use asynchronous loads if not already present, or just to query if the asset is loaded or not;
		///		2. Will naturally return nullptr if the loaded asset is not of the correct type 
		/// </summary>
		/// <typeparam name="ObjectType"> Type to cast the resource to </typeparam>
		/// <returns> Underlying resource as ObjectType (if loaded) </returns>
		template<typename ObjectType>
		inline Reference<ObjectType> GetLoadedAs() {
			Reference<Resource> ref = GetLoaded();
			return Reference<ObjectType>(dynamic_cast<ObjectType*>(ref.operator->()));
		}

		/// <summary>
		/// Loads the underlying asset
		/// Note: This will fail if there's something wrong with the asset database, 
		///		the underlying data is invalid and/or corrupted 
		///		or the asset has been deleted and one still holds it's reference.
		/// </summary>
		/// <returns> Underlying asset if Load() does not fail </returns>
		Reference<Resource> Load();

		/// <summary>
		/// Requests Load() of the resource and typecasts it to ObjectType
		/// </summary>
		/// <typeparam name="ObjectType"> Type to cast the resource to </typeparam>
		/// <returns> Underlying resource as ObjectType (if load successful) </returns>
		template<typename ObjectType>
		inline Reference<ObjectType> LoadAs() {
			Reference<Resource> ref = Load();
			return Reference<ObjectType>(dynamic_cast<ObjectType*>(ref.operator->()));
		}

	protected:
		/// <summary>
		/// Should load the underlying resource
		/// Notes: 
		///		0. The resource will have a Reference to the Asset, so to avoid memory leaks, no Asset should hold a Reference to said resource;
		///		1. Invoked under a common lock, so be carefull not to cause some cyclic dependencies with other Assets...
		/// </summary>
		/// <returns> Loaded resource if successful, nullptr otherwise </returns>
		virtual Reference<Resource> LoadResource() = 0;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="guid"> Asset identifier </param>
		Asset(const GUID& guid);

	private:
		// Guid
		GUID m_guid;

		// Lock for loading
		mutable std::mutex m_resourceLock;

		// Loaded resource (not a Reference, to avoid cyclic dependencies)
		Resource* m_resource = nullptr;

		// Resource needs access to the internals
		friend class Resource;
	};

	/// <summary>
	/// AssetDatabase gives the user access to all the assets from the project
	/// Note: Editor project and deployment are expected to use different implementations,
	///		one directly built and dynamically updated on top of the file system and another, 
	///		reading the resources from some custom binary files. 
	///		Custom implementations are allowed, as long as they conform to the interfaces defined here.
	/// </summary>
	class AssetDatabase : public virtual Object {
	public:
		/// <summary>
		/// Finds an asset within the database
		/// Note: The asset may or may not be loaded once returned;
		/// </summary>
		/// <param name="id"> Asset identifier </param>
		/// <returns> Asset reference, if found </returns>
		virtual Reference<Asset> FindAsset(const GUID& id) = 0;
	};
}
