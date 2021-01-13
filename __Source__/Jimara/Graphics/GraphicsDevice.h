#pragma once
#include "../Core/Object.h"
namespace Jimara { namespace Graphics { class GraphicsDevice; } }
#include "PhysicalDevice.h"

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
