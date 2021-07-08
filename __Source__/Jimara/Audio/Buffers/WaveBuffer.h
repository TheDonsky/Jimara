#pragma once
#include "AudioBuffer.h"
#include "../../OS/Logging/Logger.h"
#include "../../Core/Memory/MemoryBlock.h"


namespace Jimara {
	namespace Audio {
		/// <summary>
		/// Builds audio buffer based on a RIFF-encoded memory block
		/// </summary>
		/// <param name="block"> Memory block, encoding data in simple RIFF format </param>
		/// <param name="logger"> Logger for error reporting </param>
		/// <returns> A new instance of an AudioBuffer </returns>
		Reference<AudioBuffer> WaveBuffer(const MemoryBlock& block, OS::Logger* logger = nullptr);

		/// <summary>
		/// Builds audio buffer from a WAVE file
		/// </summary>
		/// <param name="filename"> File path </param>
		/// <param name="logger"> Logger for error reporting </param>
		/// <returns> A new instance of an AudioBuffer </returns>
		Reference<AudioBuffer> WaveBuffer(const std::string_view& filename, OS::Logger* logger = nullptr);
	}
}
