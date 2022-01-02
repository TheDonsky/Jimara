#pragma once
#include "../Graphics/GraphicsContext.h"
#include "../Physics/PhysicsContext.h"
#include "../Audio/AudioContext.h"
#include "../../../Core/Systems/Event.h"
#include "../../../Core/Systems/JobSystem.h"
#include "../../../Core/Collections/DelayedObjectSet.h"
#include "../../../Components/Component.h"
#include <mutex>


namespace Jimara {
namespace Refactor_TMP_Namespace {
	class Scene::LogicContext : public virtual Object {
	public:
		inline Clock* Time()const { return m_time; }

		inline OS::Input* Input()const { return m_input; }

		inline GraphicsContext* Graphics()const { return m_graphics; }

		inline PhysicsContext* Physics()const { return m_physics; }

		inline AudioContext* Audio()const { return m_audio; }

		inline std::recursive_mutex& UpdateLock()const { return m_updateLock; }

		class UpdatingComponent : public virtual Component {
		protected:
			virtual void Update() = 0;
		};

		inline Event<>& OnUpdate() { return m_onUpdate; }



	private:
		const Reference<Clock> m_time;
		const Reference<OS::Input> m_input;
		const Reference<GraphicsContext> m_graphics;
		const Reference<PhysicsContext> m_physics;
		const Reference<AudioContext> m_audio;
		
		mutable std::recursive_mutex m_updateLock;
		EventInstance<> m_onUpdate;

		void Update(float deltaTime);

		inline LogicContext(OS::Input* input, GraphicsContext* graphics, PhysicsContext* physics, AudioContext* audio)
			: m_time([]() -> Reference<Clock> { Reference<Clock> clock = new Clock(); clock->ReleaseRef(); return clock; }())
			, m_input(input), m_graphics(graphics), m_physics(physics), m_audio(audio) {}

		struct Data : public virtual Object {
			inline Data(OS::Input* input, GraphicsContext* graphics, PhysicsContext* physics, AudioContext* audio) 
				: context([&]() -> Reference<LogicContext> {
				Reference<LogicContext> instance = new LogicContext(input, graphics, physics, audio);
				instance->ReleaseRef();
				return instance;
					}()) {
				context->m_data = this;
			}

			inline virtual ~Data() { 
				context->m_data = nullptr;
			}

			const Reference<LogicContext> context;
			DelayedObjectSet<Component> allComponents;
			DelayedObjectSet<Component> enabledComponents;
			DelayedObjectSet<Component> disabledComponents;
			DelayedObjectSet<UpdatingComponent> enabledUpdatingComponents;
		};
		DataWeakReference<Data> m_data;

		friend class Scene;
	};

	typedef Scene::LogicContext SceneContext;
}

template<> inline void TypeIdDetails::GetParentTypesOf<Refactor_TMP_Namespace::Scene::LogicContext>(const Callback<TypeId>& report) { report(TypeId::Of<Object>()); }
}

