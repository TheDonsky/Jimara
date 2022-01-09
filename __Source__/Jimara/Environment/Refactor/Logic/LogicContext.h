#pragma once
#include "../Graphics/GraphicsContext.h"
#include "../Physics/PhysicsContext.h"
#include "../Audio/AudioContext.h"
#include "../../../Core/Collections/DelayedObjectSet.h"
#include "../../../Components/Component.h"
#include <mutex>

namespace Jimara {
#ifndef USE_REFACTORED_SCENE
namespace Refactor_TMP_Namespace {
#endif
	class SceneContext : public virtual Object {
	public:
		inline Scene::Clock* Time()const { return m_time; }

		inline uint64_t FrameIndex()const { return m_frameIndex.load(); }

		inline OS::Logger* Log()const { return m_logger; }

		inline const OS::Input* Input()const { return m_input; }

		inline Scene::GraphicsContext* Graphics()const { return m_graphics; }

		inline Scene::PhysicsContext* Physics()const { return m_physics; }

		inline Scene::AudioContext* Audio()const { return m_audio; }

		inline std::recursive_mutex& UpdateLock()const { return m_updateLock; }

		class UpdatingComponent : public virtual Component {
		protected:
			virtual void Update() = 0;

		private:
			// Only the Logic context is allowed to invoke Update() method
			friend class Scene::LogicContext;
		};

		inline Event<>& OnUpdate() { return m_onUpdate; }

		void StoreDataObject(const Object* object);
		void EraseDataObject(const Object* object);


	private:
		const Reference<Scene::Clock> m_time;
		std::atomic<uint64_t> m_frameIndex = 0;
		const Reference<OS::Logger> m_logger;
		const Reference<OS::Input> m_input;
		const Reference<Scene::GraphicsContext> m_graphics;
		const Reference<Scene::PhysicsContext> m_physics;
		const Reference<Scene::AudioContext> m_audio;
		
		mutable std::recursive_mutex m_updateLock;
		EventInstance<> m_onUpdate;

		void FlushComponentSets();
		void Update(float deltaTime);
		void ComponentCreated(Component* component);
		void ComponentDestroyed(Component* component);
		void ComponentEnabledStateDirty(Component* component);

		inline SceneContext(OS::Logger* logger, OS::Input* input, Scene::GraphicsContext* graphics, Scene::PhysicsContext* physics, Scene::AudioContext* audio)
			: m_time([]() -> Reference<Scene::Clock> { Reference<Scene::Clock> clock = new Scene::Clock(); clock->ReleaseRef(); return clock; }())
			, m_logger(logger), m_input(input), m_graphics(graphics), m_physics(physics), m_audio(audio) {}

		struct Data : public virtual Object {
			inline Data(OS::Logger* logger, OS::Input* input, Scene::GraphicsContext* graphics, Scene::PhysicsContext* physics, Scene::AudioContext* audio)
				: context([&]() -> Reference<Scene::LogicContext> {
				Reference<Scene::LogicContext> instance = new Scene::LogicContext(logger, input, graphics, physics, audio);
				instance->ReleaseRef();
				return instance;
					}()) {
				context->m_data.data = this;
			}

			inline virtual void OnOutOfScope()const final override {
				std::unique_lock<SpinLock> lock(context->m_data.lock);
				if (RefCount() > 0) return;
				else {
					context->m_data.data = nullptr;
					Object::OnOutOfScope();
				}
			}

			void FlushComponentSet();
			void FlushComponentStates();
			void UpdateUpdatingComponents();

			const Reference<Scene::LogicContext> context;
			DelayedObjectSet<Component> allComponents;
			DelayedObjectSet<Component> enabledComponents;
			ObjectSet<UpdatingComponent> updatingComponents;

			std::mutex dataObjectLock;
			ObjectSet<const Object> dataObjects;
		};
		Scene::DataWeakReference<Data> m_data;

		// Scene has to be able to create and update the context
		friend class Scene;

		// Component has to be able to inform the context that it has been created
		friend class Component;
	};
#ifndef USE_REFACTORED_SCENE
}
#endif
}
