#pragma once
#include "../Scene.h"
#include "../../../Audio/AudioDevice.h"


namespace Jimara {
	/// <summary>
	/// Scene sub-context for Audio related routines and storage
	/// </summary>
	class Scene::AudioContext : public virtual Object {
	public:
		/// <summary> Direct access to the audio toolbox scene (for now) </summary>
		inline Audio::AudioScene* AudioScene()const { return m_scene; }

	private:
		// Audio scene
		const Reference<Audio::AudioScene> m_scene;

		// Constructor
		inline AudioContext(Audio::AudioScene* scene) 
			: m_scene(scene) {}

		// Creates the context
		static Reference<AudioContext> Create(Audio::AudioDevice* device, OS::Logger* logger);

		// Only the Scene itself can create a context 
		// (I know, I know... 'friend' classes are supposed to be 'bad', but this object fully belongs to the scene and has no use beyond)
		friend class Scene;
	};
}
