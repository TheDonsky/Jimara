#pragma once
#include "ShapeHandles.h"

namespace Jimara {
	namespace Editor {
		class TripleAxisMoveHandle : public virtual Transform, Scene::LogicContext::UpdatingComponent {
		public:
			inline TripleAxisMoveHandle(Component* parent, const std::string_view& name, float size = 1.0f) 
				: Component(parent, name), Transform(parent, name)
				, m_center(FreeMoveSphereHandle(this, "Center"))
				, m_xHandle(FixedAxisArrowHandle(this, "HandleX"))
				, m_yHandle(FixedAxisArrowHandle(this, "HandleY"))
				, m_zHandle(FixedAxisArrowHandle(this, "HandleZ"))
				, m_size(size) {
				m_xHandle->LookTowardsLocal(Vector3(1.0f, 0.0f, 0.0f));
				m_yHandle->LookTowardsLocal(Vector3(0.0f, 1.0f, 0.0f));
				m_zHandle->LookTowardsLocal(Vector3(0.0f, 0.0f, 1.0f));
				UpdateScale();
			}

			inline ~TripleAxisMoveHandle() {}

			inline bool HandleActive()const {
				return 
					m_center->HandleActive() || 
					m_xHandle->HandleActive() || 
					m_yHandle->HandleActive() || 
					m_zHandle->HandleActive();
			}

			inline Vector3 Delta()const {
				return 
					m_center->Delta() +
					m_xHandle->Delta() + 
					m_yHandle->Delta() + 
					m_zHandle->Delta();
			}

		protected:
			inline virtual void Update()override { UpdateScale(); }

		private:
			const Reference<FreeMoveHandle> m_center;
			const Reference<FixedAxisMoveHandle> m_xHandle;
			const Reference<FixedAxisMoveHandle> m_yHandle;
			const Reference<FixedAxisMoveHandle> m_zHandle;
			const float m_size;

			inline void UpdateScale() {
				SetLocalScale(Vector3(m_size * m_center->GizmoContext()->Viewport()->GizmoSizeAt(WorldPosition())));
			}
		};
	}
}