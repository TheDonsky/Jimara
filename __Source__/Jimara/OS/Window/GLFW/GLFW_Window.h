#pragma once
#include "../Window.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <condition_variable>
#include <thread>

namespace Jimara {
	namespace OS {
		class GLFW_Window : public Window {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="logger"> Logger </param>
			/// <param name="name"> Name </param>
			/// <param name="size"> Size hint </param>
			/// <param name="resizable"> If true, user will be able to resize the window </param>
			GLFW_Window(Logger* logger, const std::string& name, Size2 size, bool resizable);

			/// <summary> Virtual destructor </summary>
			virtual ~GLFW_Window();

			/// <summary> Window title </summary>
			virtual std::string Name()const override;

			/// <summary> Changes window title </summary>
			virtual void SetName(const std::string& newName) override;

			/// <summary> True, when the user closes the window </summary>
			virtual bool Closed()const override;

			/// <summary> Halts thread till window is closed (keep in mind that this is completely unsafe to invoke from the said window's update thread) </summary>
			virtual void WaitTillClosed() override;

			/// <summary> Current frame buffer dimensions </summary>
			virtual Size2 FrameBufferSize()const override;

			/// <summary> Event invoked on update (every time the api handles window events) </summary>
			virtual Event<Window*>& OnUpdate() override;

			/// <summary> Invoked when the window dimensions change </summary>
			virtual Event<Window*>& OnSizeChanged() override;

			/// <summary> Locks message handling </summary>
			virtual std::mutex& MessageLock() override;

			/// <summary>
			/// Instantiates a compatible Input module
			/// </summary>
			/// <returns> New instance of an Input module </returns>
			virtual Reference<Input> CreateInputModule() override;

#ifdef _WIN32
			/// <summary> Underlying Win32 window handle </summary>
			virtual HWND GetHWND() override;

#elif __APPLE__
			/// <summary> No idea how MAC handles windowing, will revisit this later </summary>
			// virtual CAMetalLayer* GetMetalLayer() override;

			/// <summary> No idea how MAC handles windowing, will revisit this later </summary>
			virtual VkSurfaceKHR MakeVulkanSurface(VkInstance instance) override;
#else
			/// <summary> Underlying xcb_connection_t for X11 window (linux) </summary>
			virtual xcb_connection_t* GetConnectionXCB() override;

			/// <summary> Underlying xcb_window_t for X11 window (linux) </summary>
			virtual xcb_window_t GetWindowXCB() override;
#endif

			/// <summary> Library handle </summary>
			GLFWwindow* Handle()const;

			/// <summary> Invoked, right after the events are polled and before OnUpdate() gets invoked with message lock locked </summary>
			Event<GLFW_Window*>& OnPollEvents();



		private:
			// Basic struct that loads/unloads the library
			struct GLFW_Instance {
				// Loads library if not loaded
				GLFW_Instance(Logger* logger);

				// Unloads library if no windows exist
				~GLFW_Instance();
			} m_instance;

			// Window thread
			std::thread m_windowLoop;

			// Some condition variable for synchronizing with m_windowLoop termination
			std::condition_variable m_windowLoopFinished;

			// True, if window should close
			volatile bool m_windowShouldClose;

			// True, if window name changed
			volatile bool m_nameChanged;

			// Underlying window (nullptr after the window gets closed)
			GLFWwindow* m_activeWindow;

			// Underlying window (exists before the object gets destroyed)
			GLFWwindow* m_window;

			// Name of the window
			std::string m_name;

			// Window dimensions
			std::atomic<uint32_t> m_width, m_height;

			// True if resizable
			volatile bool m_resizable;

			// OnUpdate event's instance
			EventInstance<Window*> m_onUpdate;

			// Invoked, right after the events are polled and before OnUpdate() gets invoked
			EventInstance<GLFW_Window*> m_onPollEvents;

			// OnSizeChanged event's instance
			EventInstance<Window*> m_onSizeChanged;

			// Locks message handling
			std::mutex m_messageLock;

			// Window thread function
			static void WindowLoop(GLFW_Window* self, volatile bool* initError, std::condition_variable* initialized);

			// Creates the window
			void MakeWindow(volatile bool* initError, std::condition_variable* initialized);

			// Processes window evets and updates OnUpdate listeners
			bool UpdateWindow();

			// Wipes all the memories of the window ever existing
			void DestroyWindow();

			// Invoked, whenever the frame buffer gets resized
			static void OnFramebufferResize(GLFWwindow* window, int width, int height);
		};
	}
}
