#pragma once
#include "../Transform.h"


namespace Jimara {
	/// <summary> This will make sure, AudioListener is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::AudioListener);

	/// <summary>
	/// Audio listener component
	/// </summary>
	class JIMARA_API AudioListener : public virtual Scene::LogicContext::UpdatingComponent {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Component name </param>
		/// <param name="volume"> Starting volume </param>
		AudioListener(Component* parent, const std::string_view& name = "AudioListener", float volume = 1.0f);

		/// <summary> Listener volume </summary>
		float Volume()const;

		/// <summary>
		/// Sets listener volume
		/// </summary>
		/// <param name="volume"> Listener volume to use </param>
		void SetVolume(float volume);

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

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

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<AudioListener>(const Callback<TypeId>& report) { report(TypeId::Of<Scene::LogicContext::UpdatingComponent>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<AudioListener>(const Callback<const Object*>& report);
}
