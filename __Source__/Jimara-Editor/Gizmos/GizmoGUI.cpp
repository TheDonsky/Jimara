#include "GizmoGUI.h"
#include "../GUI/ImGuiRenderer.h"


namespace Jimara {
	namespace Editor {
		GizmoGUI::GizmoGUI(Scene::LogicContext* gizmoContext) : m_gizmoContext(gizmoContext) {
			std::unique_lock<std::recursive_mutex> lock(m_gizmoContext->UpdateLock());
			m_gizmoContext->OnComponentCreated() += Callback(&GizmoGUI::OnComponentCreated, this);
		}

		GizmoGUI::~GizmoGUI() {
			std::unique_lock<std::recursive_mutex> lock(m_gizmoContext->UpdateLock());
			m_gizmoContext->OnComponentCreated() -= Callback(&GizmoGUI::OnComponentCreated, this);
			for (size_t i = 0; i < m_drawers.Size(); i++)
				m_drawers[i]->OnDestroyed() -= Callback(&GizmoGUI::OnComponentDestroyed, this);
		}

		void GizmoGUI::Draw() {
			std::unique_lock<std::recursive_mutex> lock(m_gizmoContext->UpdateLock());
			m_drawerList.clear();
			for (size_t i = 0; i < m_drawers.Size(); i++)
				m_drawerList.push_back(m_drawers[i]);
			
			// __TODO__: Do some sorting here...

			for (size_t i = 0; i < m_drawerList.size(); i++) {
				Drawer* drawer = m_drawerList[i];
				if (drawer == nullptr) {
					m_gizmoContext->Log()->Warning("GizmoGUI::OnComponentDestroyed - Null drawer in drawer list! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					continue;
				}
				else if (drawer->Destroyed()) {
					m_drawers.Remove(drawer);
					drawer->OnDestroyed() -= Callback(&GizmoGUI::OnComponentDestroyed, this);
					continue;
				}
				else if (drawer->ActiveInHeirarchy())
					drawer->OnDrawGizmoGUI(); // Note: maybe, track enabled/disabled state and check only the active ones
			}
			m_drawerList.clear();
		}

		void GizmoGUI::OnComponentCreated(Component* component) {
			Drawer* drawer = dynamic_cast<Drawer*>(component);
			if (drawer == nullptr) return;
			m_drawers.Add(drawer);
			drawer->OnDestroyed() += Callback(&GizmoGUI::OnComponentDestroyed, this);
		}
		void GizmoGUI::OnComponentDestroyed(Component* component) {
			Drawer* drawer = dynamic_cast<Drawer*>(component);
			if (drawer == nullptr) {
				m_gizmoContext->Log()->Warning("GizmoGUI::OnComponentDestroyed - Notification came in from a non-gizmo component! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return;
			}
			m_drawers.Remove(drawer);
			drawer->OnDestroyed() -= Callback(&GizmoGUI::OnComponentDestroyed, this);
		}
	}
}
