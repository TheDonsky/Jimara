#include "LogicContext.h"
#include "../../../OS/Input/NoInput.h"
#include "../../../Data/AssetDatabase/AssetSet.h"

namespace Jimara {
	Reference<Component> SceneContext::RootObject()const {
		Reference<Data> data = m_data;
		if (data == nullptr) return nullptr;
		Reference<Component> rootObject = data->rootObject;
		return rootObject;
	}

	void SceneContext::ExecuteAfterUpdate(const Callback<Object*>& callback, Object* userData) {
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		data->postUpdateActions.Schedule(callback, userData);
	}

	void SceneContext::StoreDataObject(const Object* object) {
		if (object == nullptr) return;
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		std::unique_lock<std::mutex> lock(data->dataObjectLock);
		data->dataObjects.Add(object);
	}
	void SceneContext::EraseDataObject(const Object* object) {
		if (object == nullptr) return;
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		std::unique_lock<std::mutex> lock(data->dataObjectLock);
		data->dataObjects.Remove(object);
	}

	void SceneContext::FlushComponentSets() {
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		data->FlushComponentSet();
		data->FlushComponentStates();
	}

	void SceneContext::FlushQueues() {
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		data->postUpdateActions.Flush();
		FlushComponentSets();
	}

	void SceneContext::Update(float deltaTime) {
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		{
			data->UpdateUpdatingComponents();
			FlushComponentSets();
		}
		{
			m_onUpdate();
			FlushComponentSets();
		}
		{
			FlushQueues();
		}
		// __TODO__: Maybe add in some more steps? (asynchronous update job, for example or some other bullcrap)
	}

	void SceneContext::ComponentCreated(Component* component) {
		if (component == nullptr) return;
		std::unique_lock<std::recursive_mutex> updateLock(m_updateLock);
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		data->allComponents.ScheduleAdd(component);
	}
	void SceneContext::ComponentDestroyed(Component* component) {
		if (component == nullptr) return;
		std::unique_lock<std::recursive_mutex> updateLock(m_updateLock);
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		data->allComponents.ScheduleRemove(component);
	}
	void SceneContext::ComponentEnabledStateDirty(Component* component) {
		if (component == nullptr) return;
		std::unique_lock<std::recursive_mutex> updateLock(m_updateLock);
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		if (data->allComponents.Contains(component)) {
			static thread_local std::vector<Component*> allChildren;
			component->GetComponentsInChildren<Component>(allChildren, true);
			allChildren.push_back(component);
			for (size_t i = 0; i < allChildren.size(); i++) {
				Component* child = allChildren[i];
				if (child->ActiveInHeirarchy())
					data->enabledComponents.ScheduleAdd(child);
				else data->enabledComponents.ScheduleRemove(child);
			}
			allChildren.clear();
		}
	}




	
	namespace {
		class RootComponent : public virtual Component {
		private:
			const Callback<const void*> m_resetRootComponent;

		public:
			inline RootComponent(const Callback<const void*>& resetRootComponent, SceneContext* context)
				: Component(context, "SceneRoot"), m_resetRootComponent(resetRootComponent) {
				OnDestroyed() += Callback<Component*>(&RootComponent::OnDestroyedByUser, this);
			}

			inline virtual void SetParent(Component*) final override {
				Context()->Log()->Fatal("Scene Root Object can not have a parent!");
			}

			inline void OnDestroyedByUser(Component*) {
				OnDestroyed() -= Callback<Component*>(&RootComponent::OnDestroyedByUser, this);
				m_resetRootComponent((const void*)(&m_resetRootComponent));
			}
		};
	}

	void SceneContext::Cleanup() {
		std::unique_lock<std::recursive_mutex> updateLock(m_updateLock);
		FlushComponentSets();
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		for (size_t i = 0; i < data->allComponents.Size(); i++) {
			Component* component = data->allComponents[i];
			if (component != nullptr && component != data->rootObject && (component->Parent() == nullptr || component->Parent() == component))
				component->Destroy();
		}
		FlushComponentSets();
		if (data->rootObject != nullptr) {
			data->rootObject->OnDestroyed() -= Callback<Component*>(&RootComponent::OnDestroyedByUser, dynamic_cast<RootComponent*>(data->rootObject.operator->()));
			data->rootObject->Destroy();
			data->rootObject = nullptr;
		}
		FlushComponentSets();
		data->postUpdateActions.Flush();
		{
			std::unique_lock<std::mutex> lock(data->dataObjectLock);
			data->dataObjects.Clear();
		}
	}

	SceneContext::Data::Data(Scene::LogicContext* ctx) : context(ctx) {
		context->m_data.data = this;
		void (*resetRootComponent)(SceneContext*, const void*) = [](SceneContext* scene, const void* callbackPtr) {
			std::unique_lock<std::recursive_mutex> lock(scene->UpdateLock());
			const Callback<const void*>& resetRootComponent = *((const Callback<const void*>*)callbackPtr);
			Reference<Data> data = scene->m_data;
			if (data != nullptr)
				data->rootObject = Object::Instantiate<RootComponent>(resetRootComponent, scene);
		};
		rootObject = Object::Instantiate<RootComponent>(Callback<const void*>(resetRootComponent, context.operator->()), context);
	}

	Reference<SceneContext::Data> SceneContext::Data::Create(
		Scene::CreateArgs& createArgs,
		Scene::GraphicsContext* graphics, 
		Scene::PhysicsContext* physics, 
		Scene::AudioContext* audio) {
		if (createArgs.logic.input == nullptr) {
			if (createArgs.createMode == Scene::CreateArgs::CreateMode::CREATE_DEFAULT_FIELDS_AND_WARN)
				createArgs.logic.logger->Warning("Scene::LogicContext::Create - Created a mock-input, since no valid input was provided!");
			else if (createArgs.createMode == Scene::CreateArgs::CreateMode::ERROR_ON_MISSING_FIELDS) {
				createArgs.logic.logger->Error("Scene::LogicContext::Create - No valid input was provided!");
				return nullptr;
			}
			createArgs.logic.input = Object::Instantiate<OS::NoInput>();
		}

		if (createArgs.logic.assetDatabase == nullptr) {
			if (createArgs.createMode == Scene::CreateArgs::CreateMode::CREATE_DEFAULT_FIELDS_AND_WARN)
				createArgs.logic.logger->Warning("Scene::LogicContext::Create - Creating a default asset collection, since no valid asset database was provided!");
			else if (createArgs.createMode == Scene::CreateArgs::CreateMode::ERROR_ON_MISSING_FIELDS) {
				createArgs.logic.logger->Error("Scene::LogicContext::Create - No valid asset database was provided!");
				return nullptr;
			}
			createArgs.logic.assetDatabase = Object::Instantiate<AssetSet>();
		}

		Reference<Scene::LogicContext> instance = new Scene::LogicContext(createArgs, graphics, physics, audio);
		instance->ReleaseRef();
		return Object::Instantiate<Data>(instance);
	}

	void SceneContext::Data::OnOutOfScope()const {
		AddRef();
		context->Cleanup();
		{
			std::unique_lock<SpinLock> lock(context->m_data.lock);
			if (RefCount() <= 1)
				context->m_data.data = nullptr;
			else {
				ReleaseRef();
				return;
			}
		}
		{
			std::unique_lock<std::mutex> lock(dataObjectLock);
			dataObjects.Clear();
		}
		Object::OnOutOfScope();
	}

	void SceneContext::Data::FlushComponentSet() {
		auto componentCreated = [&](Component* component) {
			context->ComponentEnabledStateDirty(component);
			component->OnComponentInitialized();
		};
		auto componentDestroyed = [&](Component* component) {
			enabledComponents.ScheduleRemove(component);
		};

		// These buffers make it viable to create/remove components from callbacks:
		static thread_local std::vector<Reference<Component>> addedRefs;
		static thread_local std::vector<Reference<Component>> removedRefs;

		// Flush the component set:
		allComponents.Flush(
			[&](const Reference<Component>* removed, size_t count) {
				for (size_t i = 0; i < count; i++)
					removedRefs.push_back(removed[i]);
			}, [&](const Reference<Component>* added, size_t count) {
				for (size_t i = 0; i < count; i++)
					addedRefs.push_back(added[i]);
			});

		// Invoke relevant callbacks:
		for (size_t i = 0; i < addedRefs.size(); i++)
			componentCreated(addedRefs[i]);
		for (size_t i = 0; i < removedRefs.size(); i++)
			componentDestroyed(removedRefs[i]);
		addedRefs.clear();
		removedRefs.clear();
	}

	void SceneContext::Data::FlushComponentStates() {
		auto componentEnabled = [&](Component* component) {
			{
				Reference<Scene::PhysicsContext::Data> data = context->Physics()->m_data;
				if (data != nullptr) data->ComponentEnabled(component);
			}
			{
				UpdatingComponent* updater = dynamic_cast<UpdatingComponent*>(component);
				if (updater != nullptr) updatingComponents.Add(updater);
			}
			if (component != nullptr) {
				component->OnComponentEnabled();
				if (component->ActiveInHeirarchy())
					if ((component->m_flags.load() & static_cast<uint8_t>(Component::Flags::STARTED)) == 0) {
						component->m_flags |= static_cast<uint8_t>(Component::Flags::STARTED);
						component->OnComponentStart();
					}
			}
		};
		auto componentDisabled = [&](Component* component) {
			{
				Reference<Scene::PhysicsContext::Data> data = context->Physics()->m_data;
				if (data != nullptr) data->ComponentDisabled(component);
			}
			{
				UpdatingComponent* updater = dynamic_cast<UpdatingComponent*>(component);
				if (updater != nullptr) updatingComponents.Remove(updater);
			}
			if (component != nullptr && allComponents.Contains(component) && (!component->Destroyed()))
				component->OnComponentDisabled();
		};
		
		// These buffers make it viable to create/remove components from callbacks:
		static thread_local std::vector<Reference<Component>> addedRefs;
		static thread_local std::vector<Reference<Component>> removedRefs;

		// Flush the enabled component set:
		enabledComponents.Flush(
			[&](const Reference<Component>* removed, size_t count) {
				for (size_t i = 0; i < count; i++)
					removedRefs.push_back(removed[i]);
			}, [&](const Reference<Component>* added, size_t count) {
				for (size_t i = 0; i < count; i++)
					addedRefs.push_back(added[i]);
			});

		// Invoke relevant callbacks:
		for (size_t i = 0; i < addedRefs.size(); i++)
			componentEnabled(addedRefs[i]);
		for (size_t i = 0; i < removedRefs.size(); i++)
			componentDisabled(removedRefs[i]);
		addedRefs.clear();
		removedRefs.clear();
	}

	void SceneContext::Data::UpdateUpdatingComponents() {
		const Reference<UpdatingComponent>* ptr = updatingComponents.Data();
		const Reference<UpdatingComponent>* const end = ptr + updatingComponents.Size();
		while (ptr < end) {
			UpdatingComponent* component = (*ptr);
			if (component->ActiveInHeirarchy())
				component->Update();
			ptr++;
		}
	}
}
