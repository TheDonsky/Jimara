#include "AudioSource.h"
#include "../Physics/Rigidbody.h"


namespace Jimara {
	float AudioSource::Volume()const { return m_volume; }

	void AudioSource::SetVolume(float volume) { m_volume = volume; }

	float AudioSource::Pitch()const { return m_pitch; }

	void AudioSource::SetPitch(float pitch) { m_pitch = pitch; }

	int AudioSource::Priority()const { return m_source->Priority(); }

	void AudioSource::SetPriority(int priority) { m_source->SetPriority(priority); }

	bool AudioSource::Looping()const { return m_source->Looping(); }

	void AudioSource::SetLooping(bool looping) { m_source->SetLooping(looping); }

	Audio::AudioClip* AudioSource::Clip()const { return m_source->Clip(); }

	void AudioSource::SetClip(Audio::AudioClip* clip) { m_source->SetClip(clip, true); }

	bool AudioSource::Playing()const { return m_source->State() == Audio::AudioSource::PlaybackState::PLAYING; }

	void AudioSource::Play() { SynchSource(); m_source->Play(); }

	void AudioSource::Play(Audio::AudioClip* clip) { SetClip(clip); Play(); }

	void AudioSource::Pause() { m_source->Pause(); }

	void AudioSource::Stop() { m_source->Stop(); }

	void AudioSource::Update() { SynchSource(); }

	AudioSource::AudioSource(Reference<Audio::AudioSource> source, float volume, float pitch)
		: m_source(source) {
		SetVolume(volume);
		SetPitch(pitch);
	}

	Audio::AudioSource* AudioSource::Source()const { return m_source; }




	namespace {
		inline static Audio::AudioSource2D::Settings Settings(float volume, float pitch) {
			Audio::AudioSource2D::Settings settings;
			settings.volume = volume;
			settings.pitch = pitch;
			return settings;
		}
		
		template<typename SourceType, typename SettingsType>
		inline static void UpdateSources(
			Audio::AudioSource* source, const SettingsType& settings, SettingsType& oldSettings, std::mutex& mutex, 
			std::unordered_set<Reference<SourceType>>& oneShotSources) {
			if (settings == oldSettings) return;
			
			oldSettings = settings;
			dynamic_cast<SourceType*>(source)->Update(settings);
			
			std::unique_lock<std::mutex> lock(mutex);
			
			static thread_local std::vector<Reference<SourceType>> outOfScopeSources;
			for (typename std::unordered_set<Reference<SourceType>>::const_iterator it = oneShotSources.begin(); it != oneShotSources.end(); ++it) {
				SourceType* source = (*it);
				if (source->State() == Audio::AudioSource::PlaybackState::PLAYING) source->Update(settings);
				else outOfScopeSources.push_back(source);
			}
			for (size_t i = 0; i < outOfScopeSources.size(); i++) oneShotSources.erase(outOfScopeSources[i]);
			outOfScopeSources.clear();
		}
	}


	AudioSource2D::AudioSource2D(Component* parent, const std::string_view& name, Audio::AudioClip* clip, float volume, float pitch) 
		: Component(parent, name)
		, AudioSource(parent->Context()->Audio()->AudioScene()->CreateSource2D(Settings(volume, pitch), clip), volume, pitch)
		, m_settings(Settings(volume, pitch)) {}

	void AudioSource2D::PlayOneShot(Audio::AudioClip* clip) {
		if (clip == nullptr) return;
		Reference<Audio::AudioSource2D> source = Context()->Audio()->AudioScene()->CreateSource2D(Settings(Volume(), Pitch()), clip);
		source->SetPriority(Priority());
		source->Play();

		std::unique_lock<std::mutex> lock(m_lock);
		m_oneShotSources.insert(source);
	}

	void AudioSource2D::SynchSource() {
		UpdateSources(Source(), Settings(Volume(), Pitch()), m_settings, m_lock, m_oneShotSources);
	}



	namespace {
		static thread_local Audio::AudioSource3D::Settings lastSettings;

		inline static const Audio::AudioSource3D::Settings& Settings(Component* component, float volume, float pitch) {
			const Transform* transform = component->GetTransfrom();
			const Rigidbody* rigidbody = component->GetComponentInParents<Rigidbody>();
			Audio::AudioSource3D::Settings& settings = lastSettings;
			settings.position = (transform == nullptr) ? Vector3(0.0f) : transform->WorldPosition();
			settings.velocity = (rigidbody == nullptr) ? Vector3(0.0f) : rigidbody->Velocity();
			settings.volume = volume;
			settings.pitch = pitch;
			return settings;
		}
	}


	AudioSource3D::AudioSource3D(Component* parent, const std::string_view& name, Audio::AudioClip* clip, float volume, float pitch)
		: Component(parent, name)
		, AudioSource(parent->Context()->Audio()->AudioScene()->CreateSource3D(Settings(parent, volume, pitch), clip), volume, pitch) {
		m_settings = lastSettings;
	}

	void AudioSource3D::PlayOneShot(Audio::AudioClip* clip) {
		if (clip == nullptr) return;
		Reference<Audio::AudioSource3D> source = Context()->Audio()->AudioScene()->CreateSource3D(Settings(this, Volume(), Pitch()), clip);
		source->SetPriority(Priority());
		source->Play();

		std::unique_lock<std::mutex> lock(m_lock);
		m_oneShotSources.insert(source);
	}

	void AudioSource3D::SynchSource() {
		UpdateSources(Source(), Settings(this, Volume(), Pitch()), m_settings, m_lock, m_oneShotSources);
	}
}
