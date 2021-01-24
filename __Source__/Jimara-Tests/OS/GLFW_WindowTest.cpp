#include "../GtestHeaders.h"
#include "Core/Stopwatch.h"
#include "OS/Window/Window.h"
#include "OS/Window/GLFW_Window.h"
#include "OS/Logging/StreamLogger.h"
#include <sstream>
#include <iomanip>

namespace Jimara {
	namespace OS {
		namespace {
			// Waits for some amount of time before closing the window, or till it's closed manually after being resized
			inline static void WaitForWindow(Window* window, Size2 initialSize, float waitTimeBeforeResize) {
				Stopwatch stopwatch;
				while (!window->Closed()) {
					if (initialSize != window->FrameBufferSize()) window->WaitTillClosed();
					else {
						std::this_thread::sleep_for(std::chrono::milliseconds(32));
						if (stopwatch.Elapsed() > waitTimeBeforeResize) break;
					}
				}
			}
		}

		// Opens a window and waits for closure
		TEST(GLFW_WindowTest, BasicManipulation) {
			Reference<StreamLogger> logger(Object::Instantiate<StreamLogger>());
			Size2 size(1280, 720);
			Reference<Window> window(Window::Create(
				logger, "Manipulate and close this window to make sure it's properly interactable (will close in 5 seconds unless resized)", size, true, OS::Window::Backend::GLFW));
			EXPECT_EQ(window->Log(), logger);
			EXPECT_TRUE(dynamic_cast<GLFW_Window*>(window.operator->()) != nullptr);
			WaitForWindow(window, size, 5.0f);
		}

		// Opens two windows and waits for one of them to be closed
		TEST(GLFW_WindowTest, TwoWindows) {
			Reference<StreamLogger> logger(Object::Instantiate<StreamLogger>());
			Size2 size(1280, 720);
			Reference<Window> windowA(Window::Create(logger, "Close me to end test (will close in 5 seconds unless resized)", size, true, OS::Window::Backend::GLFW));
			Reference<Window> windowB(Window::Create(
				logger, "Closing me will not change anything (but I should be somewhat small and non-resizable)", Size2(1024, 128), false, OS::Window::Backend::GLFW));
			WaitForWindow(windowA, size, 5.0f);
		}

		// Opens a window and changes it's title in a few seconds
		TEST(GLFW_WindowTest, ChangeName) {
			Reference<StreamLogger> logger(Object::Instantiate<StreamLogger>());
			Size2 size(1280, 720);
			Reference<Window> window(Window::Create(logger, "", size, true, OS::Window::Backend::GLFW));
			Stopwatch stopwatch;
			while (!window->Closed()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(4));
				float timeRemaining = (5.0f - stopwatch.Elapsed());
				if (timeRemaining <= 0.0f) break;
				std::stringstream stream;
				stream << "This window will automatically close in " << std::setprecision(4) << timeRemaining << " seconds..." << std::endl;
				window->SetName(stream.str());
			}
		}
	}
}
