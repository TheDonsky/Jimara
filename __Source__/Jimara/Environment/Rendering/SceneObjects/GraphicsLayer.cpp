#include "GraphicsLayer.h"


namespace Jimara {
	GraphicsLayers* GraphicsLayers::Main() {
		static GraphicsLayers instance;
		return &instance;
	}
}
