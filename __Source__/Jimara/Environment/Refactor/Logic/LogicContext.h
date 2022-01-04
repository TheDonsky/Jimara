#pragma once
#include "../Graphics/GraphicsContext.h"
#include "../Physics/PhysicsContext.h"
#include "../Audio/AudioContext.h"
#include "../../../Core/Collections/DelayedObjectSet.h"
#include "../../../Components/Component.h"
#include <mutex>

namespace Jimara {
namespace Refactor_TMP_Namespace {
	class Scene::LogicContext : public virtual Object {
	public:
		inline Clock* Time()const { return m_time; }

		inline uint64_t FrameIndex()const { return m_frameIndex.load(); }

		inline OS::Logger* Log()const { return m_logger; }

		inline OS::Input* Input()const { return m_input; }

		inline GraphicsContext* Graphics()const { return m_graphics; }

		inline PhysicsContext* Physics()const { return m_physics; }

		inline AudioContext* Audio()const { return m_audio; }

		inline std::recursive_mutex& UpdateLock()const { return m_updateLock; }

		class UpdatingComponent : public virtual Component {
		protected:
			virtual void Update() = 0;

		private:
			// Only the Logic context is allowed to invoke Update() method
			friend class LogicContext;
		};

		inline Event<>& OnUpdate() { return m_onUpdate; }

		void StoreDataObject(const Object* object);
		void EraseDataObject(const Object* object);


	private:
		const Reference<Clock> m_time;
		std::atomic<uint64_t> m_frameIndex = 0;
		const Reference<OS::Logger> m_logger;
		const Reference<OS::Input> m_input;
		const Reference<GraphicsContext> m_graphics;
		const Reference<PhysicsContext> m_physics;
		const Reference<AudioContext> m_audio;
		
		mutable std::recursive_mutex m_updateLock;
		EventInstance<> m_onUpdate;

		void FlushComponentSets();
		void Update(float deltaTime);
		void ComponentCreated(Component* component);
		void ComponentDestroyed(Component* component);
		void ComponentEnabledStateDirty(Component* component);

		inline LogicContext(OS::Logger* logger, OS::Input* input, GraphicsContext* graphics, PhysicsContext* physics, AudioContext* audio)
			: m_time([]() -> Reference<Clock> { Reference<Clock> clock = new Clock(); clock->ReleaseRef(); return clock; }())
			, m_logger(logger), m_input(input), m_graphics(graphics), m_physics(physics), m_audio(audio) {}

		struct Data : public virtual Object {
			inline Data(OS::Logger* logger, OS::Input* input, GraphicsContext* graphics, PhysicsContext* physics, AudioContext* audio)
				: context([&]() -> Reference<LogicContext> {
				Reference<LogicContext> instance = new LogicContext(logger, input, graphics, physics, audio);
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

			const Reference<LogicContext> context;
			DelayedObjectSet<Component> allComponents;
			DelayedObjectSet<Component> enabledComponents;
			ObjectSet<UpdatingComponent> updatingComponents;

			std::mutex dataObjectLock;
			ObjectSet<const Object> dataObjects;
		};
		DataWeakReference<Data> m_data;

		// Scene has to be able to create and update the context
		friend class Scene;

		// Component has to be able to inform the context that it has been created
		friend class Component;
	};

	typedef Scene::LogicContext SceneContext;
}

template<> inline void TypeIdDetails::GetParentTypesOf<Refactor_TMP_Namespace::Scene::LogicContext>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }
}
