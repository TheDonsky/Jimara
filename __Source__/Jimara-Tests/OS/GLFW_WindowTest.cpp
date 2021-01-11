#include "../GtestHeaders.h"
#include "OS/Window/Window.h"
#include "OS/Window/GLFW_Window.h"
#include "OS/Logging/StreamLogger.h"

namespace Jimara {
	namespace OS {
		TEST(GLFW_WindowTest, BasicManipulation) {
			Reference<StreamLogger> logger(Object::Instantiate<StreamLogger>());
			Reference<Window> window(Window::Create(logger, "Manipulate and close this window to make sure it's properly interactable"));
			EXPECT_EQ(window->Log(), logger);
			window->WaitTillClosed();
		}
	}
}
