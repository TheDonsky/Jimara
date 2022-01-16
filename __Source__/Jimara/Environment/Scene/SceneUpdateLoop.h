#pragma once
#include "Scene.h"

namespace Jimara {
	/// <summary> 
	/// Simple update loop fot a scene, to run scene updates automatically on some external thread
	/// </summary>
	class SceneUpdateLoop : public virtual Object {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="scene"> Scene to update (can not be null) </param>
		/// <param name="startPaused"> If true, the updater will start in 'paused' mode </param>
		SceneUpdateLoop(Scene* scene, bool startPaused = false);

		/// <summary> Virtual destructor </summary>
		virtual ~SceneUpdateLoop();

		/// <summary> If true, update thread will keep updating only graphics and input to prevent most of the simulation from doing anything </summary>
		bool Paused()const;

		/// <summary> Switches simulation to 'paused mode', preventing logic/physics updates from happening </summary>
		void Pause();

		/// <summary> Switches 'paused mode' off, simulating scene normally </summary>
		void Resume();

	private:
		// Scene
		const Reference<Scene> m_scene;

		// Paused state
		std::atomic<bool> m_paused;

		// Destruction flag
		std::atomic<bool> m_destroyed = false;

		// Update thread
		std::thread m_updateThread;

		// No copy-construction and/or copy-assignment allowed
		SceneUpdateLoop(const SceneUpdateLoop&) = delete;
		SceneUpdateLoop(SceneUpdateLoop&&) = delete;
		SceneUpdateLoop& operator=(const SceneUpdateLoop&) = delete;
		SceneUpdateLoop& operator=(SceneUpdateLoop&&) = delete;
	};
}
