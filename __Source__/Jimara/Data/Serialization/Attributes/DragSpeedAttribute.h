#pragma once
#include "../../../Core/Object.h"


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Simple attribute, telling the editor how fast field dragging should be
		/// </summary>
		class DragSpeedAttribute : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="speed"> Drag speed </param>
			inline DragSpeedAttribute(float speed) : m_speed(speed) {}

			/// <summary> Drag speed </summary>
			inline float Speed()const { return m_speed; }

		private:
			// Drag speed
			const float m_speed;
		};
	}
}
