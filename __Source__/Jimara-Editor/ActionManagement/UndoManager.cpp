#include "UndoManager.h"
#include <mutex>


namespace Jimara {
	namespace Editor {
		UndoManager::UndoManager(size_t maxActions) {
			SetMaxActions(maxActions);
		}

		void UndoManager::AddAction(Action* action) {
			if (action == nullptr) return;
			std::unique_lock<SpinLock> lock(m_actionLock);
			m_actionStack.push_back(action);
			while (m_actionStack.size() > m_maxActions)
				m_actionStack.pop_front();
		}

		void UndoManager::Undo() {
			while (true) {
				const Reference<Action> action = [&]() ->Reference<Action> {
					std::unique_lock<SpinLock> lock(m_actionLock);
					if (m_actionStack.empty()) return nullptr;
					Reference<Action> rv = m_actionStack.back();
					m_actionStack.pop_back();
				}();
				if (action == nullptr) break;
				else if (action->Invalidated()) continue;
				else {
					action->Undo();
					break;
				}
			}
		}

		size_t UndoManager::MaxActions()const {
			return m_maxActions;
		}

		void UndoManager::SetMaxActions(size_t maxAction) {
			std::unique_lock<SpinLock> lock(m_actionLock);
			m_maxActions = maxAction;
			while (m_actionStack.size() > m_maxActions)
				m_actionStack.pop_front();
		}
	}
}
