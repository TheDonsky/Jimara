#include "OpenALSource.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			SourcePlayback::SourcePlayback(OpenALClip* clip, float timeOffset, bool looping) : m_clip(clip), m_time(timeOffset), m_looping(looping) {}

			OpenALClip* SourcePlayback::Clip()const { return m_clip; }
			
			bool SourcePlayback::Playing() {
				return Looping() || m_clip->Duration() < m_time;
			}

			bool SourcePlayback::Looping()const { return m_looping; }

			void SourcePlayback::SetLooping(bool looping) { m_looping = looping; }

			float SourcePlayback::Time()const { return m_time; }


			Reference<ClipPlayback2D> SourcePlayback2D::BeginClipPlayback(const AudioSource2D::Settings& settings, ListenerContext* context, bool looping, float timeOffset) {
				return Clip()->Play2D(context, settings, looping, timeOffset);
			}

			Reference<ClipPlayback3D> SourcePlayback3D::BeginClipPlayback(const AudioSource3D::Settings& settings, ListenerContext* context, bool looping, float timeOffset) {
				return Clip()->Play3D(context, settings, looping, timeOffset);
			}
		}
	}
}
