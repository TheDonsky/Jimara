#pragma once
namespace Jimara {
	namespace Graphics {
		class RenderSurface;
	}
}
#include "../GraphicsInstance.h"

namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Surface, we can draw graphics on
		/// </summary>
		class RenderSurface : public virtual Object {
		public:
			/// <summary> Virtual destructor </summary>
			inline virtual ~RenderSurface() {}

			/// <summary>
			/// Tells, if given physical device can draw on the surface or not
			/// </summary>
			/// <param name="device"> Device to check </param>
			/// <returns> True, if the device is compatible with the surface </returns>
			virtual bool DeviceCompatible(PhysicalDevice* device)const = 0;

			/// <summary> Size of the surface (in pixels) </summary>
			virtual glm::uvec2 Size()const = 0;
		};
	}
}
