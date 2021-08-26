#include "../../GtestHeaders.h"
#include "../../Memory.h"
#include "Audio/AudioInstance.h"
#include "Audio/Buffers/SineBuffer.h"
#include "Audio/Buffers/WaveBuffer.h"
#include "OS/Logging/StreamLogger.h"
#include "Math/Math.h"
#include "Core/Stopwatch.h"
#include <thread>

#include "../TestEnvironment/TestEnvironment.h"
#include "../Shaders/SampleDiffuseShader.h"
#include "Components/Audio/AudioListener.h"
#include "Components/Audio/AudioSource.h"
#include "Components/Lights/PointLight.h"
#include "Components/Physics/Rigidbody.h"
#include "Components/MeshRenderer.h"


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

			/*
			const SineBuffer::ChannelSettings frequencies[] = { 
				SineBuffer::ChannelSettings(256.0f),
				SineBuffer::ChannelSettings(512.0f),
				SineBuffer::ChannelSettings(768.0f),
				SineBuffer::ChannelSettings(64.0f),
				SineBuffer::ChannelSettings(128.0f),
				SineBuffer::ChannelSettings(196.0f)
			};
			Reference<SineBuffer> buffer = Object::Instantiate<SineBuffer>(frequencies, 48000u, 240000u, AudioFormat::SURROUND_5_1);
			/*/
			Reference<AudioBuffer> buffer_Mono_sub_44_1_16 = WaveBuffer("Assets/Audio/Mono_sub/Mono_sub_44.1_16.wav", logger);
			EXPECT_NE(buffer_Mono_sub_44_1_16, nullptr);
			if (buffer_Mono_sub_44_1_16 != nullptr) {
				EXPECT_EQ(buffer_Mono_sub_44_1_16->Format(), AudioFormat::MONO);
				EXPECT_EQ(buffer_Mono_sub_44_1_16->ChannelCount(), 1);
				EXPECT_EQ(buffer_Mono_sub_44_1_16->SampleRate(), 44100);
			}

			Reference<AudioBuffer> buffer_Mono_sub_44_1_32 = WaveBuffer("Assets/Audio/Mono_sub/Mono_sub_44.1_32.wav", logger);
			EXPECT_NE(buffer_Mono_sub_44_1_32, nullptr);
			if (buffer_Mono_sub_44_1_32 != nullptr) {
				EXPECT_EQ(buffer_Mono_sub_44_1_32->Format(), AudioFormat::MONO);
				EXPECT_EQ(buffer_Mono_sub_44_1_32->ChannelCount(), 1);
				EXPECT_EQ(buffer_Mono_sub_44_1_32->SampleRate(), 44100);
			}

			Reference<AudioBuffer> buffer_Mono_sub_48_16 = WaveBuffer("Assets/Audio/Mono_sub/Mono_sub_48_16.wav", logger);
			EXPECT_NE(buffer_Mono_sub_48_16, nullptr);
			if (buffer_Mono_sub_48_16 != nullptr) {
				EXPECT_EQ(buffer_Mono_sub_48_16->Format(), AudioFormat::MONO);
				EXPECT_EQ(buffer_Mono_sub_48_16->ChannelCount(), 1);
				EXPECT_EQ(buffer_Mono_sub_48_16->SampleRate(), 48000);
			}

			Reference<AudioBuffer> buffer_Mono_sub_48_32 = WaveBuffer("Assets/Audio/Mono_sub/Mono_sub_48_32.wav", logger);
			EXPECT_NE(buffer_Mono_sub_48_32, nullptr);
			if (buffer_Mono_sub_48_32 != nullptr) {
				EXPECT_EQ(buffer_Mono_sub_48_32->Format(), AudioFormat::MONO);
				EXPECT_EQ(buffer_Mono_sub_48_32->ChannelCount(), 1);
				EXPECT_EQ(buffer_Mono_sub_48_32->SampleRate(), 48000);
			}

			Reference<AudioBuffer> buffer_Mono_sub_88_2_16 = WaveBuffer("Assets/Audio/Mono_sub/Mono_sub_88.2_16.wav", logger);
			EXPECT_NE(buffer_Mono_sub_88_2_16, nullptr);
			if (buffer_Mono_sub_88_2_16 != nullptr) {
				EXPECT_EQ(buffer_Mono_sub_88_2_16->Format(), AudioFormat::MONO);
				EXPECT_EQ(buffer_Mono_sub_88_2_16->ChannelCount(), 1);
				EXPECT_EQ(buffer_Mono_sub_88_2_16->SampleRate(), 88200);
			}

			Reference<AudioBuffer> buffer_Mono_sub_96_32 = WaveBuffer("Assets/Audio/Mono_sub/Mono_sub_96_32.wav", logger);
			EXPECT_NE(buffer_Mono_sub_96_32, nullptr);
			if (buffer_Mono_sub_96_32 != nullptr) {
				EXPECT_EQ(buffer_Mono_sub_96_32->Format(), AudioFormat::MONO);
				EXPECT_EQ(buffer_Mono_sub_96_32->ChannelCount(), 1);
				EXPECT_EQ(buffer_Mono_sub_96_32->SampleRate(), 96000);
			}

			Reference<AudioBuffer> buffer_Mono_sub_192_16 = WaveBuffer("Assets/Audio/Mono_sub/Mono_sub_192_16.wav", logger);
			EXPECT_NE(buffer_Mono_sub_192_16, nullptr);
			if (buffer_Mono_sub_192_16 != nullptr) {
				EXPECT_EQ(buffer_Mono_sub_192_16->Format(), AudioFormat::MONO);
				EXPECT_EQ(buffer_Mono_sub_192_16->ChannelCount(), 1);
				EXPECT_EQ(buffer_Mono_sub_192_16->SampleRate(), 192000);
			}

			/**/
			Reference<AudioClip> clip = device->CreateAudioClip(buffer_Mono_sub_192_16, true);
			ASSERT_NE(clip, nullptr);

			Reference<AudioScene> scene = device->CreateScene();
			ASSERT_NE(scene, nullptr);

			Reference<AudioListener> listener = scene->CreateListener(AudioListener::Settings());
			ASSERT_NE(listener, nullptr);

			listener->Update({ Math::MatrixFromEulerAngles(Vector3(0.0f, 135.0f, 0.0f)) });

			logger->Info("Duration: ", clip->Duration());

			{
				AudioSource2D::Settings settings;
				settings.pitch = 48.0f;
				Reference<AudioSource2D> source2D = scene->CreateSource2D(settings, clip);
				ASSERT_NE(source2D, nullptr);
				
				Stopwatch stopwatch;
				source2D->Play();
				while (source2D->State() == AudioSource::PlaybackState::PLAYING && stopwatch.Elapsed() < 1.0f);
				EXPECT_GE(stopwatch.Elapsed() + 0.1f, min(1.0f, clip->Duration() / settings.pitch));

				std::this_thread::sleep_for(std::chrono::milliseconds(1024));

				stopwatch.Reset();
				source2D->Play();
				while (source2D->State() == AudioSource::PlaybackState::PLAYING && stopwatch.Elapsed() < 1.0f);
				EXPECT_GE(stopwatch.Elapsed() + 0.1f, min(1.0f, clip->Duration() / settings.pitch));

				std::this_thread::sleep_for(std::chrono::milliseconds(1024));

				logger->Info("Stopping source2D....");
				source2D->Stop();
				logger->Info("source2D stopped!");
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
					float time = elapsed * 4.0f;
					settings.position = (4.0f * Vector3(cos(time), 0.0f, sin(time)));
					settings.velocity = (2.0f * Vector3(-sin(time), 0.0f, cos(time)));
					source3D->Update(settings);
				}
			}
		}
	}
}
