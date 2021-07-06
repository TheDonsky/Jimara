#pragma once
namespace Jimara { namespace Audio { class PhysicalAudioDevice; } }
#include "AudioInstance.h"
#include "AudioDevice.h"


namespace Jimara {
	namespace Audio {
		/// <summary>
		/// Physical audio device descriptor
		/// </summary>
		class PhysicalAudioDevice : public virtual Object {
		public:
			/// <summary> Devcie name </summary>
			virtual const std::string& Name()const = 0;

			/// <summary> True, if this device is selected as the system-wide default device </summary>
			virtual bool IsDefaultDevice()const = 0;

			/// <summary> Creates an instance of a logical AudioDevice </summary>
			virtual Reference<AudioDevice> CreateLogicalDevice() = 0;

			/// <summary> "Owner" AudioInstance </summary>
			virtual AudioInstance* APIInstance()const = 0;
		};
	}
}
