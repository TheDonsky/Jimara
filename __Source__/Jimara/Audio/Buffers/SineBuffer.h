#pragma once
#include "AudioBuffer.h"


namespace Jimara {
	namespace Audio {
		class SineBuffer : public virtual AudioBuffer {
		public:
			struct ChannelSettings {
				/// <summary> Wave frequency </summary>
				float frequency;

				/// <summary> Phase offset in radians </summary>
				float phaseOffset;

				inline ChannelSettings(float freq = 128.0f, float off = 0.0f) : frequency(freq), phaseOffset(off) {}
			};

			SineBuffer(const ChannelSettings* channels, size_t channelCount, size_t sampleRate, size_t sampleCount);

			template<size_t ChannelCount>
			inline SineBuffer(const ChannelSettings channels[ChannelCount], size_t sampleRate, size_t sampleCount)
				: SineBuffer(channels, ChannelCount, sampleRate, sampleCount) {}

			SineBuffer(const ChannelSettings& settings, size_t sampleRate, size_t sampleCount);

			SineBuffer(float frequency, float phaseOffset, size_t sampleRate, size_t sampleCount);

			ChannelSettings Settings(size_t channel = 0)const;

			virtual void GetData(size_t sampleRangeOffset, AudioData& data)const override;

		private:
			const std::vector<ChannelSettings> m_settings;
		};
	}
}
