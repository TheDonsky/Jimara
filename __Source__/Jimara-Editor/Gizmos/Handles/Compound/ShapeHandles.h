#pragma once
#include "../DragHandle.h"


namespace Jimara {
	namespace Editor {
		/// <summary>
		/// Creates a free movement handle with a sphere renderer
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="color"> Handle color </param>
		/// <param name="name"> Handle name </param>
		/// <returns> Handle instance </returns>
		Reference<DragHandle> FreeMoveSphereHandle(Component* parent, Vector4 color, const std::string_view& name);

		/// <summary>
		/// Creates a fixed axis movement handle with an arrow renderer
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="color"> Handle color </param>
		/// <param name="name"> Handle name </param>
		/// <returns> Handle instance </returns>
		Reference<DragHandle> FixedAxisArrowHandle(Component* parent, Vector4 color, const std::string_view& name);
	}
}
