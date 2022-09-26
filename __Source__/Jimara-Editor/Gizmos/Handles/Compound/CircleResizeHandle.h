#pragma once
#include "../../Handles/DragHandle.h"


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Resize handle for circular boundary
		/// </summary>
		class CircleResizeHandle : public virtual Component {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="color"> Handle color </param>
			CircleResizeHandle(Component* parent, const Vector3& color);

			/// <summary> Virtual destructor </summary>
			virtual ~CircleResizeHandle();

			/// <summary>
			/// Updates handle visuals and inputs new radius
			/// </summary>
			/// <param name="position"> World-space position </param>
			/// <param name="rotation"> World-space euler angles </param>
			/// <param name="radius"> Local-space radius </param>
			void Update(Vector3 position, Vector3 rotation, float& radius);

		protected:
			/// <summary> Invoked, wnen the component stops being active in hierarchy </summary>
			virtual void OnComponentDisabled()override;

			/// <summary> Invoked, wnen the component becomes active in hierarchy </summary>
			virtual void OnComponentEnabled()override;

			/// <summary> Invoked, wnen the component gets destroyed </summary>
			virtual void OnComponentDestroyed()override;

		private:
			// Renderer transform
			const Reference<Transform> m_poseTransform;

			// Handles
			const Reference<DragHandle> m_resizeRight;
			const Reference<DragHandle> m_resizeLeft;
			const Reference<DragHandle> m_resizeUp;
			const Reference<DragHandle> m_resizeDown;

			// Basic helper interface for internal logic
			struct Helpers;
		};
	}
}
