#include "SceneUpdateLoop.h"
#include "../../Core/Stopwatch.h"

namespace Jimara {
	SceneUpdateLoop::SceneUpdateLoop(Scene* scene, bool startPaused) 
		: m_scene(scene), m_paused(startPaused) {
		assert(m_scene != nullptr);
		m_updateThread = std::thread([](SceneUpdateLoop* self) {
			Stopwatch stopwatch;
			while (!self->m_destroyed.load()) {
				std::this_thread::yield();
				if (stopwatch.Elapsed() < 0.000001f) continue;
				float deltaTime = stopwatch.Reset();
				if (self->m_paused.load())
					self->m_scene->SynchAndRender(deltaTime);
				else self->m_scene->Update(deltaTime);
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
