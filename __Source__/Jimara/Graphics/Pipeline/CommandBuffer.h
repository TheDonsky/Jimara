#pragma once
namespace Jimara {
	namespace Graphics {
		class CommandPool;
		class CommandBuffer;
	}
}
#include "../../Core/Object.h"


namespace Jimara {
	namespace Graphics {
		class CommandPool : public virtual Object {
		public:
			virtual Reference<CommandBuffer> CreateCommandBuffer() = 0;

			virtual std::vector<Reference<CommandBuffer>> CreateCommandBuffers(size_t count) = 0;
		};


		class CommandBuffer : public virtual Object {
		public:

		};
	}
}
