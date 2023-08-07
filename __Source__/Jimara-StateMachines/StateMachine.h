#pragma once
#include <Jimara/Components/Transform.h>


namespace Jimara {
	namespace StateMachine {
		JIMARA_REGISTER_TYPE(Jimara::StateMachine::StateMachine);

		class State;
		class Machine;

		struct Context {
			virtual void AddState(State* state) = 0;
			virtual void RemoveState(State* state) = 0;
		};

		class State : public virtual WeaklyReferenceable {
		protected:
			virtual void UpdateState(Context* context) = 0;

			virtual void OnStateEnter() = 0;

			virtual void OnStateReEnter() {}
			
			virtual void OnStateExit() = 0;
			
			friend class Machine;
		};

#pragma warning(disable: 4250)
		class StateComponent : public virtual Component, public virtual State {
		protected:
			inline StateComponent() {}
		};
#pragma warning(default: 4250)

		class Machine : public virtual Object {
		public:
			bool Active()const;

			void Stop();

			void Restart(State* initialState);

			void Update();

		private:
			std::unordered_map<State*, WeakReference<State>> m_activeStates;
			std::vector<Reference<State>> m_executionBuffer;
			std::unordered_set<Reference<State>> m_addedBuffer;
			std::unordered_set<Reference<State>> m_removedBuffer;
			std::atomic<uint32_t> m_actionLock = 0u;
			typedef void(*ScheduledActionFn)(Machine*, State*);
			using ScheduledAction = std::pair<ScheduledActionFn, WeakReference<State>>;
			using ActionQueue = std::vector<ScheduledAction>;
			ActionQueue m_actionQueue;

			struct Helpers;
		};

		class StateMachine : public virtual SceneContext::UpdatingComponent {
		public:
			StateMachine(Component* parent, const std::string_view& name = "StateMachine");

			virtual ~StateMachine();

			Reference<State> InitialState()const;

			void SetInitialState(State* state);

			bool Active()const;

			void Stop();

			void Restart();

			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		protected:
			virtual void Update() override;

		private:
			WeakReference<State> m_initialState;
			Machine m_machine;
			bool m_play = true;

			State* GetInitialState()const;
			void ResetInitialState(State* state);
		};
	}

	template<> inline void TypeIdDetails::GetParentTypesOf<StateMachine::StateComponent>(const Callback<TypeId>& report) {
		report(TypeId::Of<Component>());
		report(TypeId::Of<StateMachine::State>());
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<StateMachine::StateMachine>(const Callback<TypeId>& report) {
		report(TypeId::Of<SceneContext::UpdatingComponent>());
	}
	template<> inline void TypeIdDetails::GetTypeAttributesOf<StateMachine::StateMachine>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<StateMachine::StateMachine> serializer("Jimara/StateMachine/StateMachine", "StateMachine");
		report(&serializer);
	}
}
