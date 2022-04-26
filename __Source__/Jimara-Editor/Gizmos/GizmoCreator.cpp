#include "GizmoCreator.h"


namespace Jimara {
	namespace Editor {
		GizmoCreator::GizmoCreator(GizmoScene::Context* context) 
			: m_context(context) {
			// __TODO__: Implement this crap!
			std::unique_lock<std::recursive_mutex> lock(m_context->TargetContext()->UpdateLock());
			m_context->GizmoContext()->OnUpdate() += Callback(&GizmoCreator::OnUpdate, this);
			m_context->TargetContext()->OnComponentCreated() += Callback(&GizmoCreator::OnComponentCreated, this);
			m_context->Selection()->OnComponentSelected() += Callback(&GizmoCreator::OnComponentSelected, this);
			m_context->Selection()->OnComponentDeselected() += Callback(&GizmoCreator::OnComponentDeselected, this);
		}

		GizmoCreator::~GizmoCreator() {
			// __TODO__: Implement this crap!
			std::unique_lock<std::recursive_mutex> lock(m_context->TargetContext()->UpdateLock());
			m_context->GizmoContext()->OnUpdate() -= Callback(&GizmoCreator::OnUpdate, this);
			m_context->TargetContext()->OnComponentCreated() -= Callback(&GizmoCreator::OnComponentCreated, this);
			m_context->Selection()->OnComponentSelected() -= Callback(&GizmoCreator::OnComponentSelected, this);
			m_context->Selection()->OnComponentDeselected() -= Callback(&GizmoCreator::OnComponentDeselected, this);
		}

		void GizmoCreator::OnUpdate() {
			// __TODO__: Implement this crap!
		}
		void GizmoCreator::OnComponentCreated(Component* component) {
			// __TODO__: Implement this crap!
		}
		void GizmoCreator::OnComponentDestroyed(Component* component) {
			// __TODO__: Implement this crap!
		}
		void GizmoCreator::OnComponentSelected(Component* component) {
			// __TODO__: Implement this crap!
		}
		void GizmoCreator::OnComponentDeselected(Component* component) {
			// __TODO__: Implement this crap!
		}
	}
}
