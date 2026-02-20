#include <Jimara/OS/Logging/StreamLogger.h>
#include <Jimara/OS/Window/Window.h>
#include <Jimara/OS/Window/GLFW/GLFW_Window.h>

int main()
{
	Jimara::Reference<Jimara::OS::StreamLogger> logger = Jimara::Object::Instantiate<Jimara::OS::StreamLogger>();
	Jimara::Reference<Jimara::OS::Window> window = Jimara::OS::Window::Create(logger, "jimara mac window", Jimara::Size2(800, 600), true, Jimara::OS::Window::Backend::GLFW);
		
	if (window == nullptr)
	{
		return 1;
	}
	
	window->WaitTillClosed();
	
	return 0;
}
