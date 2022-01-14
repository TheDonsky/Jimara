#pragma once
#include "Environment/Scene/Scene.h"
#include "Core/Stopwatch.h"
#include "Core/Synch/Semaphore.h"
#include <thread>
#include <queue>

namespace Jimara {
	namespace Test {
		class TestEnvironment : public virtual Object {
		public:
			TestEnvironment(const std::string_view& windowTitle, float windowTimeout = 5.0f);

			virtual ~TestEnvironment();

			void SetWindowName(const std::string_view& newName);

			Component* RootObject()const;

			void ExecuteOnUpdate(const Callback<Object*>& callback, Object* userData = nullptr);

			void ExecuteOnUpdateNow(const Callback<Object*>& callback, Object* userData = nullptr);

			template<typename CallbackType>
			void ExecuteOnUpdateNow(const CallbackType& callback) {
				void(*invoke)(const CallbackType*, Object*) = [](const CallbackType* call, Object*) { (*call)(); };
				ExecuteOnUpdateNow(Callback<Object*>(invoke, &callback), nullptr);
			}


		private:
			const float m_windowTimeout;
			std::mutex m_windowNameLock;
			std::string m_baseWindowName;
			std::string m_windowName;
			Reference<OS::Window> m_window;
			Reference<Scene> m_scene;
			Reference<Graphics::RenderEngine> m_renderEngine;
			Reference<Graphics::ImageRenderer> m_renderer;
			std::atomic<bool> m_windowResized = false;

			struct {
				Stopwatch deltaTime;
				Stopwatch timeSinceRefresh;
				std::atomic<float> smoothDeltaTime = 0.1f;
			} m_fpsCounter;

			struct {
				std::thread thread;
				Stopwatch stopwatch;
				std::atomic<bool> quit = false;
			} m_asynchUpdate;

			void OnWindowUpdate(OS::Window*);
			void OnWindowResized(OS::Window*);
			void AsynchUpdateThread();
		};
	}
}
