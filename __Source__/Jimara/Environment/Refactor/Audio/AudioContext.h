#pragma once
#include "../Scene.h"




namespace Jimara {
namespace Refactor_TMP_Namespace {
	class Scene::AudioContext : public virtual Object {
	public:

	private:
		const Reference<Audio::AudioDevice> m_device;

		inline AudioContext(Audio::AudioDevice* device) : m_device(device) {}

		inline static Reference<AudioContext> Create(Audio::AudioDevice* device) {
			Reference<AudioContext> context = new AudioContext(device);
			context->ReleaseRef();
			return context;
		}

		friend class Scene;
	};
}
}
