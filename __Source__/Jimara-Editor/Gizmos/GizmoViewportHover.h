#pragma once
#include "GizmoScene.h"
#include <Environment/Rendering/LightingModels/ObjectIdRenderer/ViewportObjectQuery.h>

namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Queries both Target scene and Gizmo scene of a GizmoViewport image at mouse position
		/// </summary>
		class GizmoViewportHover : public virtual ObjectCache<Reference<Object>>::StoredObject {
		public:
			/// <summary>
			/// Creates or retrives cahced hover query for given viewport
			/// </summary>
			/// <param name="viewport"> GizmoViewport of interest </param>
			/// <returns> Cached hover query for viewport (nullptr if something goes wrong, or provided parameter is nullptr) </returns>
			static Reference<GizmoViewportHover> GetFor(GizmoViewport* viewport);

			/// <summary> Virtual destructor </summary>
			virtual ~GizmoViewportHover();

			/// <summary> ViewportObjectQuery result from the target scene context at cursor location (blocked by SelectionGizmoHover/HandleGizmoHover) </summary>
			ViewportObjectQuery::Result TargetSceneHover()const;

			/// <summary> ViewportObjectQuery result from the gizmo scene context at cursor location for selection layers (blocked by HandleGizmoHover) </summary>
			ViewportObjectQuery::Result SelectionGizmoHover()const;

			/// <summary> ViewportObjectQuery result from the gizmo scene context at cursor location for handle layers </summary>
			ViewportObjectQuery::Result HandleGizmoHover()const;

			/// <summary> ViewportObjectQuery for the target scene </summary>
			ViewportObjectQuery* TargetSceneQuery()const;

			/// <summary> ViewportObjectQuery for the gizmo scene 'selection' layers </summary>
			ViewportObjectQuery* SelectionGizmoQuery()const;

			/// <summary> ViewportObjectQuery for the gizmo scene 'handle' layers </summary>
			ViewportObjectQuery* HandleGizmoQuery()const;

			/// <summary> Short for Vector2(input->GetAxis(MOUSE_POSITION_X), input->GetAxis(MOUSE_POSITION_X)) </summary>
			Vector2 CursorPosition()const;

		private:
			// Underlying updater, that does all the queries
			const Reference<Object> m_updater;

			// Constructor
			GizmoViewportHover(GizmoViewport* viewport);

			// Object cache
			class Cache;
		};
	}
}
