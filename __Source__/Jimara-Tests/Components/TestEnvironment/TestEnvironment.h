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

			void ExecuteOnUpdate(const Callback<TestEnvironment*>& callback);

			void ExecuteOnUpdateNow(const Callback<TestEnvironment*>& callback);

			template<typename CallbackType>
			void ExecuteOnUpdateNow(const CallbackType& callback) {
				static std::mutex cs;
				static const CallbackType* callbackRef = nullptr;
				static Callback<TestEnvironment*> invoke = Callback<TestEnvironment*>([](TestEnvironment*) { (*callbackRef)(); });
				callbackRef = &callback;
				std::unique_lock<std::mutex> lock(cs);
				ExecuteOnUpdateNow(invoke);
			}


		private:
			const float m_windowTimeout;
			std::mutex m_windowNameLock;
			std::string m_baseWindowName;
			std::string m_windowName;
			Reference<OS::Window> m_window;
			Reference<OS::Input> m_input;
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
				std::mutex updateQueueLock;
				std::queue<Callback<TestEnvironment*>> updateQueue[2];
				std::atomic<uint8_t> updateQueueBackBufferId = 0;
				std::atomic<bool> quit = false;
			} m_asynchUpdate;

			void OnWindowUpdate(OS::Window*);
			void OnWindowResized(OS::Window*);
			void AsynchUpdateThread();
		};
	}
}
