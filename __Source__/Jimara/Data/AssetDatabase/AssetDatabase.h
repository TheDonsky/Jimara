#pragma once
#include "../../Core/Object.h"
#include "../../Core/Synch/SpinLock.h"
#include "../../Core/Helpers.h"
#include "../GUID.h"
#include <mutex>

namespace Jimara {
	class Asset;

	/// <summary>
	/// A Resource is any runtime object, loaded from the AssetDatabase 
	/// (could be a texture/mesh/animation/sound/material or whatever)
	/// </summary>
	class JIMARA_API Resource : public virtual Object {
	public:
		/// <summary> 
		/// Asset, the resource is loaded from
		/// <para /> Note: Resource may not be tied to an asset, existing independently as "Runtime resources"
		/// </summary>
		Asset* GetAsset()const;

		/// <summary> 
		/// Checks if the Resource is tied to an asset of some kind associated with it
		/// <para /> Note: Resource may not be tied to an asset, existing independently as "Runtime resources"
		/// </summary>
		inline bool HasAsset()const { return GetAsset() != nullptr; }

		/// <summary>
		/// Checks if a resource has given external dependency
		/// <para /> Notes: 
		///		<para /> 0. Returns false by default; (this == dependency) does not count;
		///		<para /> 1. "External Dependency" generally means another resource that is always loaded, 
		///		as long as this one exists and is from a different 'loading unit';
		///		<para /> 2. Since specialization is completely optional, it is not required for the Resources to be 'Honest' about it,
		///		but the likes of a Scene File and any other types should implement and use this to avoid circular dependencies.
		/// </summary>
		/// <param name="dependency"> Resource to check </param>
		/// <returns> True, if this resource uses the other as an external dependency </returns>
		inline virtual bool HasExternalDependency(const Resource* dependency)const { Unused(dependency); return false; }

	protected:
		/// <summary>
		/// Destroyes the asset connection, once the Resource goes out of scope
		/// <para /> Note: this can not be further overriden, mainly for safety
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

	// Prent types of Resource
	template<> inline void TypeIdDetails::GetParentTypesOf<Resource>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }

	/// <summary>
	/// Arbitrary Asset from the database.
	/// <para /> Notes: 
	///		<para /> 0. This one does not hold the object itself and just provides a way to load the underlying resources;
	///		<para /> 1. All Assets are expected to exist inside the database (even if not necessary), but it would be unwise to have all of the underlying resources loaded;
	///		<para /> 2. Depending on the resource and urgency, one should be encuraged to invoke Load() asynchronously wherever possible to avoid stutter.
	/// </summary>
	class JIMARA_API Asset : public virtual Object {
	public:
		/// <summary> Unique asset identifier </summary>
		const GUID& Guid()const;

		/// <summary> True, if the resource, once loaded, can have any recursive external dependencies </summary>
		inline virtual bool HasRecursiveDependencies()const { return false; }

		/// <summary>
		/// Refreshes/reloads all 'external' dependencies, thus making sure the Resource is up to date with the Asset Database.
		/// </summary>
		void RefreshExternalDependencies();

		/// <summary> 
		/// Gets the resource if already loaded.
		/// <para /> Note: Use Load()/LoadAs() to actually load the resource; 
		///		this is for the fast access only when you would want to use asynchronous loads if not already present, 
		///		or just to query if the asset is loaded or not.
		/// </summary>
		/// <returns> Underlying asset if loaded </returns>
		Reference<Resource> GetLoadedResource()const;

		/// <summary>
		/// Gets the resource if already loaded.
		/// <para /> Notes: 
		///		<para /> 0. Use Load()/LoadAs() to actually load the resource;
		///		<para /> 1. This is for the fast access only when you would want to use asynchronous loads if not already present, or just to query if the asset is loaded or not;
		///		<para /> 2. Will naturally return nullptr if the loaded asset is not of the correct type 
		/// </summary>
		/// <typeparam name="ObjectType"> Type to cast the resource to </typeparam>
		/// <returns> Underlying resource as ObjectType (if loaded) </returns>
		template<typename ObjectType>
		inline Reference<ObjectType> GetLoadedAs()const {
			Reference<Resource> ref = GetLoadedResource();
			return Reference<ObjectType>(dynamic_cast<ObjectType*>(ref.operator->()));
		}

		/// <summary> Information about loading progress </summary>
		struct LoadInfo {
			/// <summary> Number of subresources to load or substeps to take </summary>
			size_t totalSteps = 0;

			/// <summary> Number of subresources already loaded or substeps taken </summary>
			size_t stepsTaken = 0;

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="total"> totalSteps </param>
			/// <param name="taken"> stepsTaken </param>
			inline LoadInfo(size_t total = 0, size_t taken = 0) : totalSteps(total), stepsTaken(taken) {}

			/// <summary> stepsTaken / totalSteps, but bound to [0.0f; 1.0f] range </summary>
			inline float Fraction()const { 
				if (totalSteps <= 0) return (stepsTaken > 0) ? 1.0f : 0.0f;
				else {
					float rawFraction = (static_cast<float>(stepsTaken) / static_cast<float>(totalSteps));
					return (rawFraction > 1.0f) ? 1.0f : rawFraction;
				}
			}
		};

		/// <summary>
		/// Loads the underlying asset
		/// Note: This will fail if there's something wrong with the asset database, 
		///		the underlying data is invalid and/or corrupted 
		///		or the asset has been deleted and one still holds it's reference.
		/// </summary>
		/// <param name="reportProgress"> 
		///		Callback, through which the asset can report loading progress 
		///		(Not all assets will report progress; support is optional, but one would expect the slow-loading ones to use this) 
		/// </param>
		/// <returns> Underlying asset if LoadResource() does not fail </returns>
		Reference<Resource> LoadResource(const Callback<LoadInfo>& reportProgress = Callback(Unused<LoadInfo>));

		/// <summary>
		/// Requests Load() of the resource and typecasts it to ObjectType
		/// </summary>
		/// <typeparam name="ObjectType"> Type to cast the resource to </typeparam>
		/// <param name="reportProgress"> 
		///		Callback, through which the asset can report loading progress 
		///		(Not all assets will report progress; support is optional, but one would expect the slow-loading ones to use this) 
		/// </param>
		/// <returns> Underlying resource as ObjectType (if load successful) </returns>
		template<typename ObjectType>
		inline Reference<ObjectType> LoadAs(const Callback<LoadInfo>& reportProgress = Callback(Unused<LoadInfo>)) {
			Reference<Resource> ref = LoadResource(reportProgress);
			return Reference<ObjectType>(dynamic_cast<ObjectType*>(ref.operator->()));
		}

		/// <summary> Type of the resource, this asset can load </summary>
		virtual TypeId ResourceType()const = 0;

		/// <summary>
		/// Asset, that loads a specific resource type
		/// </summary>
		/// <typeparam name="ResourceType"> Resource type, the asset loads </typeparam>
		template<typename ResourceType>
		class Of;

	protected:
		/// <summary>
		/// Should load the underlying resource
		/// <para /> Notes: 
		///		<para /> 0. The resource will have a Reference to the Asset, so to avoid memory leaks, no Asset should hold a Reference to said resource;
		///		<para /> 1. Invoked under a common lock, so be carefull not to cause some cyclic dependencies with other Assets...
		/// </summary>
		/// <returns> Loaded resource if successful, nullptr otherwise </returns>
		virtual Reference<Resource> LoadResourceObject() = 0;

		/// <summary>
		/// Should "release" the resource previously loaded using LoadResourceObject()
		/// <para /> Notes:
		///		<para /> 0. Invoked when the resource goes out of scope;
		///		<para /> 1. When this function receives the reference, the resource will have it's Asset dependency erased, but you can be confident, 
		///		that it's the same resource, previously returned by LoadResourceObject();
		///		1. Invoked under a common lock, so be carefull not to cause some cyclic dependencies with other Assets...
		/// </summary>
		/// <param name="resource"> Resource to release </param>
		inline virtual void UnloadResourceObject(Reference<Resource> resource) = 0;

		/// <summary>
		/// Should refresh/reload all 'external' dependencies, thus making sure the Resource is up to date with the Asset Database.
		/// </summary>
		/// <param name="resource"> Resource, previously loaded with LoadResourceObject() </param>
		inline virtual void RefreshExternalDependencies(Resource* resource) { Unused(resource); }

		/// <summary>
		/// Reports loading progress to the 'Load' action listener
		/// <para /> Note: This is only valid inside the Resource loading logic.
		/// </summary>
		/// <param name="info"> Info to report </param>
		inline void ReportProgress(const LoadInfo& info) { if (m_reportProgress != nullptr) (*m_reportProgress)(info); }

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

		// Temporary storage for progress report callbacks
		const Callback<LoadInfo>* m_reportProgress = nullptr;

		// When load starts, this is set to some thread-local address, so that recursion is caught and early-terminated
		std::atomic<const void*> m_loadingThreadToken = nullptr;

		// Loaded resource (not a Reference, to avoid cyclic dependencies)
		Resource* m_resource = nullptr;

		// Resource needs access to the internals
		friend class Resource;
	};

	/// <summary>
	/// Asset, that loads a specific resource type
	/// </summary>
	/// <typeparam name="Type"> Resource type, the asset loads </typeparam>
	template<typename Type>
	class Asset::Of : public virtual Asset {
	public:
		/// <summary> 
		/// Gets the resource if already loaded.
		/// <para /> Note: Use Load()/LoadAs() to actually load the resource; 
		///		this is for the fast access only when you would want to use asynchronous loads if not already present, 
		///		or just to query if the asset is loaded or not.
		/// </summary>
		/// <returns> Underlying asset if loaded </returns>
		inline Reference<Type> GetLoaded()const { return GetLoadedAs<Type>(); }

		/// <summary>
		/// Loads the underlying asset
		/// <para /> Note: This will fail if there's something wrong with the asset database, 
		///		the underlying data is invalid and/or corrupted 
		///		or the asset has been deleted and one still holds it's reference.
		/// </summary>
		/// <param name="reportProgress"> 
		///		Callback, through which the asset can report loading progress 
		///		(Not all assets will report progress; support is optional, but one would expect the slow-loading ones to use this) 
		/// </param>
		/// <returns> Underlying asset if Load() does not fail </returns>
		inline Reference<Type> Load(const Callback<LoadInfo>& reportProgress = Callback(Unused<LoadInfo>)) { return LoadAs<Type>(reportProgress); }

		/// <summary> Type of the resource, this asset can load </summary>
		inline virtual TypeId ResourceType()const final override { return TypeId::Of<Type>(); }

	protected:
		/// <summary>
		/// Should load the underlying resource
		/// <para /> Notes: 
		///		<para /> 0. The resource will have a Reference to the Asset, so to avoid memory leaks, no Asset should hold a Reference to said resource;
		///		<para /> 1. Invoked under a common lock, so be carefull not to cause some cyclic dependencies with other Assets...
		/// </summary>
		/// <returns> Loaded resource if successful, nullptr otherwise </returns>
		virtual Reference<Type> LoadItem() = 0;

		/// <summary>
		/// Should "release" the resource previously loaded using LoadItem()
		/// <para /> Notes:
		///		<para /> 0. Invoked when the resource goes out of scope;
		///		<para /> 1. When this function receives the reference, the resource will have it's Asset dependency erased, but you can be confident, 
		///		that it's the same resource, previously returned by LoadItem();
		///		<para /> 2. Invoked under a common lock, so be carefull not to cause some cyclic dependencies with other Assets...
		/// </summary>
		/// <param name="resource"> Resource to release </param>
		inline virtual void UnloadItem(Type* resource) { Unused(resource);  }

		/// <summary>
		/// Should refresh/reload all 'external' dependencies, thus making sure the Resource is up to date with the Asset Database.
		/// </summary>
		/// <param name="resource"> Resource, previously loaded with LoadItem() </param>
		inline virtual void ReloadExternalDependencies(Type* resource) { Unused(resource); }

		/// <summary>
		/// Invokes LoadItem()
		/// </summary>
		/// <returns> Loaded resource if successful, nullptr otherwise </returns>
		inline virtual Reference<Resource> LoadResourceObject() final override { return LoadItem(); }

		/// <summary>
		/// Invokes UnloadItem()
		/// </summary>
		/// <param name="resource"> Resource to release </param>
		inline virtual void UnloadResourceObject(Reference<Resource> resource) final override { UnloadItem(dynamic_cast<Type*>(resource.operator->())); }

		/// <summary>
		/// Invokes ReloadExternalDependencies()
		/// </summary>
		/// <param name="resource"> Resource, previously loaded with LoadResourceObject() </param>
		inline virtual void RefreshExternalDependencies(Resource* resource) final override { ReloadExternalDependencies(dynamic_cast<Type*>(resource)); }
	};

	// Prent types of Asset
	template<> inline void TypeIdDetails::GetParentTypesOf<Asset>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }

	// TypeIdDetails::TypeDetails for Asset::Of
	template<typename Type>
	struct TypeIdDetails::TypeDetails<Asset::Of<Type>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) { reportParentType(TypeId::Of<Asset>()); }
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
	};

	/// <summary>
	/// Asset, that can be modified and stored on the fly
	/// <para /> Note: Some assets will inherit this as a part of the FileSystemDatabase for the editor,
	///		However, one should not expect any of the "deployment" database assets to have this functionality,
	///		with an exception of some custom game-specific types defined by user and/or some savefile data 
	///		that may or may not be implemented inside the AssetDB
	/// </summary>
	class JIMARA_API ModifiableAsset : public virtual Asset {
	public:
		/// <summary>
		/// Stores modified resource
		/// <para /> Notes:
		///		<para /> 0. Action will be taken if and only if there exists a loaded resource;
		///		<para /> 1. Internal logic simply invokes ModifiableAsset::Of::Store with GetLoaded() as argument, if it's not nullptr;
		///		<para /> 2. For file system assets (in Editor), this is expected to update some files on disk, possibly triggering 
		///		the entire chain of update and reimport events, leading up to invalidating the Asset instance; 
		///		Depending on implementation, Asset loader may keep the old Asset instance alive, which is advisable, 
		///		but in theory, nothing prevents the Asset from being invalidated and recreated; In that case, 
		///		please listen to FileSystemDB updates and refresh asset references once the corresponding GUID is reported to be dirty.
		/// </summary>
		virtual void StoreResource() = 0;

		/// <summary>
		/// ModifiableAsset, that loads a resource of a given type
		/// </summary>
		/// <typeparam name="ResourceType"> Resource type, the asset loads </typeparam>
		template<typename ResourceType>
		class Of;
	};

#pragma warning(disable: 4250)
	/// <summary>
	/// ModifiableAsset, that loads a resource of a given type
	/// </summary>
	/// <typeparam name="Type"> Resource type, the asset loads </typeparam>
	template<typename Type>
	class ModifiableAsset::Of : public virtual Asset::Of<Type>, public virtual ModifiableAsset {
	protected:
		/// <summary>
		/// Stores resourse
		/// <para /> Notes:
		///		<para /> 0. This will be invoked by StoreResource(), and resource will be the same as GetLoaded();
		///		<para /> 1. Expected behaviour is to save the resource to disk or, in theory, RAM as well, for further reloads;
		///		<para /> 2. If saved to disc and the resource is from a FileSystemDB, after calling this the Asset and Resource will be requested to be invalidated and reloaded.
		///		In this case, the implementation of the asset importer has a freedom to keep or discard the asset as it pleases, but keeping it is somewhat safer;
		///		<para /> 3. In case this is not a file system asset, the implementation is free to do whatever...
		/// </summary>
		/// <param name="resource"> Resource to save </param>
		virtual void Store(Type* resource) = 0;

	public:
		/// <summary> Constructor </summary>
		inline Of() : ModifiableAsset() {}

		/// <summary> Stores modified resource </summary>
		inline virtual void StoreResource() final override {
			Reference<Type> resource = GetLoadedAs<Type>();
			if (resource != nullptr) Store(resource);
		}
	};
#pragma warning(default: 4250)

	// Prent types of ModifiableAsset
	template<> inline void TypeIdDetails::GetParentTypesOf<ModifiableAsset>(const Callback<TypeId>& report) { report(TypeId::Of<Asset>()); }

	// TypeIdDetails::TypeDetails for ModifiableAsset::Of
	template<typename Type>
	struct TypeIdDetails::TypeDetails<ModifiableAsset::Of<Type>> {
		inline static void GetParentTypes(const Callback<TypeId>& reportParentType) {
			reportParentType(TypeId::Of<Asset::Of<Type>>());
			reportParentType(TypeId::Of<ModifiableAsset>());
		}
		inline static void GetTypeAttributes(const Callback<const Object*>&) {}
		inline static void OnRegisterType() {}
		inline static void OnUnregisterType() {}
	};

	/// <summary>
	/// AssetDatabase gives the user access to all the assets from the project
	/// <para /> Note: Editor project and deployment are expected to use different implementations,
	///		one directly built and dynamically updated on top of the file system and another, 
	///		reading the resources from some custom binary files. 
	///		Custom implementations are allowed, as long as they conform to the interfaces defined here.
	/// </summary>
	class JIMARA_API AssetDatabase : public virtual Object {
	public:
		/// <summary>
		/// Finds an asset within the database
		/// <para /> Note: The asset may or may not be loaded once returned;
		/// </summary>
		/// <param name="id"> Asset identifier </param>
		/// <returns> Asset reference, if found </returns>
		virtual Reference<Asset> FindAsset(const GUID& id) = 0;
	};

	// Prent types of AssetDatabase
	template<> inline void TypeIdDetails::GetParentTypesOf<AssetDatabase>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }
}
