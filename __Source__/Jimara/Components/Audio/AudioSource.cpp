#include "AudioSource.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
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

	void AudioSource::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement, {
			JIMARA_SERIALIZE_FIELD_GET_SET(Volume, SetVolume, "Volume", "Source volume");
			JIMARA_SERIALIZE_FIELD_GET_SET(Pitch, SetPitch, "Pitch", "Playback speed");
			JIMARA_SERIALIZE_FIELD_GET_SET(Priority, SetPriority, "Priority",
				"Source priority (in case there are some limitations about the number of actively playing sounds on the underlying hardware, "
				"higherst priority ones will be heared)");
			JIMARA_SERIALIZE_FIELD_GET_SET(Looping, SetLooping, "Looping", "If true, playback will keep looping untill paused/stopped or made non-looping");
			JIMARA_SERIALIZE_FIELD_GET_SET(Clip, SetClip, "Clip", "Audio clip, currently playing");
			})
		typedef Serialization::ItemSerializer::Of<AudioSource> FieldSerializer;
		{
			static const Reference<const FieldSerializer> serializer = Serialization::BoolSerializer::For<AudioSource>(
				"Playing", "True, while playing",
				[](AudioSource* source) -> bool { return source->Playing(); },
				[](const bool& value, AudioSource* source) {
					if (value) source->Play();
					else source->Stop();
				});
			recordElement(serializer->Serialize(this));
		}
	}

	void AudioSource::OnComponentEnabled() { SynchSource(); }
	
	void AudioSource::OnComponentDisabled() { SynchSource(); }

	AudioSource::AudioSource(Reference<Audio::AudioSource> source, float volume, float pitch)
		: m_source(source) {
		SetVolume(volume);
		SetPitch(pitch);
	}

	Audio::AudioSource* AudioSource::Source()const { return m_source; }




	namespace {
		inline static Audio::AudioSource2D::Settings Settings2D(Component* source, float volume, float pitch) {
			Audio::AudioSource2D::Settings settings;
			settings.volume = source->ActiveInHeirarchy() ? volume : 0.0f;
			settings.pitch = pitch;
			return settings;
		}
		
		template<typename SourceType, typename SettingsType>
		inline static void UpdateSources(
			AudioSource* sourceComponent,
			Audio::AudioSource* source, const SettingsType& settings, SettingsType& oldSettings, std::mutex& mutex, 
			std::unordered_set<Reference<SourceType>>& oneShotSources) {
			if (settings == oldSettings) return;
			
			oldSettings = settings;
			dynamic_cast<SourceType*>(source)->Update(settings);
			
			std::unique_lock<std::mutex> lock(mutex);
			if (sourceComponent->ActiveInHeirarchy()) {
				static thread_local std::vector<Reference<SourceType>> outOfScopeSources;
				for (typename std::unordered_set<Reference<SourceType>>::const_iterator it = oneShotSources.begin(); it != oneShotSources.end(); ++it) {
					SourceType* source = (*it);
					if (source->State() == Audio::AudioSource::PlaybackState::PLAYING) source->Update(settings);
					else outOfScopeSources.push_back(source);
				}
				for (size_t i = 0; i < outOfScopeSources.size(); i++) oneShotSources.erase(outOfScopeSources[i]);
				outOfScopeSources.clear();
			}
			else {
				for (typename std::unordered_set<Reference<SourceType>>::const_iterator it = oneShotSources.begin(); it != oneShotSources.end(); ++it)
					(*it)->Stop();
				oneShotSources.clear();
			}
		}
	}


	AudioSource2D::AudioSource2D(Component* parent, const std::string_view& name, Audio::AudioClip* clip, float volume, float pitch) 
		: Component(parent, name)
		, AudioSource(parent->Context()->Audio()->AudioScene()->CreateSource2D(Settings2D(this, volume, pitch), clip), volume, pitch)
		, m_settings(Settings2D(this, volume, pitch)) {}

	void AudioSource2D::PlayOneShot(Audio::AudioClip* clip) {
		if (clip == nullptr || (!ActiveInHeirarchy())) return;
		Reference<Audio::AudioSource2D> source = Context()->Audio()->AudioScene()->CreateSource2D(Settings2D(this, Volume(), Pitch()), clip);
		source->SetPriority(Priority());
		source->Play();

		std::unique_lock<std::mutex> lock(m_lock);
		m_oneShotSources.insert(source);
	}

	void AudioSource2D::SynchSource() {
		UpdateSources(this, Source(), Settings2D(this, Volume(), Pitch()), m_settings, m_lock, m_oneShotSources);
	}



	namespace {
		static thread_local Audio::AudioSource3D::Settings lastSettings;

		inline static const Audio::AudioSource3D::Settings& Settings3D(Component* component, float volume, float pitch) {
			const Transform* transform = component->GetTransfrom();
			const Rigidbody* rigidbody = component->GetComponentInParents<Rigidbody>();
			Audio::AudioSource3D::Settings& settings = lastSettings;
			settings.position = (transform == nullptr) ? Vector3(0.0f) : transform->WorldPosition();
			settings.velocity = (rigidbody == nullptr) ? Vector3(0.0f) : rigidbody->Velocity();
			settings.volume = component->ActiveInHeirarchy() ? volume : 0.0f;
			settings.pitch = pitch;
			return settings;
		}
	}


	AudioSource3D::AudioSource3D(Component* parent, const std::string_view& name, Audio::AudioClip* clip, float volume, float pitch)
		: Component(parent, name)
		, AudioSource(parent->Context()->Audio()->AudioScene()->CreateSource3D(Settings3D(parent, volume, pitch), clip), volume, pitch) {
		m_settings = lastSettings;
	}

	void AudioSource3D::PlayOneShot(Audio::AudioClip* clip) {
		if (clip == nullptr || (!ActiveInHeirarchy())) return;
		Reference<Audio::AudioSource3D> source = Context()->Audio()->AudioScene()->CreateSource3D(Settings3D(this, Volume(), Pitch()), clip);
		source->SetPriority(Priority());
		source->Play();

		std::unique_lock<std::mutex> lock(m_lock);
		m_oneShotSources.insert(source);
	}

	void AudioSource3D::SynchSource() {
		UpdateSources(this, Source(), Settings3D(this, Volume(), Pitch()), m_settings, m_lock, m_oneShotSources);
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<AudioSource>(const Callback<const Object*>& report) {}
	template<> void TypeIdDetails::GetTypeAttributesOf<AudioSource2D>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<AudioSource2D> serializer("Jimara/Audio/AudioSource2D", "Audio Source 2D");
		report(&serializer);
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<AudioSource3D>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<AudioSource3D> serializer("Jimara/Audio/AudioSource3D", "Audio Source 3D");
		report(&serializer);
	}
}
