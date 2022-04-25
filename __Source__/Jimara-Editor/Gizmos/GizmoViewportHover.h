#pragma once
#include "GizmoScene.h"
#include <Environment/GraphicsContext/LightingModels/ObjectIdRenderer/ViewportObjectQuery.h>

namespace Jimara {
	namespace Editor {
		class GizmoViewportHover : public virtual ObjectCache<Reference<Object>>::StoredObject {
		public:
			static Reference<GizmoViewportHover> GetFor(GizmoViewport* viewport);

			virtual ~GizmoViewportHover();

			ViewportObjectQuery::Result TargetSceneHover()const;

			ViewportObjectQuery::Result GizmoSceneHover()const;

		private:
			const Reference<Object> m_updater;

			GizmoViewportHover(GizmoViewport* viewport);

			class Cache;
		};
	}
}
