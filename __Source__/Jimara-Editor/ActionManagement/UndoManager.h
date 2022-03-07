#pragma once
#include <Core/Object.h>
#include <Core/Property.h>
#include <Core/Synch/SpinLock.h>
#include <deque>


namespace Jimara {
	namespace Editor {
		class UndoManager : public virtual Object {
		public:
			class Action : public virtual Object {
			private:
				std::atomic<bool> m_invalidated = false;

			protected:
				inline void Invalidate() { m_invalidated = true; }

				inline bool Invalidated()const { return m_invalidated.load(); }

				virtual void Undo() = 0;

				friend class UndoManager;
			};

			UndoManager(size_t maxActions = 512);

			void AddAction(Action* action);

			void Undo();

			size_t MaxActions()const;

			void SetMaxActions(size_t maxAction);

			inline Property<size_t> MaxActions() {
				size_t(*getFn)(UndoManager*) = [](UndoManager* manager) { return ((const UndoManager*)manager)->MaxActions(); };
				void(*setFn)(UndoManager*, const size_t&) = [](UndoManager* manager, const size_t& maxAction) { manager->SetMaxActions(maxAction); };
				return Property<size_t>(getFn, setFn, this);
			}

		private:
			SpinLock m_actionLock;
			std::atomic<size_t> m_maxActions;
			std::deque<Reference<Action>> m_actionStack;
		};
	}
}
