#pragma once
namespace Jimara { namespace Audio { class AudioScene; } }
#include "AudioDevice.h"
#include "AudioSource.h"
#include "AudioListener.h"


namespace Jimara {
	namespace Audio {
		/// <summary>
		/// Audio representation of the scene, consisting of 2D and 3D audio emitters(sources) and listeners
		/// </summary>
		class AudioScene : public virtual Object {
		public:
			/// <summary>
			/// Creates a 2D (flat/non-posed/background audio) audio source
			/// Note: When the source goes out of scope, it will automatically be removed from the scene
			/// </summary>
			/// <param name="settings"> Source settings </param>
			/// <param name="clip"> Clip to asign (can be nullptr, if you wish to set it later) </param>
			/// <returns> A new AudioSource2D that resides on the scene </returns>
			virtual Reference<AudioSource2D> CreateSource2D(const AudioSource2D::Settings& settings, AudioClip* clip) = 0;

			/// <summary>
			/// Creates a 3D (posed audio) audio source
			/// Note: When the source goes out of scope, it will automatically be removed from the scene
			/// </summary>
			/// <param name="settings"> Source settings </param>
			/// <param name="clip"> Clip to asign (can be nullptr, if you wish to set it later) </param>
			/// <returns> A new AudioSource3D that resides on the scene </returns>
			virtual Reference<AudioSource3D> CreateSource3D(const AudioSource3D::Settings& settings, AudioClip* clip) = 0;

			/// <summary>
			/// Creates an audio listener
			/// Note: When the listener goes out of scope, it will automatically be removed from the scene
			/// </summary>
			/// <param name="settings"> Listener settings </param>
			/// <returns> A new instance of an AudioListener </returns>
			virtual Reference<AudioListener> CreateListener(const AudioListener::Settings& settings) = 0;

			/// <summary> Device, the scene resides on </summary>
			inline AudioDevice* Device()const { return m_device; }

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="device"> Device, the scene resides on </param>
			inline AudioScene(AudioDevice* device) : m_device(device) {}

		private:
			// Device, the scene resides on
			const Reference<AudioDevice> m_device;
		};
	}
}
