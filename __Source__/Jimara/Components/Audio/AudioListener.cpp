#include "AudioListener.h"
#include "../Physics/Rigidbody.h"


namespace Jimara {
	namespace {
		inline static Audio::AudioListener::Settings GetSettings(const Component* component, float volume) {
			const Transform* transform = component->GetTransfrom();
			const Rigidbody* rigidbody = component->GetComponentInParents<Rigidbody>();
			Audio::AudioListener::Settings settings;
			if (transform != nullptr) {
				settings.pose = transform->WorldRotationMatrix();
				settings.pose[3] = Vector4(transform->WorldPosition(), 1.0f);
			}
			if (rigidbody != nullptr) settings.velocity = rigidbody->Velocity();
			settings.volume = component->ActiveInHeirarchy() ? volume : 0.0f;
			return settings;
		}
	}

	float AudioListener::Volume()const { return m_volume; }

	void AudioListener::SetVolume(float volume) { m_volume = volume; }

	AudioListener::AudioListener(Component* parent, const std::string_view& name, float volume) 
		: Component(parent, name), m_volume(volume) {
		m_lastSettings = GetSettings(this, m_volume);
		m_listener = Context()->Audio()->AudioScene()->CreateListener(m_lastSettings);
	}

	void AudioListener::Update() {
		UpdateSettings();
	}

	void AudioListener::OnComponentEnabled() {
		UpdateSettings();
	}

	void AudioListener::OnComponentDisabled() {
		UpdateSettings();
	}

	void AudioListener::UpdateSettings() {
		Audio::AudioListener::Settings settings = GetSettings(this, m_volume);
		if (m_lastSettings == settings) return;
		m_lastSettings = settings;
		m_listener->Update(settings);
	}

	namespace {
		class AudioListenerSerializer : public virtual ComponentSerializer::Of<AudioListener> {
		public:
			inline AudioListenerSerializer() : ItemSerializer("Jimara/Audio/AudioListener", "Audio Listener") {}

			inline static const AudioListenerSerializer* Instance() {
				static const AudioListenerSerializer instance;
				return &instance;
			}

			virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, AudioListener* target)const final override {
				TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>()->GetFields(recordElement, target);
				{
					static const Reference<const FieldSerializer> serializer = Serialization::FloatSerializer::For<AudioListener>(
						"Volume", "Listener volume",
						[](AudioListener* listener) -> float { return listener->Volume(); },
						[](const float& value, AudioListener* listener) { return listener->SetVolume(value); });
					recordElement(serializer->Serialize(target));
				}
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<AudioListener>(const Callback<const Object*>& report) {
		report(AudioListenerSerializer::Instance());
	}
}
