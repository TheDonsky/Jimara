#include "Handle.h"
#include "../Gizmo.h"
#include "../GizmoViewportHover.h"

namespace Jimara {
	namespace Editor {
		class Handle::HandleSelector : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
		private:
			const Reference<GizmoViewportHover> m_hover;
			Reference<Handle> m_activehandle;

			inline void ReleaseHandle() {
				if (m_activehandle == nullptr) return;
				else if ((!m_activehandle->Enabled())
					|| m_activehandle->Destroyed()
					|| (!Context()->Input()->KeyPressed(OS::Input::KeyCode::MOUSE_LEFT_BUTTON))) {
					m_activehandle->m_active = false;
					m_activehandle->HandleDeactivated();
					m_activehandle->m_onHandleDeactivated(m_activehandle);
					{
						Component* ptr = m_activehandle;
						while (ptr != nullptr) {
							Gizmo* gizmo = dynamic_cast<Gizmo*>(ptr);
							if (gizmo != nullptr) gizmo->TrackTargets(false);
							ptr = ptr->Parent();
						}
					}
					m_activehandle = nullptr;
				}
			}

			inline void SelectHandle() {
				if (m_activehandle != nullptr || (!Context()->Input()->KeyDown(OS::Input::KeyCode::MOUSE_LEFT_BUTTON))) return;
				ViewportObjectQuery::Result hover = m_hover->HandleGizmoHover();
				if (hover.component == nullptr) return;
				m_activehandle = hover.component->GetComponentInParents<Handle>(true);
				if (m_activehandle != nullptr) {
					m_activehandle->m_active = true;
					m_activehandle->HandleActivated();
					m_activehandle->m_onHandleActivated(m_activehandle);
				}
			}

			inline void UpdateHandle() {
				if (m_activehandle == nullptr) return;
				m_activehandle->UpdateHandle();
				m_activehandle->m_onHandleUpdated(m_activehandle);
			}

		public:
			inline HandleSelector(Scene::LogicContext* context) 
				: Component(context, "Handle::HandleSelector")
				, m_hover(GizmoViewportHover::GetFor(GizmoScene::GetContext(context)->Viewport())) {
				if (m_hover == nullptr) {
					context->Log()->Error("Handle::HandleSelector - Failed to get or retrieve GizmoViewportHover!");
					Destroy();
				}
			}

			inline virtual ~HandleSelector() {}

		protected:
			inline virtual void Update() override {
				ReleaseHandle();
				SelectHandle();
				UpdateHandle();
			}
		};

		Handle::Handle() {}

		Handle::~Handle() {}

		bool Handle::HandleActive()const { return m_active; }

		Event<Handle*>& Handle::OnHandleActivated()const { return m_onHandleActivated; }

		Event<Handle*>& Handle::OnHandleUpdated()const { return m_onHandleUpdated; }

		Event<Handle*>& Handle::OnHandleDeactivated()const { return m_onHandleDeactivated; }

		GizmoScene::Context* Handle::GizmoContext()const {
			if (m_context == nullptr)
				m_context = GizmoScene::GetContext(Context());
			return m_context;
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::Handle>() {
		Editor::Gizmo::AddConnection(Editor::Gizmo::ComponentConnection::Targetless<Editor::Handle::HandleSelector>());
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::Handle>() {
		Editor::Gizmo::RemoveConnection(Editor::Gizmo::ComponentConnection::Targetless<Editor::Handle::HandleSelector>());
	}
}
