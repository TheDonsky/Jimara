#include "../../GtestHeaders.h"
#include "../../Memory.h"
#include "Core/Stopwatch.h"
#include "OS/Logging/StreamLogger.h"
#include "Audio/AudioDevice.h"
#include "Audio/Buffers/SineBuffer.h"
#include "Audio/Buffers/WaveBuffer.h"


namespace Jimara {
	namespace Audio {
		TEST(AudioAPITest, CreateDevice) {
			const Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();

			const Reference<AudioInstance> instance = AudioInstance::Create(logger);
			ASSERT_NE(instance, nullptr);

			for (size_t i = 0; i < instance->PhysicalDeviceCount(); i++) {
				const Reference<PhysicalAudioDevice> physicalDevice = instance->PhysicalDevice(i);
				ASSERT_NE(physicalDevice, nullptr);
				logger->Info(i, ". Name: <", physicalDevice->Name(), "> is default: ", physicalDevice->IsDefaultDevice() ? "true" : "false");
			}

			ASSERT_NE(instance->DefaultDevice(), nullptr);
			const Reference<AudioDevice> device = instance->DefaultDevice()->CreateLogicalDevice();
			ASSERT_NE(device, nullptr);
		}

		TEST(AudioAPITest, LoadWaveFile) {
			const Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();

			{
				const Reference<AudioBuffer> buffer = WaveBuffer("Assets/Audio/Mono_sub/Mono_sub_44.1_16.wav", logger);
				EXPECT_NE(buffer, nullptr);
				if (buffer != nullptr) {
					EXPECT_EQ(buffer->Format(), AudioFormat::MONO);
					EXPECT_EQ(buffer->ChannelCount(), 1);
					EXPECT_EQ(buffer->SampleRate(), 44100);
				}
			}

			{
				const Reference<AudioBuffer> buffer = WaveBuffer("Assets/Audio/Mono_sub/Mono_sub_44.1_32.wav", logger);
				EXPECT_NE(buffer, nullptr);
				if (buffer != nullptr) {
					EXPECT_EQ(buffer->Format(), AudioFormat::MONO);
					EXPECT_EQ(buffer->ChannelCount(), 1);
					EXPECT_EQ(buffer->SampleRate(), 44100);
				}
			}

			{
				const Reference<AudioBuffer> buffer = WaveBuffer("Assets/Audio/Mono_sub/Mono_sub_48_16.wav", logger);
				EXPECT_NE(buffer, nullptr);
				if (buffer != nullptr) {
					EXPECT_EQ(buffer->Format(), AudioFormat::MONO);
					EXPECT_EQ(buffer->ChannelCount(), 1);
					EXPECT_EQ(buffer->SampleRate(), 48000);
				}
			}

			{
				const Reference<AudioBuffer> buffer = WaveBuffer("Assets/Audio/Mono_sub/Mono_sub_48_32.wav", logger);
				EXPECT_NE(buffer, nullptr);
				if (buffer != nullptr) {
					EXPECT_EQ(buffer->Format(), AudioFormat::MONO);
					EXPECT_EQ(buffer->ChannelCount(), 1);
					EXPECT_EQ(buffer->SampleRate(), 48000);
				}
			}

			{
				const Reference<AudioBuffer> buffer = WaveBuffer("Assets/Audio/Mono_sub/Mono_sub_88.2_16.wav", logger);
				EXPECT_NE(buffer, nullptr);
				if (buffer != nullptr) {
					EXPECT_EQ(buffer->Format(), AudioFormat::MONO);
					EXPECT_EQ(buffer->ChannelCount(), 1);
					EXPECT_EQ(buffer->SampleRate(), 88200);
				}
			}

			{
				const Reference<AudioBuffer> buffer = WaveBuffer("Assets/Audio/Mono_sub/Mono_sub_96_32.wav", logger);
				EXPECT_NE(buffer, nullptr);
				if (buffer != nullptr) {
					EXPECT_EQ(buffer->Format(), AudioFormat::MONO);
					EXPECT_EQ(buffer->ChannelCount(), 1);
					EXPECT_EQ(buffer->SampleRate(), 96000);
				}
			}

			{
				const Reference<AudioBuffer> buffer = WaveBuffer("Assets/Audio/Mono_sub/Mono_sub_192_16.wav", logger);
				EXPECT_NE(buffer, nullptr);
				if (buffer != nullptr) {
					EXPECT_EQ(buffer->Format(), AudioFormat::MONO);
					EXPECT_EQ(buffer->ChannelCount(), 1);
					EXPECT_EQ(buffer->SampleRate(), 192000);
				}
			}

			{
				const Reference<AudioBuffer> buffer = WaveBuffer("Assets/Audio/Tracks/Track 1 Mono 88.2KHz 16 bit.wav", logger);
				EXPECT_NE(buffer, nullptr);
				if (buffer != nullptr) {
					EXPECT_EQ(buffer->Format(), AudioFormat::MONO);
					EXPECT_EQ(buffer->ChannelCount(), 1);
					EXPECT_EQ(buffer->SampleRate(), 88200);
				}
			}

			{
				const Reference<AudioBuffer> buffer = WaveBuffer("Assets/Audio/Tracks/Track 1 Mono 96KHz 16 bit.wav", logger);
				EXPECT_NE(buffer, nullptr);
				if (buffer != nullptr) {
					EXPECT_EQ(buffer->Format(), AudioFormat::MONO);
					EXPECT_EQ(buffer->ChannelCount(), 1);
					EXPECT_EQ(buffer->SampleRate(), 96000);
				}
			}

			{
				const Reference<AudioBuffer> buffer = WaveBuffer("Assets/Audio/Tracks/Track 1 Stereo 88.2KHz 16 bit.wav", logger);
				EXPECT_NE(buffer, nullptr);
				if (buffer != nullptr) {
					EXPECT_EQ(buffer->Format(), AudioFormat::STEREO);
					EXPECT_EQ(buffer->ChannelCount(), 2);
					EXPECT_EQ(buffer->SampleRate(), 88200);
				}
			}

			{
				const Reference<AudioBuffer> buffer = WaveBuffer("Assets/Audio/Tracks/Track 1 Stereo 96KHz 16 bit.wav", logger);
				EXPECT_NE(buffer, nullptr);
				if (buffer != nullptr) {
					EXPECT_EQ(buffer->Format(), AudioFormat::STEREO);
					EXPECT_EQ(buffer->ChannelCount(), 2);
					EXPECT_EQ(buffer->SampleRate(), 96000);
				}
			}

			{
				const Reference<AudioBuffer> buffer = WaveBuffer("Assets/Audio/Tracks/Track 2 Mono 88.2KHz 16 bit.wav", logger);
				EXPECT_NE(buffer, nullptr);
				if (buffer != nullptr) {
					EXPECT_EQ(buffer->Format(), AudioFormat::MONO);
					EXPECT_EQ(buffer->ChannelCount(), 1);
					EXPECT_EQ(buffer->SampleRate(), 88200);
				}
			}

			{
				const Reference<AudioBuffer> buffer = WaveBuffer("Assets/Audio/Tracks/Track 2 Mono 96KHz 16 bit.wav", logger);
				EXPECT_NE(buffer, nullptr);
				if (buffer != nullptr) {
					EXPECT_EQ(buffer->Format(), AudioFormat::MONO);
					EXPECT_EQ(buffer->ChannelCount(), 1);
					EXPECT_EQ(buffer->SampleRate(), 96000);
				}
			}

			{
				const Reference<AudioBuffer> buffer = WaveBuffer("Assets/Audio/Tracks/Track 2 Stereo 88.2KHz 16 bit.wav", logger);
				EXPECT_NE(buffer, nullptr);
				if (buffer != nullptr) {
					EXPECT_EQ(buffer->Format(), AudioFormat::STEREO);
					EXPECT_EQ(buffer->ChannelCount(), 2);
					EXPECT_EQ(buffer->SampleRate(), 88200);
				}
			}

			{
				const Reference<AudioBuffer> buffer = WaveBuffer("Assets/Audio/Tracks/Track 2 Stereo 96KHz 16 bit.wav", logger);
				EXPECT_NE(buffer, nullptr);
				if (buffer != nullptr) {
					EXPECT_EQ(buffer->Format(), AudioFormat::STEREO);
					EXPECT_EQ(buffer->ChannelCount(), 2);
					EXPECT_EQ(buffer->SampleRate(), 96000);
				}
			}
		}

		TEST(AudioAPITest, Play_NoListener) {
			const Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();

			const Reference<AudioInstance> instance = AudioInstance::Create(logger);
			ASSERT_NE(instance, nullptr);

			ASSERT_NE(instance->DefaultDevice(), nullptr);
			const Reference<AudioDevice> device = instance->DefaultDevice()->CreateLogicalDevice();
			ASSERT_NE(device, nullptr);

			const Reference<AudioScene> scene = device->CreateScene();
			ASSERT_NE(scene, nullptr);

			const Reference<AudioBuffer> buffer = Object::Instantiate<SineBuffer>(SineBuffer::ChannelSettings(256.0f), 48000, 48000);
			
			{
				const Reference<AudioClip> clip = device->CreateAudioClip(buffer, false);
				ASSERT_NE(clip, nullptr);
				EXPECT_LT(abs(clip->Duration() - 1.0f), 0.0001f);

				{
					const Reference<AudioSource> source = scene->CreateSource2D(AudioSource2D::Settings(), clip);
					ASSERT_NE(source, nullptr);
					const Stopwatch stopwatch;
					source->Play();
					while (source->State() == AudioSource::PlaybackState::PLAYING);
					EXPECT_EQ(source->State(), AudioSource::PlaybackState::FINISHED);
					EXPECT_LT(abs(stopwatch.Elapsed() - clip->Duration()), 0.05f);
				}

				{
					AudioSource3D::Settings settings;
					settings.pitch = 2.0f;
					const Reference<AudioSource> source = scene->CreateSource3D(settings, clip);
					ASSERT_NE(source, nullptr);
					const Stopwatch stopwatch;
					source->Play();
					while (source->State() == AudioSource::PlaybackState::PLAYING);
					EXPECT_EQ(source->State(), AudioSource::PlaybackState::FINISHED);
					EXPECT_LT(abs(stopwatch.Elapsed() - clip->Duration() / settings.pitch), 0.05f);
				}
			}

			{
				const Reference<AudioClip> clip = device->CreateAudioClip(buffer, true);
				ASSERT_NE(clip, nullptr);
				EXPECT_LT(abs(clip->Duration() - 1.0f), 0.0001f);

				{
					AudioSource2D::Settings settings;
					settings.pitch = 2.0f;
					const Stopwatch stopwatch;
					{
						const Reference<AudioSource> source = scene->CreateSource2D(settings, clip);
						ASSERT_NE(source, nullptr);
						source->Play();
						std::this_thread::sleep_for(std::chrono::milliseconds((uint32_t)(clip->Duration() * 1000.0f / settings.pitch / 2.0f)));
					}
					EXPECT_LT(abs(stopwatch.Elapsed() - clip->Duration() / settings.pitch / 2.0f), 0.05f);
				}

				{
					AudioSource3D::Settings settings;
					const Stopwatch stopwatch;
					{
						const Reference<AudioSource> source = scene->CreateSource3D(settings, clip);
						ASSERT_NE(source, nullptr);
						source->Play();
						std::this_thread::sleep_for(std::chrono::milliseconds((uint32_t)(clip->Duration() * 1000.0f / 2.0f)));
					}
					EXPECT_LT(abs(stopwatch.Elapsed() - clip->Duration() / 2.0f), 0.05f);
				}
			}
		}

		TEST(AudioAPITest, Play_SingleListener) {
			const Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();

			const Reference<AudioInstance> instance = AudioInstance::Create(logger);
			ASSERT_NE(instance, nullptr);

			ASSERT_NE(instance->DefaultDevice(), nullptr);
			const Reference<AudioDevice> device = instance->DefaultDevice()->CreateLogicalDevice();
			ASSERT_NE(device, nullptr);

			const Reference<AudioScene> scene = device->CreateScene();
			ASSERT_NE(scene, nullptr);

			const Reference<AudioListener> listener = scene->CreateListener(AudioListener::Settings());
			EXPECT_NE(listener, nullptr);

			const SineBuffer::ChannelSettings channelSettings[2] = {
				SineBuffer::ChannelSettings(256.0f),
				SineBuffer::ChannelSettings(512.0f)
			};
			const Reference<AudioBuffer> buffer = Object::Instantiate<SineBuffer>(channelSettings, 48000, 48000, AudioFormat::STEREO);

			{
				const Reference<AudioClip> clip = device->CreateAudioClip(buffer, false);
				ASSERT_NE(clip, nullptr);
				EXPECT_LT(abs(clip->Duration() - 1.0f), 0.0001f);

				{
					const Reference<AudioSource> source = scene->CreateSource2D(AudioSource2D::Settings(), clip);
					ASSERT_NE(source, nullptr);
					const Stopwatch stopwatch;
					source->Play();
					while (source->State() == AudioSource::PlaybackState::PLAYING);
					EXPECT_EQ(source->State(), AudioSource::PlaybackState::FINISHED);
					EXPECT_LT(abs(stopwatch.Elapsed() - clip->Duration()), 0.05f);
				}

				{
					AudioSource3D::Settings settings;
					const Reference<AudioSource3D> source = scene->CreateSource3D(settings, clip);
					ASSERT_NE(source, nullptr);
					const Stopwatch stopwatch;
					source->Play();
					Stopwatch frameTime;
					while (source->State() == AudioSource::PlaybackState::PLAYING) {
						std::this_thread::sleep_for(std::chrono::milliseconds(2));
						const Vector3 oldPos = settings.position;
						const float time = stopwatch.Elapsed();
						settings.position = Vector3(cos(time), 0.0f, sin(time));
						settings.velocity = (oldPos - settings.position) / frameTime.Reset();
						source->Update(settings);
					}
					EXPECT_EQ(source->State(), AudioSource::PlaybackState::FINISHED);
					EXPECT_LT(abs(stopwatch.Elapsed() - clip->Duration()), 0.05f);
				}
			}

			{
				const Reference<AudioClip> clip = device->CreateAudioClip(buffer, true);
				ASSERT_NE(clip, nullptr);
				EXPECT_LT(abs(clip->Duration() - 1.0f), 0.0001f);

				{
					AudioSource2D::Settings settings;
					settings.pitch = 2.0f;
					const Stopwatch stopwatch;
					{
						const Reference<AudioSource> source = scene->CreateSource2D(settings, clip);
						ASSERT_NE(source, nullptr);
						source->Play();
						std::this_thread::sleep_for(std::chrono::milliseconds((uint32_t)(clip->Duration() * 1000.0f / settings.pitch / 2.0f)));
					}
					EXPECT_LT(abs(stopwatch.Elapsed() - clip->Duration() / settings.pitch / 2.0f), 0.05f);
				}

				{
					AudioSource3D::Settings settings;
					const Stopwatch stopwatch;
					{
						const Reference<AudioSource> source = scene->CreateSource3D(settings, clip);
						ASSERT_NE(source, nullptr);
						source->Play();
						std::this_thread::sleep_for(std::chrono::milliseconds((uint32_t)(clip->Duration() * 1000.0f / 2.0f)));
					}
					EXPECT_LT(abs(stopwatch.Elapsed() - clip->Duration() / 2.0f), 0.05f);
				}
			}
		}
	}
}
