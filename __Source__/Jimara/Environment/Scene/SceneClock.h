#pragma once
#include "Scene.h"
#include "../../Core/Property.h"


namespace Jimara {
#ifndef USE_REFACTORED_SCENE
	namespace Refactor_TMP_Namespace {
#endif
		/// <summary>
		/// Simple clock for various scene contexts
		/// </summary>
		class Scene::Clock : public virtual Object {
		public:
			/// <summary> Delta time after the last update </summary>
			inline float ScaledDeltaTime()const { return m_scaledDeltaTime; }

			/// <summary> 'Raw' delta time without scaling </summary>
			inline float UnscaledDeltaTime()const { return m_unscaledDeltaTime; }

			/// <summary> 
			/// Total scaled time recorded to be elapsed since this clock was created 
			/// Notes:
			///		0. May have some margin of error from floating point inaccuracies;
			///		1. If the game is, for example, pasued from Editor or something like that, time stops flowing;
			///		2. This is just the sum of DeltaTime-s from each update cycle; if you need higher accuracy, use your own clocks and counting methods.
			/// </summary>
			inline float TotalScaledTime()const { return m_totalScaledTime; }

			/// <summary> 
			/// Total unscaled time recorded to be elapsed since this clock was created 
			/// Notes:
			///		0. May have some margin of error from floating point inaccuracies;
			///		1. If the game is, for example, pasued from Editor or something like that, time stops flowing;
			///		2. This is just the sum of DeltaTime-s from each update cycle; if you need higher accuracy, use your own clocks and counting methods.
			/// </summary>
			inline float TotalUnscaledTime()const { return m_totalUnscaledTime; }

			/// <summary> Determines, how fast the time 'flows' </summary>
			inline float TimeScale()const { return m_timeScale; }

			/// <summary>
			/// Sets time scale
			/// </summary>
			/// <param name="timeScale"> Time scale to use from next update cycle onwards </param>
			inline void SetTimeScale(float timeScale) { m_timeScale = timeScale; }

			/// <summary> Determines, how fast the time 'flows' </summary>
			inline Property<float> TimeScale() { return Property<float>(m_timeScale); }

		private:
			// Total sum of m_unscaledDeltaTime per each update cycle
			float m_totalUnscaledTime = 0.0f;

			// Total sum of m_scaledDeltaTime per each update cycle
			float m_totalScaledTime = 0.0f;

			// Value of deltaTime from the last Update() call
			float m_unscaledDeltaTime = 0.0f;

			// Scaled deltaTime from the last Update() call
			float m_scaledDeltaTime = 0.0f;

			// Time scale
			float m_timeScale = 1.0f;

			// Constructor can only be accessed by a limited number of classes
			inline Clock() {}

			// Updates counters
			inline void Update(float deltaTime) {
				m_unscaledDeltaTime = deltaTime;
				m_scaledDeltaTime = m_unscaledDeltaTime * m_timeScale;
				m_totalUnscaledTime += UnscaledDeltaTime();
				m_totalScaledTime += ScaledDeltaTime();
			}

			// Resets all counters
			inline void Reset() {
				m_totalUnscaledTime = m_totalScaledTime = m_unscaledDeltaTime = 0.0f;
				m_timeScale = 1.0f;
			}

			// Theese classes are allowed to modify the clock
			friend class Scene;
			friend class SceneContext;
			friend class GraphicsContext;
			friend class PhysicsContext;
			friend class AudioContext;
		};
#ifndef USE_REFACTORED_SCENE
}
#endif
}

