#pragma once
namespace Jimara { namespace Audio { class AudioDevice; } }
#include "AudioInstance.h"
#include "AudioScene.h"


namespace Jimara {
	namespace Audio {
		/// <summary>
		/// Logical device, that can emit audio and share resources in-between scenes
		/// </summary>
		class JIMARA_API AudioDevice : public virtual Object {
		public:
			/// <summary> Instantiates a new AudioScene to play around in </summary>
			virtual Reference<AudioScene> CreateScene() = 0;

			/// <summary>
			/// Creates a new Audio clip based on a buffer
			/// </summary>
			/// <param name="buffer"> Buffer to base the clip on </param>
			/// <param name="streamed"> If true, the AudioClip will not keep the whole buffer in memory all the time and will synamically load chunks as needed </param>
			/// <returns> A new instance of an AudioClip </returns>
			virtual Reference<AudioClip> CreateAudioClip(const AudioBuffer* buffer, bool streamed) = 0;

			/// <summary> Audio framework instance </summary>
			inline AudioInstance* APIInstance()const { return m_instance; }

			/// <summary> Physical device, this logical device is tied to </summary>
			inline PhysicalAudioDevice* PhysicalDevice()const { return m_physicalDevice; }

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="instance"> Audio framework instance </param>
			/// <param name="physicalDevice"> Physical device, this logical device is tied to </param>
			inline AudioDevice(AudioInstance* instance, PhysicalAudioDevice* physicalDevice) : m_instance(instance), m_physicalDevice(physicalDevice) {}

		private:
			// Audio framework instance
			const Reference<AudioInstance> m_instance;

			// Physical device, this logical device is tied to
			const Reference<PhysicalAudioDevice> m_physicalDevice;
		};
	}
}