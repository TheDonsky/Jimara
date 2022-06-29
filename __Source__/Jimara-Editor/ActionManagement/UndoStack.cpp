#include "UndoStack.h"
#include <mutex>
#include <unordered_set>


namespace Jimara {
	namespace Editor {
		namespace {
			class CombinedActions : public virtual UndoStack::Action {
			private:
				const std::vector<Reference<Action>> m_actions;

			public:
				inline CombinedActions(const std::unordered_set<Reference<Action>>& actions) : m_actions(actions.begin(), actions.end()) {}

				inline virtual bool Invalidated()const final override {
					for (size_t i = 0; i < m_actions.size(); i++)
						if (!m_actions[i]->Invalidated()) return false;
					return true;
				}

				inline virtual void Undo() final override {
					for (size_t i = 0; i < m_actions.size(); i++) {
						Action* action = m_actions[i];
						if (!action->Invalidated())
							action->Undo();
					}
				}
			};
		}

		Reference<UndoStack::Action> UndoStack::Action::Combine(const Reference<Action>* actions, size_t count) {
			if (actions == nullptr) return nullptr;
			std::unordered_set<Reference<Action>> actionSet;
			for (size_t i = 0; i < count; i++) {
				Action* action = actions[i];
				if (action != nullptr)
					actionSet.insert(action);
			}
			if (actionSet.empty()) return nullptr;
			else if (actionSet.size() == 1) {
				Reference<UndoStack::Action> action = (*actionSet.begin());
				return action;
			}
			else return Object::Instantiate<CombinedActions>(actionSet);
		}

		namespace {
			class UndoNoOpAction : public virtual UndoStack::Action {
			public:
				inline virtual bool Invalidated()const final override { return false; }
				inline virtual void Undo() final override {}
			};
		}

		Reference<UndoStack::Action> UndoStack::Action::NoOp() { return Object::Instantiate<UndoNoOpAction>(); }

		UndoStack::UndoStack(size_t maxActions) {
			SetMaxActions(maxActions);
		}

		void UndoStack::AddAction(Action* action) {
			if (action == nullptr) return;
			while (true) {
				const Reference<Action> action = [&]() ->Reference<Action> {
					std::unique_lock<SpinLock> lock(m_actionLock);
					if (m_actionStack.empty()) return nullptr;
					Reference<Action> rv = m_actionStack.back();
					return rv;
				}();
				if (action == nullptr || (!action->Invalidated())) break;
				else {
					std::unique_lock<SpinLock> lock(m_actionLock);
					if (action == m_actionStack.back()) m_actionStack.pop_back();
					else break;
				}
			}
			std::unique_lock<SpinLock> lock(m_actionLock);
			m_actionStack.push_back(action);
			while (m_actionStack.size() > m_maxActions)
				m_actionStack.pop_front();
		}

		void UndoStack::Undo() {
			while (true) {
				const Reference<Action> action = [&]() ->Reference<Action> {
					std::unique_lock<SpinLock> lock(m_actionLock);
					if (m_actionStack.empty()) return nullptr;
					Reference<Action> rv = m_actionStack.back();
					m_actionStack.pop_back();
					return rv;
				}();
				if (action == nullptr) break;
				else if (action->Invalidated()) continue;
				else {
					action->Undo();
					break;
				}
			}
		}

		size_t UndoStack::MaxActions()const {
			return m_maxActions;
		}

		void UndoStack::SetMaxActions(size_t maxAction) {
			std::unique_lock<SpinLock> lock(m_actionLock);
			m_maxActions = maxAction;
			while (m_actionStack.size() > m_maxActions)
				m_actionStack.pop_front();
		}
	}
}
