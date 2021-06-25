#pragma once
#include "AudioBuffer.h"


namespace Jimara {
	namespace Audio {
		/// <summary>
		/// An AudioBuffer, that generates simple sinusoids for it's channels
		/// </summary>
		class SineBuffer : public virtual AudioBuffer {
		public:
			/// <summary>
			/// Settings for a single audio channel
			/// </summary>
			struct ChannelSettings {
				/// <summary> Wave frequency </summary>
				float frequency;

				/// <summary> Phase offset in radians </summary>
				float phaseOffset;

				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="freq"> Wave frequency</param>
				/// <param name="off"> Phase offset in radians </param>
				inline ChannelSettings(float freq = 128.0f, float off = 0.0f) : frequency(freq), phaseOffset(off) {}
			};

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="channels"> Channel settings </param>
			/// <param name="channelCount"> Number of channels to use </param>
			/// <param name="sampleRate"> Sample frames per second </param>
			/// <param name="sampleCount"> Total number of sample frames </param>
			SineBuffer(const ChannelSettings* channels, size_t channelCount, size_t sampleRate, size_t sampleCount);

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="channels"> Channel settings </param>
			/// <param name="sampleRate"> Sample frames per second </param>
			/// <param name="sampleCount"> Total number of sample frames </param>
			template<size_t ChannelCount>
			inline SineBuffer(const ChannelSettings channels[ChannelCount], size_t sampleRate, size_t sampleCount)
				: SineBuffer(channels, ChannelCount, sampleRate, sampleCount) {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="channels"> Channel settings </param>
			/// <param name="sampleRate"> Sample frames per second </param>
			/// <param name="sampleCount"> Total number of sample frames </param>
			SineBuffer(const ChannelSettings& settings, size_t sampleRate, size_t sampleCount);

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="frequency"> Wave frequency </param>
			/// <param name="phaseOffset"> Phase offset in radians </param>
			/// <param name="sampleRate"> Sample frames per second </param>
			/// <param name="sampleCount"> Total number of sample frames </param>
			SineBuffer(float frequency, float phaseOffset, size_t sampleRate, size_t sampleCount);

			/// <summary>
			/// Settings per channel
			/// </summary>
			/// <param name="channel"> Channel id </param>
			/// <returns> Channel settings </returns>
			ChannelSettings Settings(size_t channel = 0)const;

			/// <summary>
			/// Generates sine waves and stores them in the buffer data chunk
			/// </summary>
			/// <param name="sampleRangeOffset"> First sample to load </param>
			/// <param name="sampleRangeSize"> Number of samples to load </param>
			/// <param name="data"> Audio data to fill in </param>
			virtual void GetData(size_t sampleRangeOffset, size_t sampleRangeSize, AudioData& data)const override;

		private:
			// Settings per channel
			const std::vector<ChannelSettings> m_settings;
		};
	}
}
