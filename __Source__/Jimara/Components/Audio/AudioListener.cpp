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
			settings.volume = volume;
			return settings;
		}
	}

	float AudioListener::Volume()const { return m_volume; }

	void AudioListener::SetVolume(float volume) { m_volume = volume; }

	AudioListener::AudioListener(Component* parent, const std::string_view& name, float volume) 
		: Component(parent, name), m_volume(volume) {
		m_lastSettings = GetSettings(this, m_volume);
		m_listener = Context()->AudioScene()->CreateListener(m_lastSettings);
	}

	void AudioListener::Update() {
		Audio::AudioListener::Settings settings = GetSettings(this, m_volume);
		if (m_lastSettings == settings) return;
		m_lastSettings = settings;
		m_listener->Update(settings);
	}
}
