#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Audio/AudioInstance.h"
#include "Audio/Buffers/SineBuffer.h"
#include "OS/Logging/StreamLogger.h"
#include "Math/Math.h"
#include "Core/Stopwatch.h"
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

			const SineBuffer::ChannelSettings frequencies[2] = { SineBuffer::ChannelSettings(128.0f), SineBuffer::ChannelSettings(512.0f) };
			Reference<SineBuffer> buffer = Object::Instantiate<SineBuffer>(frequencies, 2, 48000u, 48000u, AudioFormat::STEREO);
			Reference<AudioClip> clip = device->CreateAudioClip(buffer, true);
			ASSERT_NE(clip, nullptr);

			Reference<AudioScene> scene = device->CreateScene();
			ASSERT_NE(scene, nullptr);

			Reference<AudioListener> listener = scene->CreateListener(AudioListener::Settings());
			ASSERT_NE(listener, nullptr);

			listener->Update({ Math::MatrixFromEulerAngles(Vector3(0.0f, 135.0f, 0.0f)) });

			logger->Info("Duration: ", clip->Duration());

			{
				Reference<AudioSource> source2D = scene->CreateSource2D(AudioSource2D::Settings(), clip);
				ASSERT_NE(source2D, nullptr);
				Stopwatch stopwatch;
				source2D->Play();
				while (source2D->State() == AudioSource::PlaybackState::PLAYING);
				EXPECT_GE(stopwatch.Elapsed() + 0.1f, clip->Duration());

				std::this_thread::sleep_for(std::chrono::milliseconds(1024));

				stopwatch.Reset();
				source2D->Play();
				while (source2D->State() == AudioSource::PlaybackState::PLAYING);
				EXPECT_GE(stopwatch.Elapsed() + 0.1f, clip->Duration());

				std::this_thread::sleep_for(std::chrono::milliseconds(1024));
			}

			{
				Reference<AudioSource3D> source3D = scene->CreateSource3D(AudioSource3D::Settings(), clip);
				ASSERT_NE(source3D, nullptr);
				source3D->SetLooping(true);
				Stopwatch stopwatch;
				source3D->Play();
				while (true) {
					float elapsed = stopwatch.Elapsed();
					if (elapsed > 8.0f) break;
					AudioSource3D::Settings settings;
					float time = elapsed * 8.0f;
					settings.position = (4.0f * Vector3(cos(time), 0.0f, sin(time)));
					settings.velocity = (8.0f * Vector3(-sin(time), 0.0f, cos(time)));
					source3D->Update(settings);
				}
			}
		}
	}
}
