#pragma once
#include "../../Handles/DragHandle.h"


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Handle for controlling a box-type boundary
		/// </summary>
		class BoxResizeHandle : public virtual Component {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parent"> Parent component </param>
			/// <param name="color"> Handle color </param>
			BoxResizeHandle(Component* parent, const Vector3& color);

			/// <summary> Virtual destructor </summary>
			virtual ~BoxResizeHandle();

			/// <summary>
			/// Updates handle visuals and inputs size
			/// </summary>
			/// <param name="position"> World-space position </param>
			/// <param name="rotation"> World-space euler angles </param>
			/// <param name="size"> Local-space box size (will be updated if the handles are pulled) </param>
			void Update(Vector3 position, Vector3 rotation, Vector3& size);

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
			const Reference<DragHandle> m_resizeFront;
			const Reference<DragHandle> m_resizeBack;

			// Basic helper interface for internal logic
			struct Helpers;
		};
	}
}
