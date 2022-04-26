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
			RecreateGizmos();
		}

		GizmoCreator::~GizmoCreator() {
			// __TODO__: Implement this crap!
			std::unique_lock<std::recursive_mutex> lock(m_context->TargetContext()->UpdateLock());
			m_context->GizmoContext()->OnUpdate() -= Callback(&GizmoCreator::OnUpdate, this);
			m_context->TargetContext()->OnComponentCreated() -= Callback(&GizmoCreator::OnComponentCreated, this);
			m_context->Selection()->OnComponentSelected() -= Callback(&GizmoCreator::OnComponentSelected, this);
			m_context->Selection()->OnComponentDeselected() -= Callback(&GizmoCreator::OnComponentDeselected, this);
			ClearGizmos();
		}

		void GizmoCreator::OnUpdate() {
			Reference<const Gizmo::ComponentConnectionSet> connections = Gizmo::ComponentConnectionSet::Current();
			if (connections != m_connections) RecreateGizmos();
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

		void ClearGizmos() {
			// __TODO__: Implement this crap!
		}
		void GizmoCreator::RecreateGizmos() {
			ClearGizmos();
			m_connections = Gizmo::ComponentConnectionSet::Current();
			if (m_connections == nullptr) {
				m_context->GizmoContext()->Log()->
					Fatal("GizmoCreator::RecreateGizmos - Gizmo::ComponentConnectionSet::Current() returned false! [File: ", __FILE__ "; Line: ", __LINE__, "]");
				return;
			}
			// __TODO__: Implement this crap!
		}
	}
}
