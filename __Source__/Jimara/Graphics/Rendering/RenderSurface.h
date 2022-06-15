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
		class JIMARA_API RenderSurface : public virtual Object {
		public:
			/// <summary> Virtual destructor </summary>
			inline virtual ~RenderSurface() {}

			/// <summary>
			/// Tells, if given physical device can draw on the surface or not
			/// </summary>
			/// <param name="device"> Device to check </param>
			/// <returns> True, if the device is compatible with the surface </returns>
			virtual bool DeviceCompatible(const PhysicalDevice* device)const = 0;

			/// <summary> Size of the surface (in pixels) </summary>
			virtual Size2 Size()const = 0;

			/// <summary> "Owner" graphics instance </summary>
			Graphics::GraphicsInstance* GraphicsInstance()const;

			/// <summary> "Recommended" graphics device for the surface </summary>
			PhysicalDevice* PrefferedDevice()const;

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="graphicsInstance"> "Owner" graphics instance </param>
			RenderSurface(Graphics::GraphicsInstance* graphicsInstance);

		private:
			// "Owner" graphics instance
			Reference<Graphics::GraphicsInstance> m_graphicsInstance;
		};
	}
}
