#pragma once
#include "../Scene.h"




namespace Jimara {
namespace Refactor_TMP_Namespace {
	class Scene::AudioContext : public virtual Object {
	public:
		inline Audio::AudioScene* AudioScene()const { return m_scene; }

	private:
		const Reference<Audio::AudioScene> m_scene;

		inline AudioContext(Audio::AudioScene* scene)
			: m_scene(scene) {}

		static Reference<AudioContext> Create(Audio::AudioDevice* device, OS::Logger* logger);

		friend class Scene;
	};
}
}
