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


	namespace {
		class AudioSourceSerializer : public virtual Serialization::SerializerList::From<AudioSource> {
		public:
			inline AudioSourceSerializer() : ItemSerializer("Jimara/Audio/AudioSource", "Audio Source") {}

			inline static const AudioSourceSerializer* Instance() {
				static const AudioSourceSerializer instance;
				return &instance;
			}

			virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AudioSource* target)const final override {
				TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>()->GetFields(recordElement, target);
				typedef Serialization::ItemSerializer::Of<AudioSource> FieldSerializer;
				{
					static const Reference<const FieldSerializer> serializer = Serialization::FloatSerializer::For<AudioSource>(
						"Volume", "Source volume",
						[](AudioSource* source) -> float { return source->Volume(); },
						[](const float& value, AudioSource* source) { source->SetVolume(value); });
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const FieldSerializer> serializer = Serialization::FloatSerializer::For<AudioSource>(
						"Pitch", "Playback speed",
						[](AudioSource* source) -> float { return source->Pitch(); },
						[](const float& value, AudioSource* source) { source->SetPitch(value); });
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const FieldSerializer> serializer = Serialization::IntSerializer::For<AudioSource>(
						"Priority", 
						"Source priority (in case there are some limitations about the number of actively playing sounds on the underlying hardware, "
						"higherst priority ones will be heared)",
						[](AudioSource* source) -> int { return source->Priority(); },
						[](const int& value, AudioSource* source) { source->SetPriority(value); });
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const FieldSerializer> serializer = Serialization::BoolSerializer::For<AudioSource>(
						"Looping", "If true, playback will keep looping untill paused/stopped or made non-looping",
						[](AudioSource* source) -> bool { return source->Looping(); },
						[](const bool& value, AudioSource* source) { source->SetLooping(value); });
					recordElement(serializer->Serialize(target));
				}
				{
					typedef Audio::AudioClip* (*GetClipFn)(AudioSource*);
					typedef void (*SetClipFn)(Audio::AudioClip* const&, AudioSource*);
					static const Reference<const FieldSerializer> serializer = Serialization::ValueSerializer<Audio::AudioClip*>::For<AudioSource>(
						"Looping", "If true, playback will keep looping untill paused/stopped or made non-looping",
						Function((GetClipFn)[](AudioSource* source) -> Audio::AudioClip* { return source->Clip(); }),
						Callback((SetClipFn)[](Audio::AudioClip* const& value, AudioSource* source) { source->SetClip(value); }));
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const FieldSerializer> serializer = Serialization::BoolSerializer::For<AudioSource>(
						"Playing", "True, while playing",
						[](AudioSource* source) -> bool { return source->Playing(); },
						[](const bool& value, AudioSource* source) {
							if (value) source->Play();
							else source->Stop();
						});
					recordElement(serializer->Serialize(target));
				}
			}
		};

		class AudioSourceSerializer2D : public virtual ComponentSerializer::Of<AudioSource2D> {
		public:
			inline AudioSourceSerializer2D() : ItemSerializer("Jimara/Audio/AudioSource2D", "Audio Source 2D") {}

			virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AudioSource2D* target)const final override {
				AudioSourceSerializer::Instance()->GetFields(recordElement, target);
			}
		};

		class AudioSourceSerializer3D : public virtual ComponentSerializer::Of<AudioSource3D> {
		public:
			inline AudioSourceSerializer3D() : ItemSerializer("Jimara/Audio/AudioSource3D", "Audio Source 3D") {}

			virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AudioSource3D* target)const final override {
				AudioSourceSerializer::Instance()->GetFields(recordElement, target);
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<AudioSource>(const Callback<const Object*>& report) {
		report(AudioSourceSerializer::Instance());
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<AudioSource2D>(const Callback<const Object*>& report) {
		static const AudioSourceSerializer2D instance;
		report(&instance);
	}
	template<> void TypeIdDetails::GetTypeAttributesOf<AudioSource3D>(const Callback<const Object*>& report) {
		static const AudioSourceSerializer3D instance;
		report(&instance);
	}
}
