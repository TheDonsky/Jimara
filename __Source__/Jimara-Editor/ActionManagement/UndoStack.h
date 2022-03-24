#pragma once
#include <Core/Object.h>
#include <Core/Property.h>
#include <Core/Synch/SpinLock.h>
#include <deque>


namespace Jimara {
	namespace Editor {
		class UndoStack : public virtual Object {
		public:
			class Action : public virtual Object {
			public:
				static Reference<Action> Combine(const Reference<Action>* actions, size_t count);

				static Reference<Action> NoOp();

				virtual bool Invalidated()const = 0;

				virtual void Undo() = 0;

				friend class UndoStack;
			};

			UndoStack(size_t maxActions = 1024);

			void AddAction(Action* action);

			void Undo();

			size_t MaxActions()const;

			void SetMaxActions(size_t maxAction);

			inline Property<size_t> MaxActions() {
				size_t(*getFn)(UndoStack*) = [](UndoStack* manager) { return ((const UndoStack*)manager)->MaxActions(); };
				void(*setFn)(UndoStack*, const size_t&) = [](UndoStack* manager, const size_t& maxAction) { manager->SetMaxActions(maxAction); };
				return Property<size_t>(getFn, setFn, this);
			}

		private:
			SpinLock m_actionLock;
			std::atomic<size_t> m_maxActions;
			std::deque<Reference<Action>> m_actionStack;
		};
	}
}
