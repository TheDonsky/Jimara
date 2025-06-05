#pragma once
#include "../../Gizmo.h"
#include "../../Handles/DragHandle.h"
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Resize handle for capsule-shaped boundary
		/// </summary>
		class CapsuleResizeHandle : public virtual Component {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="color"> Handle color </param>
			CapsuleResizeHandle(Component* parent, const Vector3& color);

			/// <summary> Virtual destructor </summary>
			virtual ~CapsuleResizeHandle();

			/// <summary>
			/// Updates handle visuals and inputs radius/height
			/// </summary>
			/// <param name="position"> World-space position </param>
			/// <param name="rotation"> World-space euler angles </param>
			/// <param name="radius"> Local-space radius </param>
			/// <param name="height"> Local-space height </param>
			void Update(Vector3 position, Vector3 rotation, float& radius, float& height);

		protected:
			/// <summary> Invoked by the scene on the first frame this component gets instantiated </summary>
			virtual void OnComponentInitialized()override;

			/// <summary> Invoked, wnen the component stops being active in hierarchy </summary>
			virtual void OnComponentDisabled()override;

			/// <summary> Invoked, wnen the component becomes active in hierarchy </summary>
			virtual void OnComponentEnabled()override;

			/// <summary> Invoked, wnen the component gets destroyed </summary>
			virtual void OnComponentDestroyed()override;

		private:
			// Underlying renderer and last capsule shape
			const Reference<MeshRenderer> m_renderer;
			float m_lastRadius = 0.0f;
			float m_lastHeight = 0.0f;

			// Handles
			struct RadiusHandles {
				const Reference<DragHandle> right;
				const Reference<DragHandle> left;
				const Reference<DragHandle> front;
				const Reference<DragHandle> back;

				RadiusHandles(CapsuleResizeHandle* parent);
			};
			struct HeightHandles {
				const Reference<DragHandle> top;
				const Reference<DragHandle> bottom;

				HeightHandles(CapsuleResizeHandle* parent);
			};
			HeightHandles m_heightHandles;
			RadiusHandles m_topRadiusHandles;
			RadiusHandles m_bottomRadiusHandles;

			// Basic helper interface for internal logic
			struct Helpers;
		};
	}
}

