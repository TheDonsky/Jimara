#include "OpenALClip.h"
#include "../../Math/Math.h"
#include "../../Core/Collections/ObjectCache.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			namespace {
				class OpenALClipChunk : public virtual Object {
				private:
					const Reference<OpenALInstance> m_instance;
					const Reference<OpenALContext> m_context;
					ALuint m_buffer = 0;

				public:
					inline OpenALClipChunk(OpenALInstance* instance, OpenALContext* context, const AudioBuffer* buffer, size_t firstSample, size_t sampleCount, bool twoDimensional)
						: m_instance(instance), m_context(context) {
						
						const size_t channelCount = buffer->ChannelCount();
						if ((channelCount > 2 && twoDimensional) || channelCount <= 0) {
							m_instance->Log()->Fatal("OpenALClipChunk::OpenALClipChunk - buffer with ", channelCount, " channels not[yet] supported!");
							return;
						}

						std::vector<int16_t> bufferData((twoDimensional ? buffer->ChannelCount() : 1) * sampleCount);
						{
							AudioData data(channelCount, sampleCount);
							buffer->GetData(firstSample, sampleCount, data);
							float MX = static_cast<float>(SHRT_MAX);
							float MN = static_cast<float>(SHRT_MIN);
							if (twoDimensional) {
								size_t index = 0;
								for (size_t i = 0; i < sampleCount; i++)
									for (size_t channel = 0; channel < channelCount; channel++) {
										bufferData[index] = static_cast<int16_t>(min(max(MN, data(channel, i) * MX), MX));
										index++;
									}
							}
							else {
								float multiplier = 1.0f / static_cast<float>(channelCount);
								for (size_t i = 0; i < sampleCount; i++) {
									float total = 0.0f;
									for (size_t channel = 0; channel < channelCount; channel++) total += data(channel, i);
									bufferData[i] = static_cast<int16_t>(min(max(MN, total * multiplier * MX), MX));
								}
							}
						}
						const ALenum format = (channelCount > 1 && twoDimensional) ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16;

						std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
						OpenALContext::SwapCurrent setContext(m_context);
						alGenBuffers(1, &m_buffer);
						if (m_instance->ReportALError("OpenALClipChunk::OpenALClipChunk - alGenBuffers(1, &m_buffer) Failed!") >= OS::Logger::LogLevel::LOG_WARNING) {
							m_buffer = 0;
							return;
						}
						else if (m_buffer == 0) m_instance->Log()->Fatal("OpenALClipChunk::OpenALClipChunk - alGenBuffers() returned 0!");
						alBufferData(m_buffer, format, bufferData.data(), static_cast<ALsizei>(sizeof(int16_t) * bufferData.size()), static_cast<ALsizei>(buffer->SampleRate()));
						if (m_instance->ReportALError("OpenALClipChunk::OpenALClipChunk - alBufferData(...) Failed!") >= OS::Logger::LogLevel::LOG_WARNING) return;
					}

					inline ~OpenALClipChunk() {
						if (m_buffer == 0) return;
						std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
						OpenALContext::SwapCurrent setContext(m_context);
						alDeleteBuffers(1, &m_buffer);
						if (m_instance->ReportALError("OpenALClip::~OpenALClip - alDeleteBuffers(1, &m_buffer) Failed!") >= OS::Logger::LogLevel::LOG_WARNING) return;
						m_buffer = 0;
					}

					inline operator ALuint()const { return m_buffer; }

					inline static bool CanShare2DAnd3DChunks(const AudioBuffer* buffer) {
						return (buffer->ChannelCount() == 1 || buffer->ChannelLayout() == AudioChannelLayout::Mono());
					}
				};

				class SimpleClipPlayback2D : public ClipPlayback2D {
				public:

				};

				class SimpleClipPlayback3D : public ClipPlayback3D {
				public:

				};



				class SimpleClip : public virtual OpenALClip {
				private:
					std::mutex m_lock;
					Reference<OpenALClipChunk> m_monoChunk;
					Reference<OpenALClipChunk> m_stereoChunk;

				public:
					inline SimpleClip(OpenALDevice* device, const AudioBuffer* buffer) : AudioClip(buffer), OpenALClip(device, buffer) {}

					inline virtual Reference<ClipPlayback2D> Play2D(ListenerContext* context, const AudioSource2D::Settings& settings, bool loop, float timeOffset) override {
						if (m_stereoChunk == nullptr) {
							std::unique_lock<std::mutex> lock(m_lock);
							if (m_stereoChunk == nullptr) {
								const bool shareChunks = OpenALClipChunk::CanShare2DAnd3DChunks(Buffer());
								if (shareChunks) m_stereoChunk = m_monoChunk;
								if (m_stereoChunk == nullptr) {
									m_stereoChunk = Object::Instantiate<OpenALClipChunk>(Device()->ALInstance(), Device()->DefaultContext(), Buffer(), 0, Buffer()->SampleCount(), true);
									if (shareChunks) m_monoChunk = m_stereoChunk;
								}
							}
						}
						// __TODO__: Create actual playback...
						Device()->APIInstance()->Log()->Warning("SimpleClip::Play2D - Not yet implemented!");
						return nullptr;
					}

					inline virtual Reference<ClipPlayback3D> Play3D(ListenerContext* context, const AudioSource3D::Settings& settings, bool loop, float timeOffset) override {
						if (m_monoChunk == nullptr) {
							std::unique_lock<std::mutex> lock(m_lock);
							if (m_monoChunk == nullptr) m_monoChunk = Object::Instantiate<OpenALClipChunk>(Device()->ALInstance(), Device()->DefaultContext(), Buffer(), 0, Buffer()->SampleCount(), false);
						}
						// __TODO__: Create actual playback...
						Device()->APIInstance()->Log()->Warning("SimpleClip::Play3D - Not yet implemented!");
						return nullptr;
					}
				};

				class StreamedClip : public virtual OpenALClip {
				private:
					ObjectCache<size_t> m_monoCache;
					ObjectCache<size_t> m_stereoCache;

				public:
					inline StreamedClip(OpenALDevice* device, const AudioBuffer* buffer) : AudioClip(buffer), OpenALClip(device, buffer) {}


					inline virtual Reference<ClipPlayback2D> Play2D(ListenerContext* context, const AudioSource2D::Settings& settings, bool loop, float timeOffset) override {
						// __TODO__: Create actual playback...
						Device()->APIInstance()->Log()->Warning("StreamedClip::Play2D - Not yet implemented!");
						return nullptr;
					}

					inline virtual Reference<ClipPlayback3D> Play3D(ListenerContext* context, const AudioSource3D::Settings& settings, bool loop, float timeOffset) override {
						// __TODO__: Create actual playback...
						Device()->APIInstance()->Log()->Warning("StreamedClip::Play3D - Not yet implemented!");
						return nullptr;
					}
				};
			}

			Reference<OpenALClip> OpenALClip::Create(OpenALDevice* device, const AudioBuffer* buffer, bool streamed) {
				if (buffer == nullptr) {
					device->APIInstance()->Log()->Error("OpenALClip::Create - nullptr buffer provided!");
					return nullptr;
				}
				if (streamed) return Object::Instantiate<StreamedClip>(device, buffer);
				else return Object::Instantiate<SimpleClip>(device, buffer);
			}

			OpenALDevice* OpenALClip::Device()const { return m_device; }

			OpenALClip::OpenALClip(OpenALDevice* device, const AudioBuffer* buffer) : AudioClip(buffer), m_device(device) {}


			ClipPlayback::ClipPlayback(ListenerContext* context) : m_context(context), m_source(context->GetSource()) {}

			ClipPlayback::~ClipPlayback() { 
				{
					std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
					OpenALContext::SwapCurrent swap(m_context);
					alSourceStop(m_source);
					m_context->Device()->ALInstance()->ReportALError("ClipPlayback::~ClipPlayback - alSourceStop Failed!");
				}
				m_context->FreeSource(m_source); 
			}

			ListenerContext* ClipPlayback::Context()const { return m_context; }

			ALuint ClipPlayback::Source()const { return m_source; }



			ClipPlayback2D::ClipPlayback2D(ListenerContext* context, const AudioSource2D::Settings& settings) : ClipPlayback(context) {
				Update(settings);
			}

			void ClipPlayback2D::Update(const AudioSource2D::Settings& settings) {
				std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
				OpenALContext::SwapCurrent swap(Context());

				alSourcef(Source(), AL_PITCH, settings.pitch);
				Context()->Device()->ALInstance()->ReportALError("ClipPlayback2D::Update - alSourcef(Source(), AL_PITCH, settings.pitch) Failed!");

				alSourcef(Source(), AL_GAIN, settings.volume);
				Context()->Device()->ALInstance()->ReportALError("ClipPlayback2D::Update - alSourcef(Source(), AL_GAIN, settings.volume) Failed!");
			}

			ClipPlayback3D::ClipPlayback3D(ListenerContext* context, const AudioSource3D::Settings& settings) : ClipPlayback(context) {
				Update(settings);
			}

			void ClipPlayback3D::Update(const AudioSource3D::Settings& settings) {
				std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
				OpenALContext::SwapCurrent swap(Context());

				alSource3f(Source(), AL_POSITION, settings.position.x, settings.position.y, -settings.position.z);
				Context()->Device()->ALInstance()->ReportALError("ClipPlayback3D::Update - alSource3f(Source(), AL_POSITION, settings.position) Failed!");

				alSource3f(Source(), AL_VELOCITY, settings.velocity.x, settings.velocity.y, -settings.velocity.z);
				Context()->Device()->ALInstance()->ReportALError("ClipPlayback3D::Update - alSource3f(Source(), AL_VELOCITY, settings.velocity) Failed!");

				alSourcef(Source(), AL_PITCH, settings.pitch);
				Context()->Device()->ALInstance()->ReportALError("ClipPlayback3D::Update - alSourcef(Source(), AL_PITCH, settings.pitch) Failed!");

				alSourcef(Source(), AL_GAIN, settings.volume);
				Context()->Device()->ALInstance()->ReportALError("ClipPlayback3D::Update - alSourcef(Source(), AL_GAIN, settings.volume) Failed!");
			}
		}
	}
}
