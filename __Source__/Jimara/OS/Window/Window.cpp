#include "Window.h"


namespace Jimara {
	namespace OS {
		namespace {
			typedef Reference<Window>(*WindowCreateFn)(Logger* logger, const std::string& name, glm::uvec2 size, bool resizable);

			template<typename WindowType>
			inline static Reference<Window> CreateJimaraWindow(Logger* logger, const std::string& name, glm::uvec2 size, bool resizable) {
				return Object::Instantiate<WindowType>(logger, name, size, resizable);
			}

			static const WindowCreateFn* WindowCreateFunctions() {
				static const uint8_t BACKEND_OPTION_COUNT = static_cast<uint8_t>(Window::Backend::BACKEND_OPTION_COUNT);
				static WindowCreateFn createFunctions[BACKEND_OPTION_COUNT];
				for (uint8_t i = 0; i < BACKEND_OPTION_COUNT; i++) createFunctions[i] = nullptr;
				
				createFunctions[static_cast<uint8_t>(Window::Backend::GLFW)] = nullptr;

				return createFunctions;
			}

			static const WindowCreateFn* CREATE_FUNCTIONS = WindowCreateFunctions();
		}

		Reference<Window> Window::Create(Logger* logger, const std::string& name, glm::uvec2 size, bool resizable, Backend backend) {
			WindowCreateFn createFn = ((backend < Backend::BACKEND_OPTION_COUNT) ? CREATE_FUNCTIONS[static_cast<uint8_t>(backend)] : nullptr);
			return createFn != nullptr ? createFn(logger, name, size, resizable) : nullptr;
		}

		Window::~Window() {}

		Logger* Window::Log()const { return m_logger; }

		Window::Window(Logger* logger) : m_logger(logger) {}
	}
}
