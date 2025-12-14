#include "GLFW_Window.h"
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_WIN32
#elif __APPLE__
#define GLFW_EXPOSE_NATIVE_COCOA
#else
#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>
#include <shared_mutex>
#include "GLFW_Input.h"
#include "../../../Core/Stopwatch.h"
#include "../../../Core/Synch/Semaphore.h"
#include "../../../Core/Collections/ThreadBlock.h"


namespace Jimara {
	namespace OS {
		namespace {
			std::shared_mutex API_Lock;
			volatile std::atomic<std::size_t> windowCount = 0;
			static Reference<Logger> mainInstanceLogger;

			class InstanceThread : public virtual Object {
			private:
				ThreadBlock m_block;

				struct Input {
					const Callback<Object*>* callback;
					Object* object;
				};

			public:
				inline void Execute(const Callback<Object*>& callback, Object* object) {
					Input input = { &callback, object };
					m_block.Execute(1, &input, Callback<ThreadBlock::ThreadInfo, void*>([](ThreadBlock::ThreadInfo, void* inp) {
						Input& i = *reinterpret_cast<Input*>(inp);
						(*i.callback)(i.object);
						}));
				}
			};

			static Reference<InstanceThread> instanceThread = nullptr;
		}

		GLFW_Window::GLFW_Instance::GLFW_Instance(Logger* logger) {
			std::unique_lock<std::shared_mutex> lock(API_Lock);
			if (windowCount <= 0) {
				instanceThread = Object::Instantiate<InstanceThread>();
				instanceThread->Execute(Callback<Object*>([](Object* loggerRef) {
					Logger* logger = dynamic_cast<Logger*>(loggerRef);
#ifndef _WIN32
					glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
					auto initErr = glfwInit();
					if (initErr != GLFW_TRUE) {
						logger->Fatal("GLFW_Window - Failed to initialize library: ", initErr);
					}
					else {
						mainInstanceLogger = logger;
						glfwSetJoystickCallback([](int jid, int event) {
							mainInstanceLogger->Info("Joystic ", jid, (event == GLFW_CONNECTED) ? " CONNECTED" : " DISCONNECTED");
							});
					}
					}), logger);
			}
			windowCount++;
		}

		GLFW_Window::GLFW_Instance::~GLFW_Instance() {
			std::unique_lock<std::shared_mutex> lock(API_Lock);
			windowCount--;
			if (windowCount <= 0) {
				instanceThread->Execute(Callback<Object*>([](Object*) {
					glfwTerminate();
					mainInstanceLogger = nullptr;
					}), nullptr);
				instanceThread = nullptr;
			}
		}

		GLFW_Window::GLFW_Window(Logger* logger, const std::string& name, Size2 size, bool resizable)
			: Window(logger), m_instance(logger)
			, m_windowShouldClose(false), m_nameChanged(false)
			, m_activeWindow(NULL), m_window(NULL)
			, m_name(name)
			, m_isFullscreen(false), m_fullscreenStateChanged(false)
			, m_width(size.x), m_height(size.y)
			, m_resizable(resizable) {
			volatile bool initError = false;
			{
				std::unique_lock<std::mutex> lock(m_windowLoopLock);
				m_windowLoop = std::thread(WindowLoop, this, &initError);
				m_windowLoopSignal.wait(lock);
			}
			if (initError) {
				static const char message[] = "GLFW_Window - Failed to open the window";
				if (logger != nullptr) logger->Fatal(message);
				throw new std::runtime_error(message);
			}
		}

		GLFW_Window::~GLFW_Window() {
			{
				std::unique_lock<std::shared_mutex> lock(API_Lock);
				m_windowShouldClose = true;
			}
			if (m_windowLoop.joinable())
				m_windowLoop.join();
			if (m_window != NULL) {
				std::unique_lock<std::shared_mutex> lock(API_Lock);
				instanceThread->Execute(Callback<Object*>([](Object* selfRef) {
					GLFW_Window* self = dynamic_cast<GLFW_Window*>(selfRef);
					glfwDestroyWindow(self->m_window);
					self->m_window = NULL;
					}), this);
			}
		}

		std::string GLFW_Window::Name()const {
			std::unique_lock<std::mutex> lock(m_parameterLock);
			std::string name = m_name;
			return name;
		}

		void GLFW_Window::SetName(const std::string& newName) {
			std::unique_lock<std::mutex> lock(m_parameterLock);
			m_name = newName;
			m_nameChanged = true;
		}

		bool GLFW_Window::IsFullscreen()const {
			std::unique_lock<std::mutex> lock(m_parameterLock);
			return m_isFullscreen;
		}

		void GLFW_Window::SetFullscreen(bool fullscreen) {
			std::unique_lock<std::mutex> lock(m_parameterLock);
			if (m_isFullscreen == fullscreen)
				return;
			m_isFullscreen = fullscreen;
			m_fullscreenStateChanged = true;
		}

		bool GLFW_Window::Closed()const { return m_activeWindow == NULL; }

		void GLFW_Window::WaitTillClosed() {
			std::unique_lock<std::mutex> lock(m_windowLoopLock);
			if (m_activeWindow != NULL && m_windowLoop.joinable())
				m_windowLoopSignal.wait(lock);
		}

		Size2 GLFW_Window::FrameBufferSize()const { return Size2((uint32_t)m_width, (uint32_t)m_height); }

		bool GLFW_Window::Focused()const { return m_focused.load(); }

		Event<Window*>& GLFW_Window::OnUpdate() { return m_onUpdate; }

		Event<Window*>& GLFW_Window::OnSizeChanged() { return m_onSizeChanged; }

		std::shared_mutex& GLFW_Window::MessageLock() { return API_Lock; }

		Reference<Input> GLFW_Window::CreateInputModule() { return Object::Instantiate<GLFW_Input>(this); }

		Vector2 GLFW_Window::CursorPosition()const {
			Vector2 res = {};
			{
				std::unique_lock<std::mutex> lock(m_parameterLock);
				if (m_requestedCursorPosition.has_value())
					res = m_requestedCursorPosition.value();
				else res = m_currentCursorPosition;
			}
			return res;
		}

		void GLFW_Window::SetCursorPosition(Vector2 position) {
			std::unique_lock<std::mutex> lock(m_parameterLock);
			m_requestedCursorPosition = position;
		}

#ifdef _WIN32
		HWND GLFW_Window::GetHWND() {
			std::unique_lock<std::shared_mutex> lock(API_Lock);
			return glfwGetWin32Window(m_window);
		}
#else // __APPLE__
		void GLFW_Window::MakeVulkanSurface(void* vkInstancePtr, void* vkSurfaceKHRPtr) {
			std::unique_lock<std::shared_mutex> lock(API_Lock);
			VkInstance instance = *((VkInstance*)vkInstancePtr);
			VkSurfaceKHR* surface = (VkSurfaceKHR*)vkSurfaceKHRPtr;
			if (glfwCreateWindowSurface(instance, m_window, nullptr, surface) != VK_SUCCESS)
				mainInstanceLogger->Fatal("GLFW_Window - Failed to create vulkan surface");
		}
#endif
#ifdef EXPOSE_LINUX_WINDOW_DETAILS
		Window::WindowManager GLFW_Window::GetWindowManager()const {
#ifdef JIMARA_PLATFORM_SUPPORTS_WAYLAND
			std::unique_lock<std::shared_mutex> lock(API_Lock);
			return (glfwGetWaylandWindow(m_window) == nullptr) ? WindowManager::X11 : WindowManager::WAYLAND;
#else
			return WindowManager::X11;
#endif
		}

		xcb_connection_t* GLFW_Window::GetConnectionXCB() {
#ifndef JIMARA_PLATFORM_SUPPORTS_WAYLAND
			std::unique_lock<std::shared_mutex> lock(API_Lock);
			return XGetXCBConnection(glfwGetX11Display());
#else
			return nullptr;
#endif
		}

		xcb_window_t GLFW_Window::GetWindowXCB() {
#ifndef JIMARA_PLATFORM_SUPPORTS_WAYLAND
			std::unique_lock<std::shared_mutex> lock(API_Lock);
			return static_cast<xcb_window_t>(glfwGetX11Window(m_window));
#else
			return {};
#endif
		}

		void* GLFW_Window::GetWaylandDisplay() {
#ifdef JIMARA_PLATFORM_SUPPORTS_WAYLAND
			std::unique_lock<std::shared_mutex> lock(API_Lock);
			return glfwGetWaylandDisplay();
#else
			return nullptr;
#endif
		}

		void* GLFW_Window::GetWaylandSurface() {
#ifdef JIMARA_PLATFORM_SUPPORTS_WAYLAND
			std::unique_lock<std::shared_mutex> lock(API_Lock);
			return glfwGetWaylandWindow(m_window);
#else
			return nullptr;
#endif
		}
#endif

		GLFWwindow* GLFW_Window::Handle()const { return m_window; }

		std::shared_mutex& GLFW_Window::APILock() { return API_Lock; }

		void GLFW_Window::ExecuteOnEventThread(const Callback<>& callback)const {
			typedef void(*CallType)(const Callback<>*, Object*);
			instanceThread->Execute(Callback<Object*>((CallType)[](const Callback<>* callback, Object*) { (*callback)(); }, &callback), nullptr);
		}

		Event<GLFW_Window*>& GLFW_Window::OnPollEvents() { return m_onPollEvents; }

		void GLFW_Window::WindowLoop(GLFW_Window* self, volatile bool* initError) {
			self->MakeWindow(initError);
			if (self->m_window != NULL)
				while (self->UpdateWindow());
			self->DestroyWindow();
		}

		void GLFW_Window::MakeWindow(volatile bool* initError) {
			std::unique_lock<std::shared_mutex> lock(API_Lock);
			static volatile bool* initialisationError = nullptr;
			initialisationError = initError;
			instanceThread->Execute(Callback<Object*>([](Object* selfRef) {
				GLFW_Window* self = dynamic_cast<GLFW_Window*>(selfRef);
				std::unique_lock<std::mutex> lock(self->m_parameterLock);
				glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //m_supportOpenGL ? GLFW_OPENGL_API : GLFW_NO_API);
				glfwWindowHint(GLFW_RESIZABLE, self->m_resizable ? GLFW_TRUE : GLFW_FALSE);
				self->m_activeWindow = self->m_window = glfwCreateWindow(self->m_width, self->m_height, self->m_name.c_str(), nullptr, nullptr);
				if (self->m_window == NULL) (*initialisationError) = true;
				else {
					int w, h;
					glfwGetFramebufferSize(self->m_window, &w, &h);
					self->m_width = (w > 0) ? static_cast<uint32_t>(w) : 0;
					self->m_height = (h > 0) ? static_cast<uint32_t>(h) : 0;
					glfwSetWindowUserPointer(self->m_window, self);
					glfwSetFramebufferSizeCallback(self->m_window, OnFramebufferResize);
				}
				}), this);
			std::unique_lock<std::mutex> loopLock(m_windowLoopLock);
			m_windowLoopSignal.notify_all();
		}

		bool GLFW_Window::UpdateWindow() {
			// Deal with window events:
			{
				std::unique_lock<std::shared_mutex> lock(API_Lock);
				static volatile bool exit = false;
				exit = false;

				instanceThread->Execute(Callback<Object*>([](Object* selfRef) {
					GLFW_Window* self = dynamic_cast<GLFW_Window*>(selfRef);
					std::unique_lock<std::mutex> lock(self->m_parameterLock);

					if (self->m_nameChanged) {
						glfwSetWindowTitle(self->m_window, self->m_name.c_str());
						self->m_nameChanged = false;
					}

					if (self->m_fullscreenStateChanged) {
						if (self->m_isFullscreen && glfwGetWindowMonitor(self->m_window) == NULL) {
							{
								self->m_preFullscreenWidth = int(self->m_width.load());
								self->m_preFullscreenHeight = int(self->m_height.load());
								int xPos = 0, yPos = 0;
								glfwGetWindowPos(self->m_window, &xPos, &yPos);
								self->m_preFullscreenPos_x = xPos;
								self->m_preFullscreenPos_y = yPos;
							}
							int monitorCount = 0u;
							GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
							if (monitorCount > 0) {
								GLFWmonitor* monitor = monitors[0];
								const GLFWvidmode* mode = glfwGetVideoMode(monitor);
								glfwSetWindowMonitor(self->m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
							}
							else self->m_isFullscreen = false;
						}
						if (!self->m_isFullscreen) {
							glfwSetWindowMonitor(self->m_window, NULL,
								self->m_preFullscreenPos_x, self->m_preFullscreenPos_y,
								self->m_preFullscreenWidth, self->m_preFullscreenHeight, 0);
						}
						self->m_fullscreenStateChanged = false;
					}

					self->m_focused = glfwGetWindowAttrib(self->m_window, GLFW_FOCUSED) == GLFW_TRUE;

					if (self->m_requestedCursorPosition.has_value()) {
						glfwSetCursorPos(self->m_window, self->m_requestedCursorPosition.value().x, self->m_requestedCursorPosition.value().y);
						self->m_currentCursorPosition = self->m_requestedCursorPosition.value();
						self->m_requestedCursorPosition = std::nullopt;
					}
					else {
						double posX = {}, posY = {};
						glfwGetCursorPos(self->m_window, &posX, &posY);
						self->m_currentCursorPosition = Vector2(static_cast<float>(posX), static_cast<float>(posY));
					}

					if (self->m_windowShouldClose) exit = true;
					else {
						glfwPollEvents();
						if (glfwWindowShouldClose(self->m_window)) exit = true;
					}
					}), this);
				if (exit) return false;

				m_onPollEvents(this);
			}

			// Run scheduled stuff:
			m_onUpdate(this);
			std::this_thread::yield();

			return true;
		}

		void GLFW_Window::DestroyWindow() {
			{
				std::unique_lock<std::shared_mutex> lock(API_Lock);
				if (m_activeWindow != NULL)
					instanceThread->Execute(Callback<Object*>([](Object* selfRef) {
					GLFW_Window* self = dynamic_cast<GLFW_Window*>(selfRef);
					glfwHideWindow(self->m_activeWindow);
					self->m_activeWindow = NULL;
						}), this);
			}
			std::unique_lock<std::mutex> loopLock(m_windowLoopLock);
			m_windowLoopSignal.notify_all();
		}

		void GLFW_Window::OnFramebufferResize(GLFWwindow* window, int width, int height) {
			GLFW_Window* self = reinterpret_cast<GLFW_Window*>(glfwGetWindowUserPointer(window));
			self->m_width = (width > 0) ? static_cast<uint32_t>(width) : 0;
			self->m_height = (height > 0) ? static_cast<uint32_t>(height) : 0;
			self->m_onSizeChanged(self);
		}
	}
}
