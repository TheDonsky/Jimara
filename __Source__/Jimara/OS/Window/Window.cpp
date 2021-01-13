#include "Window.h"
#include "GLFW_Window.h"


namespace Jimara {
	namespace OS {
		namespace {
			typedef Reference<Window>(*WindowCreateFn)(Logger* logger, const std::string& name, glm::uvec2 size, bool resizable);

			template<typename WindowType>
			inline static Reference<Window> CreateJimaraWindow(Logger* logger, const std::string& name, glm::uvec2 size, bool resizable) {
				return Object::Instantiate<WindowType>(logger, name, size, resizable);
			}

			static const WindowCreateFn WindowCreateFunction(Window::Backend backend) {
				static WindowCreateFn DEFAULT = [](Logger*, const std::string&, glm::uvec2, bool) -> Reference<Window> { return Reference<Window>(nullptr); };
				static const WindowCreateFn* CREATE_FUNCTIONS = []() {
					static const uint8_t BACKEND_OPTION_COUNT = static_cast<uint8_t>(Window::Backend::BACKEND_OPTION_COUNT);
					static WindowCreateFn createFunctions[BACKEND_OPTION_COUNT];
					for (uint8_t i = 0; i < BACKEND_OPTION_COUNT; i++) createFunctions[i] = DEFAULT;

					createFunctions[static_cast<uint8_t>(Window::Backend::GLFW)] = CreateJimaraWindow<GLFW_Window>;

					return createFunctions;
				}();
				return backend < Window::Backend::BACKEND_OPTION_COUNT ? CREATE_FUNCTIONS[static_cast<uint8_t>(backend)] : DEFAULT;
			}
		}

		Reference<Window> Window::Create(Logger* logger, const std::string& name, glm::uvec2 size, bool resizable, Backend backend) {
			return WindowCreateFunction(backend)(logger, name, size, resizable);
		}

		Window::~Window() {}

		Logger* Window::Log()const { return m_logger; }

		Window::Window(Logger* logger) : m_logger(logger) {}
	}
}
