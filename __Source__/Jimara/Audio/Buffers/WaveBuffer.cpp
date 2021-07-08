#include "WaveBuffer.h"
#include "../../Math/Math.h"


namespace Jimara {
	namespace Audio {
		namespace {
			struct RiffHeader {
				char chunkId[4] = { 0 }; // Should be "RIFF" for little endian and "RIFX" for big endian
				uint32_t chunkSize = 0; // Should be (block.size() - 8)
				char format[4] = { 0 }; // Should be "WAVE"
				Endian endian = Endian::LITTLE;

				inline bool Read(size_t& offset, const MemoryBlock& block, OS::Logger* logger) {
					if ((offset + 12) > block.Size()) {
						if (logger != nullptr) logger->Error("WaveBuffer::RiffHeader::Read - Memory block not large enough!");
						return false;
					}
					bool rv = true;
					{
						const char* data = reinterpret_cast<const char*>(block.Data()) + offset;
						for (size_t i = 0; i < 4; i++) chunkId[i] = data[i];
						offset += 4;
						if (memcmp(chunkId, "RIFX", 4) == 0) endian = Endian::BIG;
						else {
							endian = Endian::LITTLE;
							if (memcmp(chunkId, "RIFF", 4) != 0) {
								if (logger != nullptr) logger->Error("WaveBuffer::RiffHeader::Read - ChunkID not 'RIFF'!");
								rv = false;
							}
						}
					}
					{
						static_assert(sizeof(uint32_t) == 4);
						chunkSize = block.Get<uint32_t>(offset, endian);
						if (chunkSize != (block.Size() - offset)) {
							if (logger != nullptr) logger->Error("WaveBuffer::RiffHeader::Read - ChunkSize<", chunkSize, "> != (block.Size() - offset)<", (block.Size() - offset), ">!");
							rv = false;
						}
					}
					{
						const char* data = reinterpret_cast<const char*>(block.Data()) + offset;
						for (size_t i = 0; i < 4; i++) format[i] = data[i];
						offset += 4;
						if (memcmp(format, "WAVE", 4) != 0) {
							if (logger != nullptr) logger->Error("WaveBuffer::RiffHeader::Read - Format not 'WAVE'!");
							rv = false;
						}
					}
					return rv;
				}
			};


			struct FmtSubChunk {
				char subchunk1Id[4] = { 0 }; // Should be "fmt "
				uint32_t subchunk1Size = 0; // Should be 16
				uint16_t audioFormat = 0; // Should be 1 (for uncompressed)
				uint16_t numChannels = 0; // Should be 1 for mono, 2 for stereo, 6 for 5.1 and etc.. 
				uint32_t sampleRate = 0; // Whatever...
				uint32_t byteRate = 0; // Should be (sampleRate * numChannels * bitsPerSample / 8)
				uint16_t blockAlign = 0; // Should be (NumChannels * BitsPerSample / 8)
				uint16_t bitsPerSample = 0; // 8/16/32 Others we do not support...

				inline bool Read(size_t& offset, const MemoryBlock& block, OS::Logger* logger, Endian endian) {
					static_assert(sizeof(FmtSubChunk) == 24);
					if ((offset + sizeof(FmtSubChunk)) > block.Size()) {
						if (logger != nullptr) logger->Error("WaveBuffer::FmtSubChunk::Read - Memory block not large enough!");
						return false;
					}
					bool rv = true;
					{
						const char* data = reinterpret_cast<const char*>(block.Data()) + offset;
						for (size_t i = 0; i < 4; i++) subchunk1Id[i] = data[i];
						offset += 4;
						if (memcmp(subchunk1Id, "fmt ", 4) != 0) {
							if (logger != nullptr) logger->Error("WaveBuffer::FmtSubChunk::Read - Subchunk1Id not 'fmt '!");
							rv = false;
						}
					}
					{
						static_assert(sizeof(uint32_t) == 4);
						subchunk1Size = block.Get<uint32_t>(offset, endian);
						if (subchunk1Size != 16) {
							if (logger != nullptr) logger->Error("WaveBuffer::FmtSubChunk::Read - subchunk1Size<", subchunk1Size, "> not 16!");
							rv = false;
						}
					}
					{
						static_assert(sizeof(uint16_t) == 2);
						audioFormat = block.Get<uint16_t>(offset, endian);
						if (audioFormat != 1) {
							if (logger != nullptr) logger->Error("WaveBuffer::FmtSubChunk::Read - audioFormat<", audioFormat, "> not 1 <Compressed/Non-PCM data not supported>!");
							rv = false;
						}
					}
					{
						numChannels = block.Get<uint16_t>(offset, endian); // Will be analized later...
						sampleRate = block.Get<uint32_t>(offset, endian);
						byteRate = block.Get<uint32_t>(offset, endian);
						blockAlign = block.Get<uint16_t>(offset, endian);
						bitsPerSample = block.Get<uint16_t>(offset, endian);
						
						if ((bitsPerSample % 8) != 0) {
							if (logger != nullptr) logger->Error("WaveBuffer::FmtSubChunk::Read - bitsPerSample<", bitsPerSample, "> not multiple of 8 (If they exist, we do not yet support those)!");
							rv = false;
						}

						const uint32_t expectedByteRate = (sampleRate * numChannels * bitsPerSample / 8);
						if (byteRate != expectedByteRate) {
							if (logger != nullptr) logger->Error("WaveBuffer::FmtSubChunk::Read - byteRate<", byteRate, "> is not (sampleRate * numChannels * bitsPerSample / 8)<", expectedByteRate, ">!");
							rv = false;
						}

						const uint16_t expectedBlockAlign = (numChannels * bitsPerSample / 8);
						if (blockAlign != expectedBlockAlign) {
							if (logger != nullptr) logger->Error("WaveBuffer::FmtSubChunk::Read - blockAlign<", blockAlign, "> is not (numChannels * bitsPerSample / 8)<", expectedBlockAlign, ">!");
							rv = false;
						}
					}
					return rv;
				}
			};

			struct DataSubChunk {
				char subchunk2Id[4] = { 0 }; // Should be "data"
				uint32_t subchunk2Size = 0; // Should be (NumSamples * NumChannels * BitsPerSample / 8); Same as the size of the rest of the file from here on
				const void* data = nullptr;

				inline bool Read(size_t& offset, const MemoryBlock& block, OS::Logger* logger, Endian endian) {
					if ((offset + 8) > block.Size()) {
						if (logger != nullptr) logger->Error("WaveBuffer::DataSubChunk::Read - Memory block not large enough!");
						return false;
					}
					bool rv = true;
					{
						const char* data = reinterpret_cast<const char*>(block.Data()) + offset;
						for (size_t i = 0; i < 4; i++) subchunk2Id[i] = data[i];
						offset += 4;
						if (memcmp(subchunk2Id, "data", 4) != 0) {
							if (logger != nullptr) logger->Error("WaveBuffer::DataSubChunk::Read - subchunk2Id not 'data'!");
							rv = false;
						}
					}
					{
						static_assert(sizeof(uint32_t) == 4);
						subchunk2Size = block.Get<uint32_t>(offset, endian);
						if (subchunk2Size != (block.Size() - offset)) {
							if (logger != nullptr) logger->Error("WaveBuffer::DataSubChunk::Read - subchunk2Size<", subchunk2Size, "> not (block.Size() - offset)", (block.Size() - offset), "!");
							rv = false;
						}
					}
					data = reinterpret_cast<const void*>(reinterpret_cast<const char*>(block.Data()) + offset);
					return rv;;
				}
			};


			template<typename SampleType, SampleType ZERO, SampleType MAX_VALUE, AudioFormat FormatType, Endian Endianness>
			class WavBuffer : public virtual AudioBuffer {
			private:
				const MemoryBlock m_dataBlock;

			public:
				inline WavBuffer(size_t sampleRate, size_t sampleCount, const void* data, const Object* dataBlockOwner)
					: AudioBuffer(sampleRate, sampleCount, FormatType)
					, m_dataBlock(data, sampleCount * sizeof(SampleType) * AudioBuffer::ChannelCount(FormatType), dataBlockOwner) {}

				virtual void GetData(size_t sampleRangeOffset, size_t sampleRangeSize, AudioData& data)const override {
					const size_t channelCount = AudioBuffer::ChannelCount(FormatType);
					size_t framesPresent = min((sampleRangeOffset < SampleCount()) ? (SampleCount() - sampleRangeOffset) : 0, sampleRangeSize);
					
					size_t it = sampleRangeOffset * channelCount;
					const float ZERO_VAL = static_cast<float>(ZERO);
					const float SCALE = 1.0f / static_cast<float>(MAX_VALUE - ZERO);
					for (size_t frame = 0; frame < framesPresent; frame++)
						for (size_t channel = 0; channel < channelCount; channel++)
							data(channel, frame) = (static_cast<float>(m_dataBlock.Get<SampleType>(it, Endianness)) - ZERO_VAL) * SCALE;
					
					for (size_t frame = framesPresent; frame < sampleRangeSize; frame++)
						for (size_t channel = 0; channel < channelCount; channel++)
							data(channel, frame) = 0.0f;
				}
			};

			template<AudioFormat Format, Endian Endianness>
			Reference<AudioBuffer> CreateWaveBufferFmt(const FmtSubChunk& fmtChunk, size_t sampleCount, const void* data, const Object* dataBlockOwner, OS::Logger* logger) {
				if (fmtChunk.bitsPerSample == 8)
					return Object::Instantiate<WavBuffer<uint8_t, 128, UINT8_MAX, Format, Endianness>>(fmtChunk.sampleRate, sampleCount, data, dataBlockOwner);
				else if (fmtChunk.bitsPerSample == 16)
					return Object::Instantiate<WavBuffer<int16_t, 0, INT16_MAX, Format, Endianness>>(fmtChunk.sampleRate, sampleCount, data, dataBlockOwner);
				else if (fmtChunk.bitsPerSample == 32)
					return Object::Instantiate<WavBuffer<int32_t, 0, INT32_MAX, Format, Endianness>>(fmtChunk.sampleRate, sampleCount, data, dataBlockOwner);
				else {
					if (logger != nullptr) logger->Error("WaveBuffer::CreateWaveBufferFmt - fmtChunk.bitsPerSample<", fmtChunk.bitsPerSample, "> Not supported!");
					return nullptr;
				}
			}

			template<Endian Endianness>
			Reference<AudioBuffer> CreateWaveBuffer(const FmtSubChunk& fmtChunk, size_t sampleCount, const void* data, const Object* dataBlockOwner, OS::Logger* logger) {
				if (fmtChunk.numChannels == 1)
					return CreateWaveBufferFmt<AudioFormat::MONO, Endianness>(fmtChunk, sampleCount, data, dataBlockOwner, logger);
				else if (fmtChunk.numChannels == 2)
					return CreateWaveBufferFmt<AudioFormat::STEREO, Endianness>(fmtChunk, sampleCount, data, dataBlockOwner, logger);
				else if (fmtChunk.numChannels == 6)
					return CreateWaveBufferFmt<AudioFormat::SURROUND_5_1, Endianness>(fmtChunk, sampleCount, data, dataBlockOwner, logger);
				else {
					if (logger != nullptr) logger->Error("WaveBuffer::CreateWaveBuffer - fmtChunk.numChannels<", fmtChunk.numChannels, "> Not supported!");
					return nullptr;
				}
			}
		}

		Reference<AudioBuffer> WaveBuffer(const MemoryBlock& block, OS::Logger* logger) {
			size_t offset = 0;

			RiffHeader header;
			if (!header.Read(offset, block, logger)) return nullptr;

			FmtSubChunk fmtChunk;
			if (!fmtChunk.Read(offset, block, logger, header.endian)) return nullptr;

			DataSubChunk dataChunk;
			if (!dataChunk.Read(offset, block, logger, header.endian)) return nullptr;

			if (logger != nullptr && ((dataChunk.subchunk2Size % fmtChunk.blockAlign) != 0))
				logger->Warning("WaveBuffer - dataChunk.subchunk2Size<", dataChunk.subchunk2Size, "> not multiple of fmtChunk.blockAlign<", fmtChunk.blockAlign, ">!");

			size_t sampleCount = (dataChunk.subchunk2Size / fmtChunk.blockAlign);

			if (header.endian == Endian::LITTLE)
				return CreateWaveBuffer<Endian::LITTLE>(fmtChunk, sampleCount, dataChunk.data, block.DataOwner(), logger);
			else return CreateWaveBuffer<Endian::BIG>(fmtChunk, sampleCount, dataChunk.data, block.DataOwner(), logger);
		}
	}
}
