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

			const SineBuffer::ChannelSettings frequencies[2] = { SineBuffer::ChannelSettings(128.0f), SineBuffer::ChannelSettings(256.0f) };
			Reference<SineBuffer> buffer = Object::Instantiate<SineBuffer>(frequencies, 2, 48000u, 960000u, AudioChannelLayout::Stereo());
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

			Reference<OpenAL::OpenALClip> alClip = clip;
			ASSERT_NE(alClip, nullptr);

			logger->Info("Duration: ", alClip->Duration());

			{
				Reference<OpenAL::ClipPlayback3D> play3D = alClip->Play3D(alListener->Context(), AudioSource3D::Settings(), false, 7.0f);
				ASSERT_NE(play3D, nullptr);

				Stopwatch stopwatch;
				while (play3D->Playing()) {
					AudioSource3D::Settings settings;
					float time = stopwatch.Elapsed();
					settings.position = (4.0f * Vector3(cos(time), 0.0f, sin(time)));
					settings.velocity = (8.0f * Vector3(-sin(time), 0.0f, cos(time)));
					play3D->Update(settings);
					std::this_thread::sleep_for(std::chrono::milliseconds(8));
				}
			}

			{
				Reference<OpenAL::ClipPlayback2D> play2D = alClip->Play2D(alListener->Context(), AudioSource2D::Settings(), false, 15.0f);
				ASSERT_NE(play2D, nullptr);

				Stopwatch stopwatch;
				while (play2D->Playing());
			}

			{
				Reference<OpenAL::ClipPlayback3D> play3D = alClip->Play3D(alListener->Context(), AudioSource3D::Settings(), false, 0.0f);
				ASSERT_NE(play3D, nullptr);

				AudioSource2D::Settings settings;
				settings.pitch = 2.0f;
				Reference<OpenAL::ClipPlayback2D> play2D = alClip->Play2D(alListener->Context(), settings, false, 10.0f);
				ASSERT_NE(play2D, nullptr);

				Stopwatch stopwatch;
				while (play2D->Playing()) {
					AudioSource3D::Settings settings;
					float time = stopwatch.Elapsed();
					settings.position = (4.0f * Vector3(cos(time), 0.0f, sin(time)));
					settings.velocity = (8.0f * Vector3(-sin(time), 0.0f, cos(time)));
					play3D->Update(settings);
					std::this_thread::sleep_for(std::chrono::milliseconds(8));
				}
			}
		}
	}
}
