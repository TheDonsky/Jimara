#pragma once
#include "Buffers/AudioBuffer.h"


namespace Jimara {
	namespace Audio {
		class AudioClip : public virtual Object {
		public:
			const AudioBuffer* Buffer()const { return m_buffer; }

		protected:
			inline AudioClip(const AudioBuffer* buffer) : m_buffer(buffer) {}

		private:
			const Reference<const AudioBuffer> m_buffer;
		};
	}
}
