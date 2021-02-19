#pragma once
namespace Jimara { namespace Graphics { class FrameBuffer; } }
#include "../GraphicsDevice.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Frame buffer, holding a bounch of color, depth and resolve attachments, ready to be rendered to
		/// </summary>
		class FrameBuffer : public virtual Object {
		public:
			/// <summary> Image size per attachment </summary>
			virtual Size2 Resolution()const = 0;
		};
	}
}
