#include "Layers.h"


namespace Jimara {
	Layers* Layers::Main() {
		static Layers instance;
		return &instance;
	}
}
