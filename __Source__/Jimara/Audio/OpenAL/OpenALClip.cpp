#include "OpenALClip.h"
#include "../../Math/Math.h"
#include "../../Core/Collections/ObjectCache.h"


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			namespace {
				/** ########################################################################################################################## */
				/** ################################################ OpenALClipChunk ######################################################### */
				/** ########################################################################################################################## */


				class OpenALClipChunk : public virtual Object {
				private:
					const Reference<OpenALInstance> m_instance;
					const Reference<OpenALContext> m_context;
					ALuint m_buffer = 0;
					ALint m_sampleCount;

				public:
					inline static bool CanShare2DAnd3DChunks(const AudioBuffer* buffer) {
						return (buffer->ChannelCount() == 1 || buffer->Format() == AudioFormat::MONO || buffer->Format() >= AudioFormat::CHANNEL_LAYOUT_COUNT);
					}

					inline OpenALClipChunk(OpenALInstance* instance, OpenALContext* context, const AudioBuffer* buffer, size_t firstSample, size_t sampleCount, bool twoDimensional)
						: m_instance(instance), m_context(context), m_sampleCount(static_cast<ALint>(sampleCount)) {
						
						AudioFormat jFormat;
						if (CanShare2DAnd3DChunks(buffer)) {
							twoDimensional = false;
							jFormat = AudioFormat::MONO;
						}
						else jFormat = buffer->Format();

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
						const ALenum format =
							(jFormat == AudioFormat::MONO) ? AL_FORMAT_MONO16 :
							(jFormat == AudioFormat::STEREO) ? AL_FORMAT_STEREO16 :
							(jFormat == AudioFormat::SURROUND_5_1) ? AL_FORMAT_51CHN16 : AL_FORMAT_MONO16;

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

					inline ALint SampleCount() { return m_sampleCount; }
				};





				/** ########################################################################################################################## */
				/** ############################################# Non-Streamed playbacks ##################################################### */
				/** ########################################################################################################################## */

				inline static void PlayChunk(ListenerContext* context, ALuint source, OpenALClipChunk* chunk, bool looping, size_t sampleOffset) {
					std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
					OpenALContext::SwapCurrent swap(context);
					
					if (chunk == nullptr) {
						alSourceStop(source);
						context->Device()->ALInstance()->ReportALError("OpenALClip.cpp::PlayChunk - alSourceStop(source) Failed!");
					}

					alSourcei(source, AL_BUFFER, (chunk == nullptr) ? static_cast<ALuint>(0u) : (*chunk));
					context->Device()->ALInstance()->ReportALError("OpenALClip.cpp::PlayChunk - alSourcei(source, AL_BUFFER, chunk) Failed!");

					if (chunk != nullptr) {
						alSourcei(source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
						context->Device()->ALInstance()->ReportALError("OpenALClip.cpp::PlayChunk - alSourcei(source, AL_LOOPING, looping) Failed!");

						alSourcei(source, AL_SAMPLE_OFFSET, static_cast<ALint>(sampleOffset));
						context->Device()->ALInstance()->ReportALError("OpenALClip.cpp::PlayChunk - alSourcei(source, AL_SAMPLE_OFFSET, sampleOffset) Failed!");

						alSourcePlay(source);
						context->Device()->ALInstance()->ReportALError("OpenALClip.cpp::PlayChunk - alSourcePlay(source) Failed!");
					}
				}

				inline static bool SourcePlayingNoLock(ListenerContext* context, ALuint source) {
					ALint state;
					alGetSourcei(source, AL_SOURCE_STATE, &state);
					if (context->Device()->ALInstance()->ReportALError("OpenALClip.cpp::SourcePlayingNoLock - alGetSourcei(source, AL_SOURCE_STATE, &state) Failed!") > OS::Logger::LogLevel::LOG_WARNING) return false;
					return state == AL_PLAYING;
				}

				inline static bool SourcePlaying(ListenerContext* context, ALuint source) {
					std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
					OpenALContext::SwapCurrent swap(context);
					return SourcePlayingNoLock(context, source);
				}

				inline static void SetSourceLooping(ListenerContext* context, ALuint source, bool looping) {
					std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
					OpenALContext::SwapCurrent swap(context);

					alSourcei(source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
					context->Device()->ALInstance()->ReportALError("OpenALClip.cpp::SetSourceLooping - alSourcei(source, AL_LOOPING, looping) Failed!");

					if (looping && (!SourcePlayingNoLock(context, source))) {
						alSourcePlay(source);
						context->Device()->ALInstance()->ReportALError("OpenALClip.cpp::SetSourceLooping - alSourcePlay(source) Failed!");
					}
				}

				class SimpleClipPlayback2D : public ClipPlayback2D {
				private:
					Reference<OpenALClipChunk> m_chunk;

				public:
					inline SimpleClipPlayback2D(ListenerContext* context, const AudioSource2D::Settings& settings, OpenALClipChunk* chunk, bool loop, size_t sampleOffset)
						: ClipPlayback2D(context, settings), m_chunk(chunk) {
						if (loop || (sampleOffset < chunk->SampleCount()))
							PlayChunk(Context(), Source(), m_chunk, loop, sampleOffset);
					}

					inline virtual ~SimpleClipPlayback2D() { PlayChunk(Context(), Source(), nullptr, false, 0); }

					inline virtual bool Playing() override { return SourcePlaying(Context(), Source()); }

					virtual void Loop(bool loop) override { SetSourceLooping(Context(), Source(), loop); }
				};

				class SimpleClipPlayback3D : public ClipPlayback3D {
				private:
					Reference<OpenALClipChunk> m_chunk;

				public:
					inline SimpleClipPlayback3D(ListenerContext* context, const AudioSource3D::Settings& settings, OpenALClipChunk* chunk, bool loop, size_t sampleOffset)
						: ClipPlayback3D(context, settings), m_chunk(chunk) {
						if (loop || (sampleOffset < chunk->SampleCount()))
							PlayChunk(Context(), Source(), m_chunk, loop, sampleOffset);
					}

					inline virtual ~SimpleClipPlayback3D() { PlayChunk(Context(), Source(), nullptr, false, 0); }

					inline virtual bool Playing() override { return SourcePlaying(Context(), Source()); }

					virtual void Loop(bool loop) override { SetSourceLooping(Context(), Source(), loop); }
				};

				inline static size_t SampleOffset(AudioClip* clip, float timeOffset) {
					if (clip->Buffer()->SampleCount() != AudioBuffer::InfiniteSamples()) {
						double rem = Math::FloatRemainder(timeOffset, clip->Duration());
						return static_cast<size_t>(rem * clip->Buffer()->SampleRate());
					}
					else return static_cast<size_t>(static_cast<double>(timeOffset) * clip->Buffer()->SampleRate());
				}

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
						return Object::Instantiate<SimpleClipPlayback2D>(context, settings, m_stereoChunk, loop, SampleOffset(this, timeOffset));
					}

					inline virtual Reference<ClipPlayback3D> Play3D(ListenerContext* context, const AudioSource3D::Settings& settings, bool loop, float timeOffset) override {
						if (m_monoChunk == nullptr) {
							std::unique_lock<std::mutex> lock(m_lock);
							if (m_monoChunk == nullptr) m_monoChunk = Object::Instantiate<OpenALClipChunk>(Device()->ALInstance(), Device()->DefaultContext(), Buffer(), 0, Buffer()->SampleCount(), false);
						}
						return Object::Instantiate<SimpleClipPlayback3D>(context, settings, m_monoChunk, loop, SampleOffset(this, timeOffset));
					}
				};





				/** ########################################################################################################################## */
				/** ############################################### Streamed playback ######################################################## */
				/** ########################################################################################################################## */

				class ClipChunkCache : public virtual ObjectCache<size_t> {
				private:
					const Reference<OpenALDevice> m_device;
					const Reference<const AudioBuffer> m_buffer;
					const bool m_twoDimensional;
					const size_t m_chunkCount;

#pragma warning(disable: 4250)
					class CachedChunk : public virtual OpenALClipChunk, public virtual ObjectCache<size_t>::StoredObject {
					public:
						inline CachedChunk(OpenALInstance* instance, OpenALContext* context, const AudioBuffer* buffer, size_t firstSample, size_t sampleCount, bool twoDimensional)
							: OpenALClipChunk(instance, context, buffer, firstSample, sampleCount, twoDimensional) {}
					};
#pragma warning(default: 4250)

				public:
					inline ClipChunkCache(OpenALDevice* device, const AudioBuffer* buffer, bool twoDimensional)
						: m_device(device), m_buffer(buffer), m_twoDimensional(twoDimensional)
						, m_chunkCount((buffer->SampleCount() == AudioBuffer::InfiniteSamples())
							? AudioBuffer::InfiniteSamples() : ((buffer->SampleCount() + buffer->SampleRate() - 1u) / buffer->SampleRate())) {}

					inline Reference<OpenALClipChunk> GetChunk(size_t index) {
						return GetCachedOrCreate(index, false, [&]()->Reference<CachedChunk> {
							size_t start;
							size_t end;
							if (m_chunkCount != AudioBuffer::InfiniteSamples()) {
								start = ((index % m_chunkCount) * m_buffer->SampleRate());
								end = min(start + m_buffer->SampleRate(), m_buffer->SampleCount());
							}
							else {
								start = index * m_buffer->SampleRate();
								end = start + m_buffer->SampleRate();
							}
							return Object::Instantiate<CachedChunk>(m_device->ALInstance(), m_device->DefaultContext(), m_buffer, start, end - start, m_twoDimensional);
							});
					}

					inline size_t ChunkCount()const { return m_chunkCount; }

					inline void GetChunkAndOffset(size_t sampleOffset, size_t& chunk, size_t& chunkOffset) {

					}
				};


				class StreamedClipPlayback {
				private:
					const Reference<ListenerContext> m_context;
					const Reference<ClipChunkCache> m_cache;
					ALuint m_source = 0;
					bool m_looping;
					size_t m_chunkPtr;

					size_t m_queuedChunkId = 0;
					Reference<OpenALClipChunk> m_queuedChunks[3];

					inline constexpr size_t QueuedChunkCount()const { return sizeof(m_queuedChunks) / sizeof(Reference<OpenALClipChunk>); }


					inline void QueueBuffers(size_t count) {
						// __TODO__: Implement this crap!
					}

					inline void OnTick(float time, ActionQueue<>&) {
						// __TODO__: Implement this crap!
					}

				public:
					inline StreamedClipPlayback(ListenerContext* context, ClipChunkCache* cache, bool loop, size_t firstChunk, ALint chunkSampleOffset)
						: m_context(context), m_cache(cache), m_looping(loop), m_chunkPtr(firstChunk) {}

					inline void Begin(ALuint source) {
						m_source = source;
						QueueBuffers(QueuedChunkCount());
						m_context->Device()->ALInstance()->OnTick() += Callback(&StreamedClipPlayback::OnTick, this);
					}

					inline void End() {
						m_context->Device()->ALInstance()->OnTick() -= Callback(&StreamedClipPlayback::OnTick, this);
						// __TODO__: Implement this crap!
					}

					inline void Loop(bool looping) {
						// __TODO__: Implement this crap!
					}
				};

				class StreamedClipPlayback2D : public ClipPlayback2D {
				private:
					StreamedClipPlayback m_playback;

				public:
					inline StreamedClipPlayback2D(ListenerContext* context, const AudioSource2D::Settings& settings, ClipChunkCache* cache, bool loop, size_t firstChunk, ALint chunkSampleOffset)
						: ClipPlayback2D(context, settings), m_playback(context, cache, loop, firstChunk, chunkSampleOffset) {
						m_playback.Begin(Source());
					}

					inline virtual ~StreamedClipPlayback2D() { m_playback.End(); }

					inline virtual bool Playing() override { return SourcePlaying(Context(), Source()); }

					virtual void Loop(bool loop) override { m_playback.Loop(loop); }
				};

				class StreamedClipPlayback3D : public ClipPlayback3D {
				private:
					StreamedClipPlayback m_playback;

				public:
					inline StreamedClipPlayback3D(ListenerContext* context, const AudioSource3D::Settings& settings, ClipChunkCache* cache, bool loop, size_t firstChunk, ALint chunkSampleOffset)
						: ClipPlayback3D(context, settings), m_playback(context, cache, loop, firstChunk, chunkSampleOffset) {
						m_playback.Begin(Source());
					}

					inline virtual ~StreamedClipPlayback3D() { m_playback.End(); }

					inline virtual bool Playing() override { return SourcePlaying(Context(), Source()); }

					virtual void Loop(bool loop) override { m_playback.Loop(loop); }
				};


				class StreamedClip : public virtual OpenALClip {
				private:
					std::mutex m_lock;
					Reference<ClipChunkCache> m_monoCache;
					Reference<ClipChunkCache> m_stereoCache;

				public:
					inline StreamedClip(OpenALDevice* device, const AudioBuffer* buffer) : AudioClip(buffer), OpenALClip(device, buffer) {}

					inline virtual Reference<ClipPlayback2D> Play2D(ListenerContext* context, const AudioSource2D::Settings& settings, bool loop, float timeOffset) override {
						if (m_stereoCache == nullptr) {
							std::unique_lock<std::mutex> lock(m_lock);
							if (m_stereoCache == nullptr) {
								const bool shareChunks = OpenALClipChunk::CanShare2DAnd3DChunks(Buffer());
								if (shareChunks) m_stereoCache = m_monoCache;
								if (m_stereoCache == nullptr) {
									m_stereoCache = Object::Instantiate<ClipChunkCache>(Device(), Buffer(), true);
									if (shareChunks) m_monoCache = m_stereoCache;
								}
							}
						}
						// __TODO__: Create actual playback...
						Device()->APIInstance()->Log()->Warning("StreamedClip::Play2D - Not yet implemented!");
						return nullptr;
					}

					inline virtual Reference<ClipPlayback3D> Play3D(ListenerContext* context, const AudioSource3D::Settings& settings, bool loop, float timeOffset) override {
						if (m_monoCache == nullptr) {
							std::unique_lock<std::mutex> lock(m_lock);
							if (m_monoCache == nullptr) m_monoCache = Object::Instantiate<ClipChunkCache>(Device(), Buffer(), false);
						}
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

			ClipPlayback::~ClipPlayback() { m_context->FreeSource(m_source); }

			ListenerContext* ClipPlayback::Context()const { return m_context; }

			ALuint ClipPlayback::Source()const { return m_source; }



			ClipPlayback2D::ClipPlayback2D(ListenerContext* context, const AudioSource2D::Settings& settings) : ClipPlayback(context) {
				Update(settings);
			}

			void ClipPlayback2D::Update(const AudioSource2D::Settings& settings) {
				std::unique_lock<std::mutex> lock(OpenALInstance::APILock());
				OpenALContext::SwapCurrent swap(Context());

				alSourcei(Source(), AL_SOURCE_RELATIVE, AL_TRUE);
				Context()->Device()->ALInstance()->ReportALError("ClipPlayback2D::Update - alSourcei(Source(), AL_SOURCE_RELATIVE, AL_TRUE) Failed!");

				alSource3f(Source(), AL_POSITION, 0.0f, 0.0f, 0.0f);
				Context()->Device()->ALInstance()->ReportALError("ClipPlayback2D::Update - alSource3f(Source(), AL_POSITION, 0.0f) Failed!");

				alSource3f(Source(), AL_VELOCITY, 0.0f, 0.0f, 0.0f);
				Context()->Device()->ALInstance()->ReportALError("ClipPlayback2D::Update - alSource3f(Source(), AL_VELOCITY, 0.0f) Failed!");

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

				alSourcei(Source(), AL_SOURCE_RELATIVE, AL_FALSE);
				Context()->Device()->ALInstance()->ReportALError("ClipPlayback3D::Update - alSourcei(Source(), AL_SOURCE_RELATIVE, AL_FALSE) Failed!");

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
