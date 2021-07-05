#pragma once
#include "Buffers/AudioBuffer.h"


namespace Jimara {
	namespace Audio {
		class AudioClip : public virtual Object {
		public:
			const AudioBuffer* Buffer()const { return m_buffer; }

			inline float Duration()const {
				if (m_buffer == nullptr) return 0.0f;
				size_t samples = m_buffer->SampleCount();
				return (static_cast<float>(samples) / static_cast<float>(m_buffer->SampleRate()));
			}

		protected:
			inline AudioClip(const AudioBuffer* buffer) : m_buffer(buffer) {}

		private:
			const Reference<const AudioBuffer> m_buffer;
		};
	}
}
