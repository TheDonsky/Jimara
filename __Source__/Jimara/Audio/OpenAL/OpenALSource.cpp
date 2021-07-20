#include "OpenALSource.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			OpenALSource::OpenALSource(OpenALScene* scene, OpenALClip* clip) 
				: m_scene(scene), m_clip(clip), m_lock(Object::Instantiate<LockInstance>()) {
				m_lock->owner = this;
			}

			OpenALSource::~OpenALSource() {
				Reference<LockInstance> instance = m_lock;
				{
					std::unique_lock<std::mutex> lock(m_lock->lock);
					m_lock->owner = nullptr;
				}
				Stop();
				{
					std::unique_lock<SpinLock> protect(m_refProtect);
					m_lock = nullptr;
				}
			}

			int OpenALSource::Priority()const { return m_priority; }

			void OpenALSource::SetPriority(int priority) {
				if (m_priority == priority) return;
				std::unique_lock<std::mutex> lock(m_lock->lock);
				if (m_priority == priority) return;
				m_priority = priority;
				if (m_playback != nullptr) m_scene->AddPlayback(m_playback, priority);
			}

			AudioSource::PlaybackState OpenALSource::State()const {
				std::unique_lock<std::mutex> lock(m_lock->lock);
				if (m_playback == nullptr) return (m_clip != nullptr && m_time.has_value()) ? PlaybackState::PAUSED : PlaybackState::STOPPED;
				else return m_playback->Playing() ? PlaybackState::PLAYING : PlaybackState::FINISHED;
			}

			void OpenALSource::Play() {
				std::unique_lock<std::mutex> lock(m_lock->lock);
				if ((m_playback != nullptr && m_playback->Playing()) || m_clip == nullptr) return;
				if (m_playback != nullptr) {
					m_scene->RemovePlayback(m_playback);
					{
						std::unique_lock<SpinLock> protect(m_refProtect);
						m_playback = nullptr;
					}
					m_time.reset();
				}
				{
					std::unique_lock<SpinLock> protect(m_refProtect);
					m_playback = BeginPlayback(m_clip, m_time.has_value() ? m_time.value().load() : 0.0f, m_looping);
				}
				m_scene->AddPlayback(m_playback, m_priority);
				dynamic_cast<OpenALInstance*>(m_scene->Device()->APIInstance())->OnTick() += Callback(&OpenALSource::OnTick, this);
			}

			void OpenALSource::Pause() {
				std::unique_lock<std::mutex> lock(m_lock->lock);
				if (m_playback == nullptr) return;
				dynamic_cast<OpenALInstance*>(m_scene->Device()->APIInstance())->OnTick() -= Callback(&OpenALSource::OnTick, this);
				if (m_playback->Playing()) m_time = m_playback->Time();
				if (!m_playback->Playing()) m_time.reset();
				m_scene->RemovePlayback(m_playback);
				{
					std::unique_lock<SpinLock> protect(m_refProtect);
					m_playback = nullptr;
				}
			}

			void OpenALSource::Stop() {
				std::unique_lock<std::mutex> lock(m_lock->lock);
				if (m_playback == nullptr) return;
				dynamic_cast<OpenALInstance*>(m_scene->Device()->APIInstance())->OnTick() -= Callback(&OpenALSource::OnTick, this);
				m_time.reset();
				m_scene->RemovePlayback(m_playback);
				{
					std::unique_lock<SpinLock> protect(m_refProtect);
					m_playback = nullptr;
				}
			}

			float OpenALSource::Time()const {
				std::unique_lock<std::mutex> lock(m_lock->lock);
				if (m_playback != nullptr) return m_playback->Time();
				else return m_time.has_value() ? m_time.value().load() : 0.0f;
			}

			void OpenALSource::SetTime(float time) {
				std::unique_lock<std::mutex> lock(m_lock->lock);
				const bool wasPlaying = (m_playback != nullptr);
				if (wasPlaying) {
					if (m_playback->Time() == time) return;
					m_scene->RemovePlayback(m_playback);
					{
						std::unique_lock<SpinLock> protect(m_refProtect);
						m_playback = nullptr;
					}
				}
				m_time = time;
				if (wasPlaying) {
					{
						std::unique_lock<SpinLock> protect(m_refProtect);
						m_playback = BeginPlayback(m_clip, time, m_looping);
					}
					m_scene->AddPlayback(m_playback, m_priority);
					dynamic_cast<OpenALInstance*>(m_scene->Device()->APIInstance())->OnTick() += Callback(&OpenALSource::OnTick, this);
				}
			}

			bool OpenALSource::Looping()const { return m_looping; }

			void OpenALSource::SetLooping(bool loop) {
				if (m_looping == loop) return;
				std::unique_lock<std::mutex> lock(m_lock->lock);
				if (m_looping == loop) return;
				m_looping = loop;
				if (m_playback != nullptr) m_playback->SetLooping(loop);
			}

			AudioClip* OpenALSource::Clip()const { return m_clip; }

			void OpenALSource::SetClip(AudioClip* clip, bool resetTime) {
				std::unique_lock<std::mutex> lock(m_lock->lock);
				if (m_clip == clip && (!resetTime)) return;
				const bool wasPlaying = (m_playback != nullptr);
				if (wasPlaying) {
					if (dynamic_cast<OpenALClip*>(clip) == nullptr) {
						dynamic_cast<OpenALInstance*>(m_scene->Device()->APIInstance())->OnTick() -= Callback(&OpenALSource::OnTick, this);
						m_time.reset();
					}
					else if ((!resetTime) && m_playback->Playing())
						m_time = Math::FloatRemainder(m_playback->Time(), clip->Duration());
					else if ((!resetTime) && m_time.has_value() && (clip->Duration() > 0.0f))
						m_time = Math::FloatRemainder(m_time.value(), clip->Duration());
					else m_time.reset();
					m_scene->RemovePlayback(m_playback);
					{
						std::unique_lock<SpinLock> protect(m_refProtect);
						m_playback = nullptr;
					}
				}
				else m_time.reset();
				m_clip = dynamic_cast<OpenALClip*>(clip);
				if (wasPlaying && m_clip != nullptr) {
					{
						std::unique_lock<SpinLock> protect(m_refProtect);
						m_playback = BeginPlayback(m_clip, m_time.has_value() ? m_time.value().load() : 0.0f, m_looping);
					}
					m_scene->AddPlayback(m_playback, m_priority);
				}
			}

			void OpenALSource::SetPitch(float pitch) { m_pitch = pitch; }

			Reference<SourcePlayback> OpenALSource::Playback()const { return m_playback; }

			std::mutex& OpenALSource::Lock()const { return m_lock->lock; }

			void OpenALSource::OnTick(float deltaTime, ActionQueue<>& queue) {
				Reference<SourcePlayback> playback;
				{
					std::unique_lock<SpinLock> protect(m_refProtect);
					playback = m_playback;
				}
				if (playback != nullptr && playback->Playing()) playback->AdvanceTime(deltaTime * m_pitch);
				else {
					Reference<Object> lockObj;
					{
						std::unique_lock<SpinLock> protect(m_refProtect);
						lockObj = m_lock;
						if (lockObj == nullptr) return;
					}
					queue.Schedule(Callback<Object*>([](Object* lockPtr) {
						LockInstance* lockAddr = dynamic_cast<LockInstance*>(lockPtr);
						if (lockAddr == nullptr) return;
						std::unique_lock<std::mutex> lock(lockAddr->lock);
						OpenALSource* self = lockAddr->owner;
						if (self == nullptr || self->m_lock == nullptr) return;
						else if (self->m_playback == nullptr || (!self->m_playback->Playing())) {
							dynamic_cast<OpenALInstance*>(self->m_scene->Device()->APIInstance())->OnTick() -= Callback(&OpenALSource::OnTick, self);
							self->m_scene->RemovePlayback(self->m_playback);
						}
						}), lockObj);
				}
			}




			OpenALSource2D::OpenALSource2D(OpenALScene* scene, OpenALClip* clip, const AudioSource2D::Settings& settings) 
				: OpenALSource(scene, clip), m_settings(settings) { SetPitch(m_settings.pitch); }

			void OpenALSource2D::Update(const Settings& newSettings) {
				std::unique_lock<std::mutex> lock(Lock());
				m_settings = newSettings;
				SetPitch(newSettings.pitch);
				Reference<SourcePlayback2D> playback = Playback();
				if (playback != nullptr) playback->Update(newSettings);
			}

			Reference<SourcePlayback> OpenALSource2D::BeginPlayback(OpenALClip* clip, float timeOffset, bool looping) {
				return Object::Instantiate<SourcePlayback2D>(clip, timeOffset, looping, m_settings);
			}


			OpenALSource3D::OpenALSource3D(OpenALScene* scene, OpenALClip* clip, const AudioSource3D::Settings& settings) 
				: OpenALSource(scene, clip), m_settings(settings) { SetPitch(m_settings.pitch); }

			void OpenALSource3D::Update(const Settings& newSettings) {
				std::unique_lock<std::mutex> lock(Lock());
				m_settings = newSettings;
				SetPitch(newSettings.pitch);
				Reference<SourcePlayback3D> playback = Playback();
				if (playback != nullptr) playback->Update(newSettings);
			}

			Reference<SourcePlayback> OpenALSource3D::BeginPlayback(OpenALClip* clip, float timeOffset, bool looping) {
				return Object::Instantiate<SourcePlayback3D>(clip, timeOffset, looping, m_settings);
			}






			SourcePlayback::SourcePlayback(OpenALClip* clip, float timeOffset, bool looping) : m_clip(clip), m_time(timeOffset), m_looping(looping) {}

			OpenALClip* SourcePlayback::Clip()const { return m_clip; }
			
			bool SourcePlayback::Playing() {
				return Looping() || m_clip->Duration() < m_time;
			}

			bool SourcePlayback::Looping()const { return m_looping; }

			void SourcePlayback::SetLooping(bool looping) { m_looping = looping; }

			float SourcePlayback::Time()const { return m_time; }

			void SourcePlayback::AdvanceTime(float deltaTime) {
				m_time = m_looping ?
					min(m_time + deltaTime, Clip()->Duration()) :
					Math::FloatRemainder(m_time + deltaTime, Clip()->Duration());
			}


			SourcePlayback2D::SourcePlayback2D(OpenALClip* clip, float timeOffset, bool looping, const AudioSource2D::Settings& settings) 
				: SourcePlaybackWithClipPlaybacks<AudioSource2D::Settings, ClipPlayback2D>(clip, timeOffset, looping, settings) {}

			Reference<ClipPlayback2D> SourcePlayback2D::BeginClipPlayback(const AudioSource2D::Settings& settings, ListenerContext* context, bool looping, float timeOffset) {
				return Clip()->Play2D(context, settings, looping, timeOffset);
			}

			SourcePlayback3D::SourcePlayback3D(OpenALClip* clip, float timeOffset, bool looping, const AudioSource3D::Settings& settings)
				: SourcePlaybackWithClipPlaybacks<AudioSource3D::Settings, ClipPlayback3D>(clip, timeOffset, looping, settings) {}

			Reference<ClipPlayback3D> SourcePlayback3D::BeginClipPlayback(const AudioSource3D::Settings& settings, ListenerContext* context, bool looping, float timeOffset) {
				return Clip()->Play3D(context, settings, looping, timeOffset);
			}
		}
	}
}
