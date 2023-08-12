#include "StateMachine.h"
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


namespace Jimara {
	namespace StateMachine {
		struct Machine::Helpers {
			inline static void RemoveAllStates(Machine* self) {
				for (auto it = self->m_activeStates.begin(); it != self->m_activeStates.end(); ++it) {
					Reference<State> state = it->second;
					if (it->first == state)
						state->OnStateExit();
				}
				self->m_activeStates.clear();
			}

			inline static bool StartAction(Machine* self) {
				uint32_t expected = 0u;
				return self->m_actionLock.compare_exchange_strong(expected, 1u);
			}

			inline static void EndAction(Machine* self) {
				ActionQueue actions;
				std::swap(actions, self->m_actionQueue);
				self->m_actionLock = 0u;
				for (size_t i = 0u; i < actions.size(); i++) {
					Reference<State> state = actions[i].second;
					actions[i].first(self, state);
				}
			}
		};

		bool Machine::Active()const {
			return m_activeStates.size() > 0u;
		}

		void Machine::Stop() {
			if (Helpers::StartAction(this)) {
				Helpers::RemoveAllStates(this);
				Helpers::EndAction(this);
			}
			else {
				static const ScheduledActionFn action = [](Machine* self, State*) {
					self->Stop();
				};
				m_actionQueue.push_back(ScheduledAction(action, nullptr));
			}
		}

		void Machine::Restart(State* initialState) {
			if (!Helpers::StartAction(this)) {
				static const ScheduledActionFn action = [](Machine* self, State* state) {
					if (state != nullptr)
						self->Restart(state);
				};
				m_actionQueue.push_back(ScheduledAction(action, initialState));
				return;
			}

			Helpers::RemoveAllStates(this);
			if (initialState == nullptr)
				return;
			WeakReference<State>& statePtr = m_activeStates[initialState];
			statePtr = initialState;
			Reference<State> stateRef = statePtr;
			if (stateRef != nullptr)
				stateRef->OnStateEnter();

			Helpers::EndAction(this);
		}

		void Machine::Update() {
			// We do not schedule recursive updates... They can simply be 'swallowed'
			if (!Helpers::StartAction(this))
				return;

			// We need to clear temporary buffers at the begining and end:
			auto clearBuffers = [&]() {
				m_executionBuffer.clear();
				m_addedBuffer.clear();
				m_removedBuffer.clear();
			};
			clearBuffers();

			// Extract the list of states to execute:
			{
				static thread_local std::vector<State*> invalidatedStates;
				invalidatedStates.clear();
				for (auto it = m_activeStates.begin(); it != m_activeStates.end(); ++it) {
					Reference<State> state = it->second;
					if (it->first == state)
						m_executionBuffer.push_back(state);
					else invalidatedStates.push_back(it->first);
				}
				for (size_t i = 0u; i < invalidatedStates.size(); i++)
					m_activeStates.erase(invalidatedStates[i]);
				invalidatedStates.clear();
			}

			// Update states:
			{
				struct Context : public Jimara::StateMachine::Context {
					Machine* const machine;

					inline Context(Machine* m) : machine(m) {
						assert(machine != nullptr);
					}

					inline virtual void AddState(State* state) final override {
						machine->m_removedBuffer.erase(state);
						machine->m_addedBuffer.insert(state);
					}
					inline virtual void RemoveState(State* state) final override {
						machine->m_removedBuffer.insert(state);
						machine->m_addedBuffer.erase(state);
					}
				};
				Context context(this);
				for (size_t i = 0u; i < m_executionBuffer.size(); i++) {
					auto it = m_activeStates.find(m_executionBuffer[i]);
					if (it == m_activeStates.end())
						return;
					Reference<State> state = it->second;
					if (state == it->first)
						state->UpdateState(&context);
				}
			}

			// Remove states:
			for (auto it = m_removedBuffer.begin(); it != m_removedBuffer.end(); ++it) {
				auto ref = m_activeStates.find(*it);
				if (ref == m_activeStates.end())
					continue;
				m_activeStates.erase(ref);
				(*it)->OnStateExit();
			}

			// Add states:
			for (auto it = m_addedBuffer.begin(); it != m_addedBuffer.end(); ++it) {
				WeakReference<State>& statePtr = m_activeStates[*it];
				Reference<State> stateRef = statePtr;
				if (stateRef != nullptr)
					stateRef->OnStateReEnter();
				else {
					statePtr = *it;
					stateRef = statePtr;
					if (stateRef != nullptr)
						stateRef->OnStateEnter();
				}
			}

			// Clear buffers:
			clearBuffers();

			Helpers::EndAction(this);
		}


		StateMachine::StateMachine(Component* parent, const std::string_view& name)
			: Component(parent, name) {}

		StateMachine::~StateMachine() {
			Stop();
		}

		Reference<State> StateMachine::InitialState()const {
			return m_initialState;
		}

		void StateMachine::SetInitialState(State* state) {
			m_initialState = state;
		}
		
		State* StateMachine::GetInitialState()const {
			return InitialState();
		}

		void StateMachine::ResetInitialState(State* state) {
			m_play = Active();
			Stop();
			SetInitialState(state);
		}

		bool StateMachine::Active()const {
			return m_machine.Active() || m_play;
		}

		void StateMachine::Stop() {
			m_machine.Stop();
		}

		void StateMachine::Restart() {
			Stop();
			m_play = true;
		}

		void StateMachine::GetFields(Callback<Jimara::Serialization::SerializedObject> recordElement) {
			Jimara::SceneContext::UpdatingComponent::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				const Reference<State> initialState = m_initialState; // To make sure it does not get destroyed while in use...
				JIMARA_SERIALIZE_FIELD_GET_SET(GetInitialState, ResetInitialState, "Initial State",
					"Initial state of the state machine (changing this from editor will also automatically restart the state machine; "
					"if you set initial state from code, you should restart the machine manually instead)");
				JIMARA_SERIALIZE_FIELD(m_play, "Auto Play", "If true, the state machine will start playing automatically on the next update");
			};
		}

		void StateMachine::Update() {
			bool shouldPlay = m_play;
			m_play = false;
			if ((!m_machine.Active()) && shouldPlay) {
				Reference<State> state = m_initialState;
				m_machine.Restart(state);
			}
			m_machine.Update();
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<StateMachine::StateMachine>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<StateMachine::StateMachine>(
			"State Machine", "Jimara/StateMachine/StateMachine", "State machine, simulated on each frame");
		report(factory);
	}
}
