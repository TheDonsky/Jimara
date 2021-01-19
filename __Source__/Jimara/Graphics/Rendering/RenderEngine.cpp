#include "RenderEngine.h"

namespace Jimara {
	namespace Graphics {
		OS::Logger* RenderEngineInfo::Log()const { return Device()->Log(); }
	}
}
