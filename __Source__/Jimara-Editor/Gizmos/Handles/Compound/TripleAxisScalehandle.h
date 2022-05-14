#pragma once
#include "ShapeHandles.h"
#include <Components/GraphicsObjects/MeshRenderer.h>
#include <Data/Materials/SampleDiffuse/SampleDiffuseShader.h>
#include <Data/Generators/MeshGenerator.h>


namespace Jimara {
	namespace Editor {
		class TripleAxisScalehandle : public virtual Transform, Scene::LogicContext::UpdatingComponent {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent comonent </param>
			/// <param name="name"> Gizmo group name </param>
			/// <param name="size"> Gizmo size multiplier </param>
			TripleAxisScalehandle(Component* parent, const std::string_view& name, float size = 1.0f);

			/// <summary>  Virtual destructor </summary>
			virtual ~TripleAxisScalehandle();

			/// <summary> Tells, if the underlying handles are active or not </summary>
			bool HandleActive()const;

			/// <summary> Sum of all underlying giozmo deltas </summary>
			inline Vector3 Delta()const { return m_delta; }

			/// <summary> Sum of all Delta()-s, since scaling started </summary>
			inline Vector3 Scale()const { return m_scale; }

			/// <summary> Invoked when handle starts being dragged </summary>
			inline Event<TripleAxisScalehandle*>& OnHandleActivated() { return m_onHandleActivated; }

			/// <summary> Invoked on each update cycle, while the handle is being manipulated/moved around </summary>
			inline Event<TripleAxisScalehandle*>& OnHandleUpdated() { return m_onHandleUpdated; }

			/// <summary> Invoked when handle stops being dragged </summary>
			inline Event<TripleAxisScalehandle*>& OnHandleDeactivated() { return m_onHandleDeactivated; }

		protected:
			/// <summary> Updates component </summary>
			virtual void Update()override;

			/// <summary> Invoked, when destroyed </summary>
			virtual void OnComponentDestroyed()override;

		private:
			// Underlying handles
			struct ScaleHandle {
				Reference<DragHandle> handle;
				Vector3 defaultPosition;
				Reference<Transform> handleConnector;
			};
			const ScaleHandle m_center;
			const ScaleHandle m_xHandle;
			const ScaleHandle m_yHandle;
			const ScaleHandle m_zHandle;
			const ScaleHandle m_xyHandle;
			const ScaleHandle m_xzHandle;
			const ScaleHandle m_yzHandle;

			// Some helper functions reside here...
			struct Helpers;

			// Hover query
			const Reference<GizmoViewportHover> m_hover;

			// Size multipler
			const float m_size;

			// Internal state
			Vector3 m_hoverOrigin = Vector3(0.0f);
			Vector3 m_delta = Vector3(0.0f);
			Vector3 m_scale = Vector3(1.0f);

			// Handle events
			EventInstance<TripleAxisScalehandle*> m_onHandleActivated;
			EventInstance<TripleAxisScalehandle*> m_onHandleUpdated;
			EventInstance<TripleAxisScalehandle*> m_onHandleDeactivated;

			// Underlying handle events
			void HandleActivated(Handle*);
			void HandleUpdated(Handle* handle);
			void HandleDisabled(Handle*);

			// Actual update function
			void UpdateScale();
		};
	}
}
