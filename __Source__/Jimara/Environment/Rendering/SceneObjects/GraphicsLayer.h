#pragma once
#include "../../../Math/BitMask.h"


namespace Jimara {
	/// <summary> Layer of a graphics object (various renderers may choose to include or exclude layers) </summary>
	typedef uint8_t GraphicsLayer;

	/// <summary> Bitmask of graphics layers </summary>
	typedef BitMask<GraphicsLayer> GraphicsLayerMask;
}
