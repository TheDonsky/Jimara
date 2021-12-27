#pragma once
#include "Buffers/AudioBuffer.h"
#include "../Data/AssetDatabase/AssetDatabase.h"


namespace Jimara {
	namespace Audio {
		/// <summary>
		/// AudioClip instance, derived from an AudioBuffer and tied to a specific AudioDevice
		/// </summary>
		class AudioClip : public virtual Resource {
		public:
			/// <summary> AudioBuffer, the clip is based on </summary>
			const AudioBuffer* Buffer()const { return m_buffer; }

			/// <summary> Clip duration in seconds </summary>
			inline float Duration()const {
				if (m_buffer == nullptr) return 0.0f;
				size_t samples = m_buffer->SampleCount();
				return (static_cast<float>(samples) / static_cast<float>(m_buffer->SampleRate()));
			}

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="buffer"> AudioBuffer, the clip is based on </param>
			inline AudioClip(const AudioBuffer* buffer) : m_buffer(buffer) {}

		private:
			// AudioBuffer, the clip is based on
			const Reference<const AudioBuffer> m_buffer;
		};
	}

	// Parent type definition for the resource
	template<> inline void TypeIdDetails::GetParentTypesOf<Audio::AudioClip>(const Callback<TypeId>& report) { report(TypeId::Of<Resource>()); }
}
