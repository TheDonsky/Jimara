#pragma once
#include "../../Core/Object.h"
#include <vector>

namespace Jimara {
	namespace Audio {
		/// <summary>
		/// Waveform data chunk
		/// </summary>
		class AudioData {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="channelCount"> Number of audio channels </param>
			/// <param name="sampleCount"> Number of audio sample frames (total number of samples is channelCount * sampleCount) </param>
			inline AudioData(size_t channelCount, size_t sampleCount) 
				: m_channelCount(channelCount), m_sampleCount(sampleCount)
				, m_data(channelCount * sampleCount) {}

			/// <summary> Number of audio channels </summary>
			inline size_t ChannelCount()const { return m_channelCount; }

			/// <summary> Number of audio sample frames (total number of samples is ChannelCount * SampleCount) </summary>
			inline size_t SampleCount()const { return m_sampleCount; }

			/// <summary>
			/// Gets sample by channel and frame indices
			/// </summary>
			/// <param name="channel"> Sample channel </param>
			/// <param name="sample"> Sample id within the channel </param>
			/// <returns> Sample (should normally be in [-1; 1] range) </returns>
			inline float operator()(size_t channel, size_t sample)const { return m_data[Offset(channel, sample)]; }

			/// <summary>
			/// Gives access to the sample by channel and frame indices
			/// </summary>
			/// <param name="channel"> Sample channel </param>
			/// <param name="sample"> Sample id within the channel </param>
			/// <returns> Sample reference (should normally be in [-1; 1] range) </returns>
			inline float& operator()(size_t channel, size_t sample) { return m_data[Offset(channel, sample)]; }

		private:
			// Number of channels
			const size_t m_channelCount;

			// Number of frames
			const size_t m_sampleCount;

			// Underlying data
			std::vector<float> m_data;

			// Index of a sample, within the buffer
			inline size_t Offset(size_t channel, size_t sample)const { return (sample * m_channelCount) + channel; }
		};


		/// <summary>
		/// Arbitrary data buffer provider for audio clips
		/// </summary>
		class AudioBuffer : public virtual Object {
		public:
			/// <summary> Samples per second </summary>
			inline size_t SampleRate()const { return m_sampleRate; }

			/// <summary> 
			/// If SampleCount is InfiniteSamples(), the AudioBuffer will be thought as containing infinate number of samples,
			/// "Automagically" looping once the indices overflow (may work great for procedurally generated audio, or may not... I don't frankly know)
			/// </summary>
			inline static constexpr size_t InfiniteSamples()noexcept { return ~static_cast<size_t>(0u); }

			/// <summary> Total number of sample frames (InfiniteSamples() for infinately long audio) </summary>
			inline size_t SampleCount()const { return m_sampleCount; }

			/// <summary> Number of channels per sample frame </summary>
			inline size_t ChannelCount()const { return m_channelCount; }

			/// <summary>
			/// Retrieves data from sample number sampleRangeOffset to (sampleRangeOffset + sampleRangeSize) and stores it into data buffers
			/// Note: 
			///		0. It's the caller's reponsibility to provide data buffer, that has at least sampleRangeSize sample frames to store;
			///		1. It was considered, to drop sampleRangeSize parameter and demand range by deducing data size with 'data.SampleCount()', 
			///		but that would make buffer reuse implausible in some cases and the desision was made to drop it;
			///		2. It's the caller's reposnibility to match AudioBuffer's and AudioData's channel counts; mismatch should normally result in ignored source channels, 
			///		or zeroed-out destination buffers.
			/// </summary>
			/// <param name="sampleRangeOffset"> First sample to load </param>
			/// <param name="sampleRangeSize"> Number of samples to load </param>
			/// <param name="data"> Audio data to fill in </param>
			virtual void GetData(size_t sampleRangeOffset, size_t sampleRangeSize, AudioData& data)const = 0;

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="sampleRate"> Samples per second </param>
			/// <param name="sampleCount"> Total number of sample frames (InfiniteSamples() for infinately long audio) </param>
			/// <param name="channelCount"> Number of channels per sample frame </param>
			inline AudioBuffer(size_t sampleRate, size_t sampleCount, size_t channelCount)
				: m_sampleRate(sampleRate), m_sampleCount(sampleCount), m_channelCount(channelCount) {}

		private:
			// Samples per second
			const size_t m_sampleRate;

			// Total number of sample frames (InfiniteSamples() for infinately long audio)
			const size_t m_sampleCount;

			// Number of channels per sample frame
			const size_t m_channelCount;
		};
	}
}
