#pragma once
#include "GizmoScene.h"
#include <Environment/GraphicsContext/LightingModels/ObjectIdRenderer/ViewportObjectQuery.h>

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

			/// <summary> ViewportObjectQuery result from the target scene context at cursor location </summary>
			ViewportObjectQuery::Result TargetSceneHover()const;

			/// <summary> ViewportObjectQuery result from the gizmo scene context at cursor location </summary>
			ViewportObjectQuery::Result GizmoSceneHover()const;

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
