#pragma once
#include <Environment/Rendering/SceneObjects/GraphicsLayer.h>


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Main graphics layers for the gizmo viewport
		/// </summary>
		enum class GizmoLayers : GraphicsLayer {
			/// <summary> Will be drawn in target scene's world space (without depth clearing); viewport pixel queries will ignore these </summary>
			WORLD_SPACE,

			/// <summary> Will be drawn in target scene's world space (without depth clearing) and on click, will select the gizmo's targets </summary>
			SELECTION_WORLD_SPACE,

			/// <summary> Same as SELECTION_WORLD_SPACE, but the renderer will not show these visually </summary>
			SELECTION_WORLD_SPACE_INVISIBLE,

			/// <summary> Will be drawn as an overlay on top of terget scene and on click, viewport pixel queries will ignore these </summary>
			OVERLAY,

			/// <summary> Will be drawn as an overlay on top of terget scene and on click, will select the gizmo's targets </summary>
			SELECTION_OVERLAY,

			/// <summary> Same as SELECTION_OVERLAY but the renderer will not show these visually </summary>
			SELECTION_OVERLAY_INVISIBLE,

			/// <summary> Handles should be drawn on top of everything elese and selection manager will be blocked by these </summary>
			HANDLE,

			/// <summary> Same as HANDLE, but renderer will not display those </summary>
			HANDLE_INVISIBLE
		};
	}
}
