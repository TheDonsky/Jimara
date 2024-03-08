#include "Handle.h"
#include "../Gizmo.h"
#include "../GizmoViewportHover.h"

namespace Jimara {
	namespace Editor {
		class Handle::HandleSelector : public virtual Gizmo, public virtual Scene::LogicContext::UpdatingComponent {
		private:
			const Reference<GizmoViewportHover> m_hover;
			Reference<Handle> m_hoveredHandle;
			Reference<Handle> m_activeHandle;

			inline static constexpr OS::Input::KeyCode MouseButton() { return OS::Input::KeyCode::MOUSE_LEFT_BUTTON; }
			inline bool MousePressed()const { return Context()->Input()->KeyPressed(MouseButton()); }
			inline bool MouseDown()const { return Context()->Input()->KeyDown(OS::Input::KeyCode::MOUSE_LEFT_BUTTON); }

			inline void UpdateHoverState() {
				if (m_activeHandle != nullptr) return;
				ViewportObjectQuery::Result hover = m_hover->HandleGizmoHover();
				Handle* handle = (hover.component == nullptr) ? nullptr : hover.component->GetComponentInParents<Handle>(true);
				if (m_hoveredHandle == handle) return;
				if (m_hoveredHandle != nullptr) {
					m_hoveredHandle->m_hovered = false;
					m_hoveredHandle->CursorRemoved();
					m_hoveredHandle->m_onCursorExit(m_hoveredHandle);
				}
				m_hoveredHandle = handle;
				if (m_hoveredHandle != nullptr) {
					m_hoveredHandle->m_hovered = true;
					m_hoveredHandle->CursorEntered();
					m_hoveredHandle->m_onCursorEnter(m_hoveredHandle);
				}
			}

			inline void ReleaseHandle() {
				if (m_activeHandle == nullptr) return;
				else if ((!m_activeHandle->Enabled())
					|| m_activeHandle->Destroyed()
					|| (!MousePressed())) {
					m_activeHandle->m_active = false;
					m_activeHandle->HandleDeactivated();
					m_activeHandle->m_onHandleDeactivated(m_activeHandle);
					{
						Component* ptr = m_activeHandle;
						while (ptr != nullptr) {
							Gizmo* gizmo = dynamic_cast<Gizmo*>(ptr);
							if (gizmo != nullptr) gizmo->TrackTargets(false);
							ptr = ptr->Parent();
						}
					}
					m_activeHandle = nullptr;
				}
			}

			inline void SelectHandle() {
				if (m_activeHandle != nullptr || (!MouseDown()) || m_hoveredHandle == nullptr) return;
				m_activeHandle = m_hoveredHandle;
				if (m_activeHandle != nullptr) {
					m_activeHandle->m_active = true;
					m_activeHandle->HandleActivated();
					m_activeHandle->m_onHandleActivated(m_activeHandle);
				}
			}

			inline void UpdateHandle() {
				if (m_activeHandle == nullptr) return;
				m_activeHandle->UpdateHandle();
				m_activeHandle->m_onHandleUpdated(m_activeHandle);
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
				if (m_activeHandle == nullptr) 
					UpdateHoverState();
				ReleaseHandle();
				SelectHandle();
				UpdateHandle();
				if (m_activeHandle == nullptr) 
					UpdateHoverState();
			}
		};

		Handle::Handle() {}

		Handle::~Handle() {}

		bool Handle::HandleActive()const { return m_active; }

		bool Handle::HandleHovered()const { return m_active || m_hovered; }

		Event<Handle*>& Handle::OnCursorEntered()const { return m_onCursorEnter; }

		Event<Handle*>& Handle::OnHandleActivated()const { return m_onHandleActivated; }

		Event<Handle*>& Handle::OnHandleUpdated()const { return m_onHandleUpdated; }

		Event<Handle*>& Handle::OnHandleDeactivated()const { return m_onHandleDeactivated; }

		Event<Handle*>& Handle::OnCursorRemoved()const { return m_onCursorExit; }

		GizmoScene::Context* Handle::GizmoContext()const {
			if (m_context == nullptr)
				m_context = GizmoScene::GetContext(Context());
			return m_context;
		}
	}


	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::Handle>(const Callback<const Object*>& report) {
		static const Reference<const Editor::Gizmo::ComponentConnection> connection =
			Editor::Gizmo::ComponentConnection::Targetless<Editor::Handle::HandleSelector>();
		report(connection);
	}
}
