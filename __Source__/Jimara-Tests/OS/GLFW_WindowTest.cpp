#include "../GtestHeaders.h"
#include "OS/Window/Window.h"
#include "OS/Window/GLFW_Window.h"
#include "OS/Logging/StreamLogger.h"

namespace Jimara {
	namespace OS {
		// Opens a window and waits for closure
		TEST(GLFW_WindowTest, BasicManipulation) {
			Reference<StreamLogger> logger(Object::Instantiate<StreamLogger>());
			Reference<Window> window(Window::Create(
				logger, "Manipulate and close this window to make sure it's properly interactable", glm::uvec2(1280, 720), true, OS::Window::Backend::GLFW));
			EXPECT_EQ(window->Log(), logger);
			window->WaitTillClosed();
		}

		// Opens two windows and waits for one of them to be closed
		TEST(GLFW_WindowTest, TwoWindows) {
			Reference<StreamLogger> logger(Object::Instantiate<StreamLogger>());
			Reference<Window> windowA(Window::Create(logger, "Close me to end test", glm::uvec2(1280, 720), true, OS::Window::Backend::GLFW));
			Reference<Window> windowB(Window::Create(
				logger, "Closing me will not change anything (but I should be somewhat small and non-resizable)", glm::uvec2(1024, 128), false, OS::Window::Backend::GLFW));
			windowA->WaitTillClosed();
		}
	}
}
