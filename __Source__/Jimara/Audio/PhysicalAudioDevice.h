#pragma once
namespace Jimara { namespace Audio { class PhysicalAudioDevice; } }
#include "AudioInstance.h"
#include "AudioDevice.h"



namespace Jimara {
	namespace Audio {
		class PhysicalAudioDevice : public virtual Object {
		public:
			virtual const std::string& Name()const = 0;

			virtual bool IsDefaultDevice()const = 0;

			virtual Reference<AudioDevice> CreateLogicalDevice() = 0;

			virtual AudioInstance* APIInstance()const = 0;
		};
	}
}
