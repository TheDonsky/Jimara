#pragma once
#include "../../Core/Object.h"
#include <vector>

namespace Jimara {
	namespace Audio {
		class AudioData {
		public:
			inline AudioData(size_t channelCount, size_t sampleCount) 
				: m_channelCount(channelCount), m_sampleCount(sampleCount)
				, m_data(channelCount * sampleCount) {}

			inline size_t ChannelCount()const { return m_channelCount; }

			inline size_t SampleCount()const { return m_sampleCount; }

			inline float operator()(size_t channel, size_t sample)const { return m_data[Offset(channel, sample)]; }

			inline float& operator()(size_t channel, size_t sample) { return m_data[Offset(channel, sample)]; }

		private:
			const size_t m_channelCount;
			const size_t m_sampleCount;
			std::vector<float> m_data;

			inline size_t Offset(size_t channel, size_t sample)const { return (sample * m_channelCount) + channel; }
		};

		class AudioBuffer : public virtual Object {
		public:
			inline size_t SampleRate()const { return m_sampleRate; }

			inline static constexpr size_t InfiniteSamples()noexcept { return ~static_cast<size_t>(0u); }

			inline size_t SampleCount()const { return m_sampleCount; }

			inline size_t ChannelCount()const { return m_channelCount; }

			virtual void GetData(size_t sampleRangeOffset, AudioData& data)const = 0;

		protected:
			inline AudioBuffer(size_t sampleRate, size_t sampleCount, size_t channelCount)
				: m_sampleRate(sampleRate), m_sampleCount(sampleCount), m_channelCount(channelCount) {}

		private:
			const size_t m_sampleRate;
			const size_t m_sampleCount;
			const size_t m_channelCount;
		};
	}
}
