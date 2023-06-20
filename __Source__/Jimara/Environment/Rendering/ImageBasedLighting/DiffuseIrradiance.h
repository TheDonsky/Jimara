#pragma once
#include "../../../Graphics/GraphicsDevice.h"

namespace Jimara {
	// __TODO__: Implement this crap!

	class JIMARA_API DiffuseIrradiance : public virtual Object {
	public:
		static Reference<DiffuseIrradiance> GetFor(Graphics::TextureView* hdri);

		Graphics::TextureSampler* Sampler();
	};
}
