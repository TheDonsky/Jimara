#pragma once
#include "../Core/Object.h"
namespace Jimara { namespace Graphics { class PhysicalDevice; } }
#include "GraphicsInstance.h"
#include "LogicalDevice.h"

namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Physical graphics device 
		/// (Can be an actual discrete/intergated GPU or CPU or even some virtual amalgamation of those and more... 
		/// All the user should care about is that this object has certain graphics capabilities)
		/// </summary>
		class PhysicalDevice {
		public:
			/// <summary> Virtual destructor </summary>
			virtual ~PhysicalDevice();

			/// <summary> Increments reference count </summary>
			void AddRef()const;

			/// <summary> Decrements reference count </summary>
			void ReleaseRef()const;

			/// <summary> "Owner" Graphics instance </summary>
			Graphics::GraphicsInstance* GraphicsInstance()const;

			/// <summary> Logger </summary>
			OS::Logger* Log()const;

			/// <summary> Instantiates a logical device </summary>
			virtual Reference<LogicalDevice> CreateLogicalDevice() = 0;

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="instance"> Owner </param>
			PhysicalDevice(Graphics::GraphicsInstance* instance);

		private:
			// 'Owner' instance
			Graphics::GraphicsInstance* m_owner;
		};
	}
}
