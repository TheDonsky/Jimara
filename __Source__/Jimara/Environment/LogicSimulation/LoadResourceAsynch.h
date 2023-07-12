#pragma once
#include "AsynchronousActionQueue.h"


namespace Jimara {
	/// <summary>
	/// Loads a resource asynchronously
	/// <para/> Notes:
	/// <para/>		0. Requesting component is not allowed to be nullptr;
	/// <para/>		1. If requestingComponent is "dead" by the time the resource is loaded, onLoaded callback will NOT be invoked;
	/// <para/>		2. onLoaded callback will always be invoked through ExecuteAfterUpdate queue, 
	///				even if the resource is loaded to begin with; It's up to the user to handle that optimization manually;
	/// <para/>		3. This function always creates a small temporary allocation; Keep that in mind.
	/// </summary>
	/// <typeparam name="ResourceType"> Type of a resource to load </typeparam>
	/// <typeparam name="OnLoadedCallback"> Any callable that will receive raw resource pointer as argument </typeparam>
	/// <param name="requestingComponent"> Component, requesting the resource to be loaded (can not be nullptr!) </param>
	/// <param name="asset"> Asset to load </param>
	/// <param name="onLoaded"> Callback that'll receive the loaded resource reference (or nullptr if it fails) within ExecuteAfterUpdate queue </param>
	/// <param name="queue"> [Optional] cached reference of AsynchronousActionQueue; if null, it'll be retrieved based on the component's context </param>
	/// <param name="reportProgress"> [Optional] Loading proogress handler </param>
	template<typename ResourceType, typename OnLoadedCallback>
	inline static void LoadResourceAsynch(
		const Component* requestingComponent,
		Asset::Of<ResourceType>* asset,
		const OnLoadedCallback& onLoaded,
		AsynchronousActionQueue* queue = nullptr,
		const Callback<Asset::LoadInfo>& reportProgress = Callback(Unused<Asset::LoadInfo>)) {
		// Nothing should happen if there's no requesting component!
		if (requestingComponent == nullptr)
			return;

		// Make sure we have the queue reference:
		Reference<AsynchronousActionQueue> q = queue;
		if (q == nullptr) {
			q = AsynchronousActionQueue::GetFor(requestingComponent->Context());
			if (q == nullptr) {
				requestingComponent->Context()->Log()->Error(
					"LoadResourceAsynch - Failed to get action queue! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
		}

		// Load state
		struct LoadHandler : public virtual Object {
			// State:
			const Reference<const Component> component;
			const Reference<Asset::Of<ResourceType>> asset;
			const OnLoadedCallback onLoaded;
			const Callback<Asset::LoadInfo> reportProgress;
			Reference<ResourceType> resource;

			// Constructor & Destructor:
			inline LoadHandler(
				const Component* comp, Asset::Of<ResourceType>* ass, 
				const OnLoadedCallback& loaded, const Callback<Asset::LoadInfo>& progress)
				: component(comp), asset(ass), onLoaded(loaded), reportProgress(progress) {}
			inline virtual ~LoadHandler() {}

			// Schedules for the loaded resource to be reported through ExecuteAfterUpdate queue:
			inline void ScheduleResultReport() {
				if (component->Destroyed())
					return;
				static const auto action = [](Object* selfPtr) {
					LoadHandler* const self = dynamic_cast<LoadHandler*>(selfPtr);
					assert(self != nullptr);
					if (self->component->Destroyed())
						return;
					else self->onLoaded(self->resource.operator->());
				};
				component->Context()->ExecuteAfterUpdate(Callback<Object*>::FromCall(&action), this);
			}

			// Schedules asynchronous loading through the AsynchronousActionQueue:
			inline void ScheduleAsynchronousLoad(AsynchronousActionQueue* queue) {
				if (component->Destroyed())
					return;
				static const auto action = [](Object* selfPtr) {
					LoadHandler* const self = dynamic_cast<LoadHandler*>(selfPtr);
					assert(self != nullptr);
					if (self->component->Destroyed())
						return;
					self->resource = self->asset->Load(self->reportProgress);
					self->ScheduleResultReport();
				};
				queue->Schedule(Callback<Object*>::FromCall(&action), this);
			}
		};

		// Creates handler and reports progress:
		const Reference<LoadHandler> handler = Object::Instantiate<LoadHandler>(
			requestingComponent, asset, onLoaded, reportProgress);
		if (asset != nullptr)
			handler->resource = asset->GetLoaded();
		if (asset == nullptr || handler->resource != nullptr)
			handler->ScheduleResultReport();
		else handler->ScheduleAsynchronousLoad(q);
	}





	/// <summary>
	/// Loads a resource asynchronously
	/// <para/> Notes:
	/// <para/>		0. Context is not allowed to be nullptr;
	/// <para/>		1. onLoaded callback will always be invoked through ExecuteAfterUpdate queue, 
	///				even if the resource is loaded to begin with; It's up to the user to handle that optimization manually;
	/// <para/>		2. This function always creates a small temporary allocation; Keep that in mind.
	/// </summary>
	/// <typeparam name="ResourceType"> Type of a resource to load </typeparam>
	/// <typeparam name="OnLoadedCallback"> Any callable that will receive raw resource pointer as argument </typeparam>
	/// <param name="context"> Scene context (can not be nullptr!) </param>
	/// <param name="asset"> Asset to load </param>
	/// <param name="onLoaded"> Callback that'll receive the loaded resource reference (or nullptr if it fails) within ExecuteAfterUpdate queue </param>
	/// <param name="queue"> [Optional] cached reference of AsynchronousActionQueue; if null, it'll be retrieved based on the context </param>
	/// <param name="reportProgress"> [Optional] Loading proogress handler </param>
	template<typename ResourceType, typename OnLoadedCallback>
	inline static void LoadResourceAsynch(
		SceneContext* context,
		Asset::Of<ResourceType>* asset,
		const OnLoadedCallback& onLoaded,
		AsynchronousActionQueue* queue = nullptr,
		const Callback<Asset::LoadInfo>& reportProgress = Callback(Unused<Asset::LoadInfo>)) {
		// Nothing should happen if there's no context!
		if (context == nullptr)
			return;

		// Make sure we have the queue reference:
		Reference<AsynchronousActionQueue> q = queue;
		if (q == nullptr) {
			q = AsynchronousActionQueue::GetFor(context);
			if (q == nullptr) {
				context->Log()->Error(
					"LoadResourceAsynch - Failed to get action queue! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
		}

		// Load state
		struct LoadHandler : public virtual Object {
			// State:
			const Reference<SceneContext> context;
			const Reference<Asset::Of<ResourceType>> asset;
			const OnLoadedCallback onLoaded;
			const Callback<Asset::LoadInfo> reportProgress;
			Reference<ResourceType> resource;

			// Constructor & Destructor:
			inline LoadHandler(
				SceneContext* ctx, Asset::Of<ResourceType>* ass,
				const OnLoadedCallback& loaded, const Callback<Asset::LoadInfo>& progress)
				: context(ctx), asset(ass), onLoaded(loaded), reportProgress(progress) {}
			inline virtual ~LoadHandler() {}

			// Schedules for the loaded resource to be reported through ExecuteAfterUpdate queue:
			inline void ScheduleResultReport() {
				static const auto action = [](Object* selfPtr) {
					LoadHandler* const self = dynamic_cast<LoadHandler*>(selfPtr);
					assert(self != nullptr);
					self->onLoaded(self->resource.operator->());
				};
				context->ExecuteAfterUpdate(Callback<Object*>::FromCall(&action), this);
			}

			// Schedules asynchronous loading through the AsynchronousActionQueue:
			inline void ScheduleAsynchronousLoad(AsynchronousActionQueue* queue) {
				static const auto action = [](Object* selfPtr) {
					LoadHandler* const self = dynamic_cast<LoadHandler*>(selfPtr);
					assert(self != nullptr);
					self->resource = self->asset->Load(self->reportProgress);
					self->ScheduleResultReport();
				};
				queue->Schedule(Callback<Object*>::FromCall(&action), this);
			}
		};

		// Creates handler and reports progress:
		const Reference<LoadHandler> handler = Object::Instantiate<LoadHandler>(
			context, asset, onLoaded, reportProgress);
		if (asset != nullptr)
			handler->resource = asset->GetLoaded();
		if (asset == nullptr || handler->resource != nullptr)
			handler->ScheduleResultReport();
		else handler->ScheduleAsynchronousLoad(q);
	}
}
