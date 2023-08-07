#pragma once
#include "Types.h"
#include <Jimara/Components/Transform.h>


namespace Jimara {
	namespace StateMachine {
		/// <summary> Let the system know that StateMachine component exists </summary>
		JIMARA_REGISTER_TYPE(Jimara::StateMachine::StateMachine);

		class State;
		class Machine;

		/// <summary>
		/// States can activate/deactivate other states (or themselves) using a context object passed to UpdateState() method
		/// </summary>
		struct JIMARA_STATE_MACHINES_API Context {
			/// <summary>
			/// Activates state in the machine's context
			/// <para/> Notes:
			/// <para/>		0. If state was not active before this call, it will be notified via OnStateEnter() at the end of the state machine update;
			/// <para/>		1. If state was already active before this call, it will be notified via OnStateReEnter() at the end of the state machine update;
			/// <para/>		2. If both RemoveState() and AddState() are invoked with the same state as the argument, the last request will be the only one honored;
			/// <para/>		3. State machine can have multiple states active at the same time, but the same state can not be active "twise";
			/// <para/>		4. Keep in mind, that Machine only holds weak references to the states and if one needs to stay alive,
			///				the user/caller should maintain it's strong reference (does not apply to state Components; they are meant to 'die' upon destruction);
			/// <para/>		5. 'Normal' usage for state transition from inside a State would be "context->RemoveState(this); context->AddState(nextState);".
			/// </summary>
			/// <param name="state"> State to activate </param>
			virtual void AddState(State* state) = 0;

			/// <summary>
			/// Deactivates a state in the machine's context
			/// <para/> Notes:
			/// <para/>		0. If state was active, it will be notified via OnStateExit() at the end of the state machine update;
			/// <para/>		1. If state was inactive, no signal will be sent to it;
			/// <para/>		2. If both RemoveState() and AddState() are invoked with the same state as the argument, the last request will be the only one honored;
			/// <para/>		3. 'Normal' usage for state transition from inside a State would be "context->RemoveState(this); context->AddState(nextState);".
			/// </summary>
			/// <param name="state"> State to deactivate </param>
			virtual void RemoveState(State* state) = 0;
		};



		/// <summary>
		/// Abstract state for a state machine
		/// <para/> Notes:
		/// <para/>		0. State Machine is simply a collection of interconnected states; Generally speaking, it will start-off by one state
		///				and execute UpdateState() on each update cycle. State can have any behaviour attached to it and is also responsible for
		///				transitioning to other states if certain requirenments are met.
		/// <para/>		1. Generally speaking, it's recommended to code the states in such a way that only one state is active at a time, 
		///				but the API does not prohibit multiple states from running simultanuously. One example of the latter would be animation 
		///				states still being active during the fade-out period after transition;
		/// <para/>		2. Keep in mind, that State Machine will only hold weak references to the states and their lifecycles should be externally managed.
		///				(This is not an issue with Components, since once destroyed, they should not be used anyway...)
		/// </summary>
		class JIMARA_STATE_MACHINES_API State : public virtual WeaklyReferenceable {
		protected:
			/// <summary>
			/// Invoked on each state machine update, as long as the state stays active
			/// </summary>
			/// <param name="context"> Context for adding/removing active states </param>
			virtual void UpdateState(Context* context) = 0;

			/// <summary> Invoked when the state becomes active </summary>
			virtual void OnStateEnter() = 0;

			/// <summary> Invoked, if Context::AddState got invoked while state was active already </summary>
			virtual void OnStateReEnter() {}
			
			/// <summary> Invoked when the state gets 'deactivated' via Context::RemoveState() call </summary>
			virtual void OnStateExit() = 0;
			
			/// <summary> Machine should be the only class that invokes State callbacks </summary>
			friend class Machine;
		};



#pragma warning(disable: 4250)
		/// <summary> Parent class for a Component, that also happens to be a state </summary>
		class JIMARA_STATE_MACHINES_API StateComponent : public virtual Component, public virtual State {
		protected:
			/// <summary> Constructor </summary>
			inline StateComponent() {}

			/// <summary> Use Component's implementation of WeaklyReferenceable </summary>
			using Component::FillWeakReferenceHolder;
			using Component::ClearWeakReferenceHolder;
		};
#pragma warning(default: 4250)



		/// <summary>
		/// State machine
		/// </summary>
		class JIMARA_STATE_MACHINES_API Machine : public virtual Object {
		public:
			/// <summary> True, if at least one state is currently active </summary>
			bool Active()const;

			/// <summary> Deactivates all states </summary>
			void Stop();

			/// <summary>
			/// Stops and resets state machine with the initial state
			/// </summary>
			/// <param name="initialState"> State to start with </param>
			void Restart(State* initialState);

			/// <summary>
			/// Updates state machine
			/// <para/> Calls UpdateState() on each active state and informs newly activated/deactivated ones.
			/// </summary>
			void Update();

		private:
			// Active state collection
			std::unordered_map<State*, WeakReference<State>> m_activeStates;

			// Temporary buffer for active states, scheduled for execution
			std::vector<Reference<State>> m_executionBuffer;

			// Temporary buffer of states that are supposed to be activated
			std::unordered_set<Reference<State>> m_addedBuffer;

			// Temporary buffer of states that are supposed to be deactivated
			std::unordered_set<Reference<State>> m_removedBuffer;

			// Used for recursion-detection and delay
			std::atomic<uint32_t> m_actionLock = 0u;

			// Caught recursive calls
			typedef void(*ScheduledActionFn)(Machine*, State*);
			using ScheduledAction = std::pair<ScheduledActionFn, WeakReference<State>>;
			using ActionQueue = std::vector<ScheduledAction>;
			ActionQueue m_actionQueue;

			// Private stuff resides in here...
			struct Helpers;
		};



		/// <summary>
		/// State machine instance
		/// </summary>
		class JIMARA_STATE_MACHINES_API StateMachine : public virtual SceneContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="name"> Component name </param>
			StateMachine(Component* parent, const std::string_view& name = "StateMachine");

			/// <summary> Virtual destructor </summary>
			virtual ~StateMachine();

			/// <summary> Initial state the state machine will start-off with </summary>
			Reference<State> InitialState()const;

			/// <summary>
			/// Sets initial state of the state machine
			/// </summary>
			/// <param name="state"> Initial state the state machine will start-off with </param>
			void SetInitialState(State* state);

			/// <summary> True, if at least one state is currently active </summary>
			bool Active()const;

			/// <summary> Deactivates all states </summary>
			void Stop();

			/// <summary> Stops and resets state machine with the initial state </summary>
			void Restart();

			/// <summary>
			/// Exposes fields
			/// </summary>
			/// <param name="recordElement"> Fields will be exposed through this callback </param>
			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

		protected:
			/// <summary> Updates underlying state machine </summary>
			virtual void Update() override;

		private:
			// Initial state the state machine will start-off with
			WeakReference<State> m_initialState;

			// Underlying state machine
			Machine m_machine;

			// If true, the machine will start-over on the next frame, as long as no state is active
			bool m_play = true;

			// A few internal methods substituting InitialState/SetInitialState during serialization
			State* GetInitialState()const;
			void ResetInitialState(State* state);
		};
	}



	// Type details for state machine interfaces and objects
	template<> inline void TypeIdDetails::GetParentTypesOf<StateMachine::State>(const Callback<TypeId>& report) {
		report(TypeId::Of<WeaklyReferenceable>());
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<StateMachine::StateComponent>(const Callback<TypeId>& report) {
		report(TypeId::Of<Component>());
		report(TypeId::Of<StateMachine::State>());
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<StateMachine::Machine>(const Callback<TypeId>& report) {
		report(TypeId::Of<Object>());
	}
	template<> inline void TypeIdDetails::GetParentTypesOf<StateMachine::StateMachine>(const Callback<TypeId>& report) {
		report(TypeId::Of<SceneContext::UpdatingComponent>());
	}
	template<> JIMARA_STATE_MACHINES_API void TypeIdDetails::GetTypeAttributesOf<StateMachine::StateMachine>(const Callback<const Object*>& report);
}
