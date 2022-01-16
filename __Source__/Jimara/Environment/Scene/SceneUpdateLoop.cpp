#include "SceneUpdateLoop.h"
#include "../../Core/Stopwatch.h"

namespace Jimara {
	SceneUpdateLoop::SceneUpdateLoop(Scene* scene, bool startPaused) 
		: m_scene(scene), m_paused(startPaused) {
		assert(m_scene != nullptr);
		m_updateThread = std::thread([](SceneUpdateLoop* self) {
			Stopwatch stopwatch;
			while (!self->m_destroyed.load()) {
				if (stopwatch.Elapsed() < 0.000001f) continue;
				float deltaTime = stopwatch.Reset();
				self->m_scene->Update(self->m_paused.load() ? 0.0f : deltaTime);
				std::this_thread::yield();
			}
			}, this);
	}

	SceneUpdateLoop::~SceneUpdateLoop() {
		m_destroyed = true;
		m_updateThread.join();
	}

	bool SceneUpdateLoop::Paused()const { return m_paused.load(); }

	void SceneUpdateLoop::Pause() { m_paused = true; }

	void SceneUpdateLoop::Resume() { m_paused = false; }
}
