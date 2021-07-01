#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Audio/AudioInstance.h"
#include "Audio/Buffers/SineBuffer.h"
#include "OS/Logging/StreamLogger.h"
#include "Math/Math.h"
#include "Core/Stopwatch.h"


#include "Audio/OpenAL/OpenALDevice.h"
#include "Audio/OpenAL/OpenALClip.h"
#include <thread>


namespace Jimara {
	namespace Audio {
		namespace OpenAL {
			namespace {
				class OpenALSource : public virtual Object {
				private:
					const Reference<OpenALContext> m_context;
					const Reference<OpenALDevice> m_device;
					ALuint m_source = 0;
					bool m_sourcePresent = false;
					Reference<const OpenALClip> m_clip = nullptr;

				public:
					inline void SetPitch(float pitch) {
						if (m_sourcePresent) {
							alSourcef(m_source, AL_PITCH, pitch);
							m_device->ALInstance()->ReportALError("OpenALSource::SetPitch - Failed!");
						}
					}

					inline void SetGain(float gain) {
						if (m_sourcePresent) {
							alSourcef(m_source, AL_GAIN, gain);
							m_device->ALInstance()->ReportALError("OpenALSource::SetGain - Failed!");
						}
					}

					inline void SetPosition(const Vector3& position) {
						if (m_sourcePresent) {
							alSource3f(m_source, AL_POSITION, position.x, position.y, -position.z);
							m_device->ALInstance()->ReportALError("OpenALSource::SetPosition - Failed!");
						}
					}

					inline void SetVelocity(const Vector3& velocity) {
						if (m_sourcePresent) {
							alSource3f(m_source, AL_VELOCITY, velocity.x, velocity.y, -velocity.z);
							m_device->ALInstance()->ReportALError("OpenALSource::SetVelocity - Failed!");
						}
					}

					inline void SetLooping(bool looping) {
						if (m_sourcePresent) {
							alSourcei(m_source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
							m_device->ALInstance()->ReportALError("OpenALSource::SetLooping - Failed!");
						}
					}

					inline void SetClip(const AudioClip* clip) {
						m_clip = dynamic_cast<const OpenALClip*>(clip);
						if (m_sourcePresent) {
							alSourcei(m_source, AL_BUFFER, (m_clip == nullptr) ? static_cast<ALuint>(0u) : m_clip->Buffer());
							m_device->ALInstance()->ReportALError("OpenALSource::SetClip - Failed!");
						}
					}

					inline void Play() {
						if (m_sourcePresent) {
							alSourcePlay(m_source);
							m_device->ALInstance()->ReportALError("OpenALSource::Play - Failed!");
						}
					}

					inline bool Playing()const {
						if (m_sourcePresent) {
							ALint state;
							alGetSourcei(m_source, AL_SOURCE_STATE, &state);
							if (m_device->ALInstance()->ReportALError("OpenALSource::Play - Failed!") > OS::Logger::LogLevel::LOG_WARNING) return false;
							return state == AL_PLAYING;
						}
						else return false;
					}


					inline OpenALSource(OpenALContext* context, OpenALDevice* device) 
						: m_context(context), m_device(device) {
						alGenSources(1, &m_source);
						if (m_device->ALInstance()->ReportALError(
							"OpenALSource::OpenALSource - alGenSources() Failed!", Jimara::OS::Logger::LogLevel::LOG_FATAL) > Jimara::OS::Logger::LogLevel::LOG_WARNING) return;
						m_sourcePresent = true;
						SetPitch(1.0f);
						SetGain(1.0f);
						SetPosition(Vector3(0.0f));
						SetVelocity(Vector3(0.0f));
						SetLooping(false);
						SetClip(nullptr);
					}

					inline virtual ~OpenALSource() {
						if (m_sourcePresent) {
							alDeleteSources(1, &m_source);
							m_device->ALInstance()->ReportALError("OpenALSource::~OpenALSource - alDeleteSources() Failed!");
							m_sourcePresent = false;
						}
					}

					inline ALuint Source()const { return m_source; }
				};
			}
		}

		TEST(AudioPlayground, Playground) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			
			Reference<AudioInstance> instance = AudioInstance::Create(logger);
			ASSERT_NE(instance, nullptr);

			for (size_t i = 0; i < instance->PhysicalDeviceCount(); i++) {
				Reference<PhysicalAudioDevice> physicalDevice = instance->PhysicalDevice(i);
				ASSERT_NE(physicalDevice, nullptr);
				logger->Info(i, ". Name: <", physicalDevice->Name(), "> is default: ", physicalDevice->IsDefaultDevice() ? "true" : "false");
			}

			Reference<AudioDevice> device = instance->DefaultDevice()->CreateLogicalDevice();
			ASSERT_NE(device, nullptr);

			Reference<SineBuffer> buffer = Object::Instantiate<SineBuffer>(128.0f, 0.0f, 48000u, 960000u);
			Reference<AudioClip> clip = device->CreateAudioClip(buffer, false);
			ASSERT_NE(clip, nullptr);

			Reference<AudioScene> scene = device->CreateScene();
			ASSERT_NE(scene, nullptr);

			Reference<AudioListener> listener = scene->CreateListener(AudioListener::Settings());
			ASSERT_NE(listener, nullptr);

			listener->Update({ Math::MatrixFromEulerAngles(Vector3(0.0f, 135.0f, 0.0f)) });

			Reference<OpenAL::OpenALDevice> alDevice = device;
			ASSERT_NE(device, nullptr);

			logger->Info("MaxSources: ", alDevice->MaxSources());

			Reference<OpenAL::OpenALListener> alListener = listener;
			ASSERT_NE(alListener, nullptr);

			OpenAL::OpenALContext::SwapCurrent setContext(alListener);

			OpenAL::OpenALSource source(alListener, alDevice);

			source.SetClip(clip);
			source.SetPitch(2.0f);
			source.Play();
			Stopwatch stopwatch;
			while (source.Playing()) {
				float time = stopwatch.Elapsed();
				source.SetPosition(4.0f * Vector3(cos(time), 0.0f, sin(time)));
				source.SetVelocity(8.0f * Vector3(-sin(time), 0.0f, cos(time)));
				std::this_thread::sleep_for(std::chrono::milliseconds(8));
			}
		}
	}
}
