#pragma once
namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			class OpenALClip;
			class ClipPlayback;
			class ClipPlayback2D;
			class ClipPlayback3D;
		}
	}
}
#include "OpenALDevice.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			/// <summary>
			/// OpenAL-baked audio clip
			/// </summary>
			class OpenALClip : public virtual AudioClip {
			public:
				/// <summary>
				/// Creates a new OpenALClip
				/// </summary>
				/// <param name="device"> Device, the clip should reside on </param>
				/// <param name="buffer"> Audio buffer to extract data from </param>
				/// <param name="streamed"> If true, the buffer will be broken up in chunks and loaded and evicted on demand </param>
				/// <returns> A new instance of an OpenALClip </returns>
				static Reference<OpenALClip> Create(OpenALDevice* device, const AudioBuffer* buffer, bool streamed);

				/// <summary> Logical device, the buffer resides on </summary>
				OpenALDevice* Device()const;

				/// <summary>
				/// Creates a 2d clip playback
				/// </summary>
				/// <param name="context"> Listener </param>
				/// <param name="settings"> Initial source settings </param>
				/// <param name="loop"> If true, the playback will loop indefinately (unless Loop gets invoked with false, or the source is manually stopped) </param>
				/// <param name="timeOffset"> Initial time offset in case the source playback was paused, evicted or manually offset by the system or the user </param>
				/// <returns> A new instance of a clip playback </returns>
				virtual Reference<ClipPlayback2D> Play2D(ListenerContext* context, const AudioSource2D::Settings& settings, bool loop, float timeOffset) = 0;

				/// <summary>
				/// Creates a 3d clip playback
				/// </summary>
				/// <param name="context"> Listener </param>
				/// <param name="settings"> Initial source settings </param>
				/// <param name="loop"> If true, the playback will loop indefinately (unless Loop gets invoked with false, or the source is manually stopped) </param>
				/// <param name="timeOffset"> Initial time offset in case the source playback was paused, evicted or manually offset by the system or the user </param>
				/// <returns> A new instance of a clip playback </returns>
				virtual Reference<ClipPlayback3D> Play3D(ListenerContext* context, const AudioSource3D::Settings& settings, bool loop, float timeOffset) = 0;

			protected:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="device"> Logical device, the buffer resides on </param>
				/// <param name="buffer"> Audio buffer to extract data from </param>
				OpenALClip(OpenALDevice* device, const AudioBuffer* buffer);

			private:
				// Logical device, the buffer resides on
				const Reference<OpenALDevice> m_device;
			};

			/// <summary>
			/// Clip playback is an object that handles actual OpenAL source playback on a particular OpenAL context
			/// </summary>
			class ClipPlayback : public virtual Object {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="context"> Listener context </param>
				ClipPlayback(ListenerContext* context);

				/// <summary> Virtual destructor (stops playback and releases resources) </summary>
				virtual ~ClipPlayback();

				/// <summary> Returns true, when the playback is actively outputting audio </summary>
				virtual bool Playing() = 0;

				/// <summary>
				/// Makes the playback looping or non-looping
				/// </summary>
				/// <param name="loop"> If true, the playback will loop till made non-looping or goes out of scope </param>
				virtual void Loop(bool loop) = 0;

			protected:
				/// <summary> Listener context </summary>
				ListenerContext* Context()const;

				/// <summary> OpenAL source </summary>
				ALuint Source()const;

			private:
				// Listener context
				const Reference<ListenerContext> m_context;

				// OpenAL source
				const ALuint m_source;
			};

			/// <summary>
			/// 2d/non-posed/background audio playback
			/// </summary>
			class ClipPlayback2D : public ClipPlayback {
			protected:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="context"> Listener context </param>
				/// <param name="settings"> Initial source settings </param>
				ClipPlayback2D(ListenerContext* context, const AudioSource2D::Settings& settings);

			public:
				/// <summary>
				/// Updates source settings
				/// </summary>
				/// <param name="settings"> Source settings to use </param>
				void Update(const AudioSource2D::Settings& settings);
			};

			/// <summary>
			/// 3d/posed/world space audio playback
			/// </summary>
			class ClipPlayback3D : public ClipPlayback {
			protected:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="context"> Listener context </param>
				/// <param name="settings"> Initial source settings </param>
				ClipPlayback3D(ListenerContext* context, const AudioSource3D::Settings& settings);

			public:
				/// <summary>
				/// Updates source settings
				/// </summary>
				/// <param name="settings"> Source settings to use </param>
				void Update(const AudioSource3D::Settings& settings);
			};
		}
	}
}
