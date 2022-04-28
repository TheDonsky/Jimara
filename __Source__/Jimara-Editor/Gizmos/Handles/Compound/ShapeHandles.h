#pragma once
#include "../FreeMoveHandle.h"
#include "../FixedAxisMoveHandle.h"


namespace Jimara {
	namespace Editor {
		Reference<FreeMoveHandle> FreeMoveSphereHandle(Component* parent, Vector4 color, const std::string_view& name);

		Reference<FixedAxisMoveHandle> FixedAxisArrowHandle(Component* parent, Vector4 color, const std::string_view& name);
	}
}
