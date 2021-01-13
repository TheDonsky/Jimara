#pragma once
#include "../Core/Object.h"
#include "../OS/Logging/Logger.h"
#include "../Application/AppInformation.h"
namespace Jimara { namespace Graphics { class GraphicsInstance; } }
#include "PhysicalDevice.h"

namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Graphics API instance
		/// </summary>
		class GraphicsInstance : public virtual Object {
		public:
			/// <summary> Underlying graphics API </summary>
			enum class Backend : uint8_t {
				/// <summary> Vulkan API </summary>
				VULKAN = 0,

				/// <summary> Not an actual backend; represents merely the count of available backends </summary>
				BACKEND_OPTION_COUNT = 1
			};

			/// <summary>
			/// Instantiates graphics instance
			/// </summary>
			/// <param name="logger"> Logger </param>
			/// <param name="appInfo"> Basic info abot the application </param>
			/// <param name="backend"> Graphics backend </param>
			/// <returns></returns>
			static Reference<GraphicsInstance> Create(OS::Logger* logger, const Application::AppInformation* appInfo, Backend backend = Backend::VULKAN);

			/// <summary> Virtual destructor </summary>
			virtual ~GraphicsInstance();

			/// <summary> Logger </summary>
			OS::Logger* Log()const;

			/// <summary> Application information </summary>
			const Application::AppInformation* AppInfo()const;

			/// <summary> Number of available physical devices </summary>
			virtual size_t PhysicalDeviceCount()const = 0;

			/// <summary> 
			/// Physical device by index 
			/// </summary>
			/// <param name="index"> Physical device index </param>
			/// <returns> Physical device </returns>
			virtual PhysicalDevice* GetPhysicalDevice(size_t index)const = 0;


		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="logger"> Logger </param>
			/// <param name="appInfo"> Basic info abot the application </param>
			GraphicsInstance(OS::Logger* logger, const Application::AppInformation* appInfo);

		private:
			// Logger
			Reference<OS::Logger> m_logger;

			// Application Information
			Reference<const Application::AppInformation> m_appInfo;
		};
	}
}
