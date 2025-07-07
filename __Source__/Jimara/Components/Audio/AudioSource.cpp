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
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Volume, SetVolume, "Volume", "Source volume");
			JIMARA_SERIALIZE_FIELD_GET_SET(Pitch, SetPitch, "Pitch", "Playback speed");
			JIMARA_SERIALIZE_FIELD_GET_SET(Priority, SetPriority, "Priority",
				"Source priority (in case there are some limitations about the number of actively playing sounds on the underlying hardware, "
				"higherst priority ones will be heared)");
			JIMARA_SERIALIZE_FIELD_GET_SET(Looping, SetLooping, "Looping", "If true, playback will keep looping untill paused/stopped or made non-looping");
			JIMARA_SERIALIZE_FIELD_GET_SET(Clip, SetClip, "Clip", "Audio clip, currently playing");
		};
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

	void AudioSource::GetSerializedActions(Callback<Serialization::SerializedCallback> report) {
		Component::GetSerializedActions(report);

		// Volume:
		{
			static const auto serializer = Serialization::DefaultSerializer<float>::Create("Volume", "Source volume");
			report(Serialization::SerializedCallback::Create<float>::From("SetVolume", Callback<float>(&AudioSource::SetVolume, this), serializer));
		}

		// Pitch:
		{
			static const auto serializer = Serialization::DefaultSerializer<float>::Create("Pitch", "Playback speed");
			report(Serialization::SerializedCallback::Create<float>::From("SetPitch", Callback<float>(&AudioSource::SetPitch, this), serializer));
		}

		// Priority:
		{
			static const auto serializer = Serialization::DefaultSerializer<int>::Create("Priority",
				"Source priority (in case there are some limitations about the number of actively playing sounds on the underlying hardware, "
				"higherst priority ones will be heared)");
			report(Serialization::SerializedCallback::Create<int>::From("SetPriority", Callback<int>(&AudioSource::SetPriority, this), serializer));
		}

		// Looping:
		{
			static const auto serializer = Serialization::DefaultSerializer<bool>::Create(
				"Looping", "If true, playback will keep looping untill paused/stopped or made non-looping");
			report(Serialization::SerializedCallback::Create<bool>::From("SetLooping", Callback<bool>(&AudioSource::SetLooping, this), serializer));
		}

		// Clip:
		{
			static const auto serializer = Serialization::DefaultSerializer<Reference<Audio::AudioClip>>::Create("Clip", "Audio clip, currently playing");
			report(Serialization::SerializedCallback::Create<Audio::AudioClip*>::From(
				"SetClip", Callback<Audio::AudioClip*>(&AudioSource::SetClip, this), serializer));
		}

		// Play/Pause:
		{
			report(Serialization::SerializedCallback::Create<>::From("Play", Callback<>(&AudioSource::Play, this)));
			report(Serialization::SerializedCallback::Create<>::From("Pause", Callback<>(&AudioSource::Pause, this)));
		}

		// PlayOneShot:
		{
			static const auto serializer = Serialization::DefaultSerializer<Reference<Audio::AudioClip>>::Create("Clip", "Audio clip to play once");
			report(Serialization::SerializedCallback::Create<Audio::AudioClip*>::From(
				"PlayOneShot", Callback<Audio::AudioClip*>(&AudioSource::PlayOneShot, this)));
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
			settings.volume = source->ActiveInHierarchy() ? volume : 0.0f;
			settings.pitch = source->Context()->Updating() ? pitch : 0.0f;
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
			if (sourceComponent->ActiveInHierarchy()) {
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
		if (clip == nullptr || (!ActiveInHierarchy())) return;
		Reference<Audio::AudioSource2D> source = Context()->Audio()->AudioScene()->CreateSource2D(Settings2D(this, Volume(), Pitch()), clip);
		source->SetPriority(Priority());
		source->Play();

		std::unique_lock<std::mutex> lock(m_lock);
		m_oneShotSources.insert(source);
	}

	void AudioSource2D::GetSerializedActions(Callback<Serialization::SerializedCallback> report) {
		AudioSource::GetSerializedActions(report);
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
			settings.volume = component->ActiveInHierarchy() ? volume : 0.0f;
			settings.pitch = component->Context()->Updating() ? pitch : 0.0f;
			return settings;
		}
	}


	AudioSource3D::AudioSource3D(Component* parent, const std::string_view& name, Audio::AudioClip* clip, float volume, float pitch)
		: Component(parent, name)
		, AudioSource(parent->Context()->Audio()->AudioScene()->CreateSource3D(Settings3D(parent, volume, pitch), clip), volume, pitch) {
		m_settings = lastSettings;
	}

	void AudioSource3D::PlayOneShot(Audio::AudioClip* clip) {
		if (clip == nullptr || (!ActiveInHierarchy())) return;
		Reference<Audio::AudioSource3D> source = Context()->Audio()->AudioScene()->CreateSource3D(Settings3D(this, Volume(), Pitch()), clip);
		source->SetPriority(Priority());
		source->Play();

		std::unique_lock<std::mutex> lock(m_lock);
		m_oneShotSources.insert(source);
	}

	void AudioSource3D::GetSerializedActions(Callback<Serialization::SerializedCallback> report) {
		AudioSource::GetSerializedActions(report);
	}

	void AudioSource3D::SynchSource() {
		UpdateSources(this, Source(), Settings3D(this, Volume(), Pitch()), m_settings, m_lock, m_oneShotSources);
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<AudioSource>(const Callback<const Object*>& report) {}
	template<> void TypeIdDetails::GetTypeAttributesOf<AudioSource2D>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<AudioSource2D>(
			"Audio Source 2D", "Jimara/Audio/AudioSource2D", "2D/Non-Posed/Background audio emitter component");
		report(factory);
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<AudioSource3D>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<AudioSource3D>(
			"Audio Source 3D", "Jimara/Audio/AudioSource3D", "3D/Posed/World-Space audio emitter component");
		report(factory);
	}
}
