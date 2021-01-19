#pragma once
namespace Jimara { namespace Graphics { class GraphicsDevice; } }
#include "../Core/Object.h"
#include "PhysicalDevice.h"
#include "Rendering/RenderEngine.h"
#include "Rendering/RenderSurface.h"

namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Logical graphics device
		/// </summary>
		class GraphicsDevice : public virtual Object {
		public:
			/// <summary> Virtual destructor </summary>
			virtual ~GraphicsDevice();

			/// <summary> Underlying physical device </summary>
			Graphics::PhysicalDevice* PhysicalDevice()const;

			/// <summary> "Owner" graphics instance </summary>
			Graphics::GraphicsInstance* GraphicsInstance()const;

			/// <summary> Logger </summary>
			OS::Logger* Log()const;

			/// <summary>
			/// Instantiates a render engine (Depending on the context/os etc only one per surface may be allowed)
			/// </summary>
			/// <param name="targetSurface"> Surface to render to </param>
			/// <returns> New instance of a render engine </returns>
			virtual Reference<RenderEngine> CreateRenderEngine(RenderSurface* targetSurface) = 0;


		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="physicalDevice"> Underlying physical device </param>
			GraphicsDevice(Graphics::PhysicalDevice* physicalDevice);

		private:
			// Underlying physical device
			Reference<Graphics::PhysicalDevice> m_physicalDevice;
		};
	}
}
