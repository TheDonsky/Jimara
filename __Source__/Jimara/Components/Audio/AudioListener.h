#pragma once
#include "../Transform.h"


namespace Jimara {
	/// <summary>
	/// Audio listener component
	/// </summary>
	class AudioListener : public virtual Scene::LogicContext::UpdatingComponent {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		/// <param name="volume"> Starting volume </param>
		AudioListener(Component* parent, const std::string_view& name, float volume = 1.0f);

		/// <summary> Listener volume </summary>
		float Volume()const;

		/// <summary>
		/// Sets listener volume
		/// </summary>
		/// <param name="volume"> Listener volume to use </param>
		void SetVolume(float volume);

	protected:
		/// <summary> Invoked each time the logical scene is updated </summary>
		virtual void Update()override;

		/// <summary> Invoked, whenever the component becomes active in herarchy </summary>
		virtual void OnComponentEnabled()override;

		/// <summary> Invoked, whenever the component stops being active in herarchy </summary>
		virtual void OnComponentDisabled()override;

	private:
		// Underlying listener
		Reference<Audio::AudioListener> m_listener;

		// Settings from the last Update()
		Audio::AudioListener::Settings m_lastSettings;

		// Current volume
		float m_volume;

		// Updates settings if changed
		void UpdateSettings();
	};
}
