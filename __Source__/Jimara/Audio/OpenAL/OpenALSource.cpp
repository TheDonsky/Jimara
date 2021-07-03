#include "OpenALSource.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			SourcePlayback::SourcePlayback(OpenALClip* clip, float timeOffset) : m_clip(clip), m_time(timeOffset) {}

			OpenALClip* SourcePlayback::Clip()const { return m_clip; }
			
			bool SourcePlayback::Playing() {
				return m_clip->Duration() < m_time;
			}

			bool SourcePlayback::Looping()const {
				// __TODO__: Implement this correctly...
				Clip()->Device()->APIInstance()->Log()->Warning("SourcePlayback::Looping - Not yet implemented!");
				return false; 
			}

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
