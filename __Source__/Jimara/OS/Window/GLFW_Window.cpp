#include "GLFW_Window.h"
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#define GLFW_EXPOSE_NATIVE_WIN32
#elif __APPLE__
#define GLFW_EXPOSE_NATIVE_COCOA
#else
#define GLFW_EXPOSE_NATIVE_GLX
#define GLFW_EXPOSE_NATIVE_X11
#endif
#include <GLFW/glfw3native.h>


namespace Jimara {
	namespace OS {
		namespace {
			std::mutex API_Lock;
			volatile std::atomic<std::size_t> windowCount = 0;
		}

		GLFW_Window::GLFW_Instance::GLFW_Instance(Logger* logger) {
			std::unique_lock<std::mutex> lock(API_Lock);
			if (windowCount <= 0)
				if (glfwInit() != GLFW_TRUE) {
					static const char message[] = "GLFW_Window - Failed to initialize library";
					if (logger != nullptr) logger->Fatal(message);
					throw new std::runtime_error(message);
				}
			windowCount++;
		}

		GLFW_Window::GLFW_Instance::~GLFW_Instance() {
			std::unique_lock<std::mutex> lock(API_Lock);
			windowCount--;
			if (windowCount <= 0)
				glfwTerminate();
		}

		GLFW_Window::GLFW_Window(Logger* logger, const std::string& name, glm::uvec2 size, bool resizable)
			: Window(logger), m_instance(logger)
			, m_windowShouldClose(false), m_activeWindow(NULL), m_window(NULL)
			, m_name(name), m_width(size.x), m_height(size.y), m_resizable(resizable) {
			std::unique_lock<std::mutex> lock(API_Lock);
			volatile bool initError = false;
			std::condition_variable initialized;
			m_windowLoop = std::thread(WindowLoop, this, &initError, &initialized);
			initialized.wait(lock);
			if (initError) {
				static const char message[] = "GLFW_Window - Failed to open the window";
				if (logger != nullptr) logger->Fatal(message);
				throw new std::runtime_error(message);
			}
		}

		GLFW_Window::~GLFW_Window() {
			{
				std::unique_lock<std::mutex> lock(API_Lock);
				m_windowShouldClose = true;
			}
			if (m_windowLoop.joinable())
				m_windowLoop.join();
			if (m_window != NULL) {
				std::unique_lock<std::mutex> lock(API_Lock);
				glfwDestroyWindow(m_window);
				m_window = NULL;
			}
		}

		std::string GLFW_Window::Name()const {
			std::unique_lock<std::mutex> lock(API_Lock);
			return m_name;
		}

		bool GLFW_Window::Closed()const { return m_activeWindow == NULL; }

		void GLFW_Window::WaitTillClosed() {
			std::unique_lock<std::mutex> lock(API_Lock);
			if (m_activeWindow != NULL && m_windowLoop.joinable())
				m_windowLoopFinished.wait(lock);
		}

		glm::uvec2 GLFW_Window::FrameBufferSize()const { return glm::uvec2((glm::uint)m_width, (glm::uint)m_height); }

		Event<Window*>& GLFW_Window::OnUpdate() { return m_onUpdate; }

		Event<Window*>& GLFW_Window::OnSizeChanged() { return m_onSizeChanged; }

#ifdef _WIN32
		HWND GLFW_Window::GetHWND() {
			std::unique_lock<std::mutex> lock(API_Lock);
			return glfwGetWin32Window(m_window);
		}
#elif __APPLE__
		//CAMetalLayer* GLFW_Window::GetMetalLayer() {}
		VkSurfaceKHR GLFW_Window::MakeVulkanSurface(VkInstance instance) {
			std::unique_lock<std::mutex> lock(API_Lock);
			VkSurfaceKHR surface;
			if (glfwCreateWindowSurface(instance, m_window, nullptr, &surface) != VK_SUCCESS)
				throw new std::runtime_error("GLFW_Window - Failed to create vulkan surface");
			return surface;
		}
#else
		xcb_connection_t* GLFW_Window::GetConnectionXCB() {
			std::unique_lock<std::mutex> lock(API_Lock);
			return XGetXCBConnection(glfwGetX11Display());
		}

		xcb_window_t GLFW_Window::GetWindowXCB() {
			std::unique_lock<std::mutex> lock(API_Lock);
			return static_cast<xcb_window_t>(glfwGetX11Window(m_window));
		}
#endif

		void GLFW_Window::WindowLoop(GLFW_Window* self, volatile bool* initError, std::condition_variable* initialized) {
			self->MakeWindow(initError, initialized);
			if (self->m_window != NULL)
				while (self->UpdateWindow());
			self->DestroyWindow();
		}

		void GLFW_Window::MakeWindow(volatile bool* initError, std::condition_variable* initialized) {
			{
				std::unique_lock<std::mutex> lock(API_Lock);
				glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //m_supportOpenGL ? GLFW_OPENGL_API : GLFW_NO_API);
				glfwWindowHint(GLFW_RESIZABLE, m_resizable ? GLFW_TRUE : GLFW_FALSE);
				m_activeWindow = m_window = glfwCreateWindow(m_width, m_height, m_name.c_str(), nullptr, nullptr);
				if (m_window == NULL) (*initError) = true;
				else {
					int w, h;
					glfwGetFramebufferSize(m_window, &w, &h);
					m_width = (w > 0) ? static_cast<uint32_t>(w) : 0;
					m_height = (h > 0) ? static_cast<uint32_t>(h) : 0;
					glfwSetWindowUserPointer(m_window, this);
					glfwSetFramebufferSizeCallback(m_window, OnFramebufferResize);
				}
				initialized->notify_all();
			}
		}

		bool GLFW_Window::UpdateWindow() {
			// Deal with window events:
			{
				std::unique_lock<std::mutex> lock(API_Lock);
				if (m_windowShouldClose) return false;
				else if (glfwWindowShouldClose(m_window)) return false;
				else glfwPollEvents();
			}

			// Run scheduled stuff:
			m_onUpdate(this);

			return true;
		}

		void GLFW_Window::DestroyWindow() {
			std::unique_lock<std::mutex> lock(API_Lock);
			if (m_activeWindow != NULL) {
				glfwHideWindow(m_activeWindow);
				m_activeWindow = NULL;
			}
			m_windowLoopFinished.notify_all();
		}

		void GLFW_Window::OnFramebufferResize(GLFWwindow* window, int width, int height) {
			GLFW_Window* self = reinterpret_cast<GLFW_Window*>(glfwGetWindowUserPointer(window));
			self->m_width = (width > 0) ? static_cast<uint32_t>(width) : 0;
			self->m_height = (height > 0) ? static_cast<uint32_t>(height) : 0;
			self->m_onSizeChanged(self);
		}
	}
}
