#pragma once
#include "../../../../Graphics/GraphicsDevice.h"
#include "../../../../Graphics/Data/ShaderBinaries/ShaderLoader.h"
#include "../../../../Graphics/Data/ShaderBinaries/ShaderResourceBindings.h"


namespace Jimara {
	/// <summary> CPU-side definition for gl Jimara_RNG_t </summary>
	struct JIMARA_API GraphicsRNG {
		alignas(4) uint32_t a = 0u;         // Bytes [0 - 4)
		alignas(4) uint32_t b = 1u;         // Bytes [4 - 8)
		alignas(4) uint32_t c = 2u;         // Bytes [8 - 12)
		alignas(4) uint32_t d = 3u;         // Bytes [12 - 16)
		alignas(4) uint32_t e = 4u;         // Bytes [16 - 20)
		alignas(4) uint32_t counter = 0u;   // Bytes [20 - 24)
		alignas(4) uint32_t pad_0 = 0u;     // Bytes [24 - 28)
		alignas(4) uint32_t pad_1 = 0u;     // Bytes [28 - 32)
	};
	static_assert(sizeof(GraphicsRNG) == 32u);
}
