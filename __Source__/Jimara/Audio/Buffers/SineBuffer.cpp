#include "SineBuffer.h"
#include "../../Math/Math.h"


namespace Jimara {
	namespace Audio {
		SineBuffer::SineBuffer(const ChannelSettings* channels, size_t channelCount, size_t sampleRate, size_t sampleCount, const AudioChannelLayout* layout)
			: AudioBuffer(sampleRate, sampleCount, channelCount, layout)
			, m_settings(channels, channels + channelCount) {}

		SineBuffer::SineBuffer(const ChannelSettings& settings, size_t sampleRate, size_t sampleCount)
			: SineBuffer(&settings, 1, sampleRate, sampleCount) {}

		SineBuffer::SineBuffer(float frequency, float phaseOffset, size_t sampleRate, size_t sampleCount) 
			: SineBuffer(ChannelSettings(frequency, phaseOffset), sampleRate, sampleCount) {}

		SineBuffer::ChannelSettings SineBuffer::Settings(size_t channel)const { return m_settings[channel]; }

		void SineBuffer::GetData(size_t sampleRangeOffset, size_t sampleRangeSize, AudioData& data)const {
			sampleRangeSize = min(sampleRangeSize, data.SampleCount());
			size_t matched = min(m_settings.size(), data.ChannelCount());
			size_t unmatched = max(m_settings.size(), data.ChannelCount());
			size_t channel = 0;
			if (SampleRate() > 0) {
				const float ONE_HZ = 2.0f * Math::Pi();
				const float sampleTime = ONE_HZ / static_cast<float>(SampleRate());
				const size_t sampleCount = (SampleCount() == InfiniteSamples())
					? sampleRangeSize
					: ((SampleCount() >= sampleRangeOffset) ? min(SampleCount() - sampleRangeOffset, sampleRangeSize) : 0);
				while (channel < matched) {
					const ChannelSettings settings = Settings(channel);
					const float frequency = max(abs(settings.frequency), 0.0000001f);
					const float phaseDelta = (sampleTime * frequency);
					float time = (static_cast<float>(sampleRangeOffset) * phaseDelta) + settings.phaseOffset;
					size_t i = 0;
					while (i < sampleCount) {
						data(channel, i) = std::sin(time);
						time += phaseDelta;
						i++;
					}
					while (i < sampleRangeSize) {
						data(channel, i) = 0.0f;
						i++;
					}
					channel++;
				}
			}
			while (channel < unmatched) {
				for (size_t i = 0; i < sampleRangeSize; i++)
					data(channel, i) = 0.0f;
				channel++;
			}
		}
	}
}
