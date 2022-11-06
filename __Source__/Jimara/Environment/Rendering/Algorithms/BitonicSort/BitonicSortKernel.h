#pragma once
#include "../../../../Graphics/GraphicsDevice.h"


namespace Jimara {
	class JIMARA_API BitonicSortKernel : public virtual Object {
	public:
		BitonicSortKernel(
			Graphics::GraphicsDevice* device, size_t maxInFlightCommandBuffers,
			Graphics::Shader* sortShader);

		virtual ~BitonicSortKernel();

	private:

	};
}
