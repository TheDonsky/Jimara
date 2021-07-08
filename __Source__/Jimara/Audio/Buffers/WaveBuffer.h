#pragma once
#include "AudioBuffer.h"
#include "../../OS/Logging/Logger.h"
#include "../../Core/Memory/MemoryBlock.h"


namespace Jimara {
	namespace Audio {
		Reference<AudioBuffer> WaveBuffer(const MemoryBlock& block, OS::Logger* logger = nullptr);
	}
}
