#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Audio/AudioInstance.h"
#include "Audio/Buffers/SineBuffer.h"
#include "Audio/Buffers/WaveBuffer.h"
#include "OS/Logging/StreamLogger.h"
#include "Math/Math.h"
#include "Core/Stopwatch.h"
#include <thread>

#include "../Components/TestEnvironment/TestEnvironment.h"
#include "../Components/Shaders/SampleDiffuseShader.h"
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
				settings.pitch = 16.0f;
				Reference<AudioSource2D> source2D = scene->CreateSource2D(settings, clip);
				ASSERT_NE(source2D, nullptr);
				
				Stopwatch stopwatch;
				source2D->Play();
				while (source2D->State() == AudioSource::PlaybackState::PLAYING);
				EXPECT_GE(stopwatch.Elapsed() + 0.1f, clip->Duration() / settings.pitch);

				std::this_thread::sleep_for(std::chrono::milliseconds(1024));

				stopwatch.Reset();
				source2D->Play();
				while (source2D->State() == AudioSource::PlaybackState::PLAYING);
				EXPECT_GE(stopwatch.Elapsed() + 0.1f, clip->Duration() / settings.pitch);

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
					float time = elapsed * 4.0f;
					settings.position = (4.0f * Vector3(cos(time), 0.0f, sin(time)));
					settings.velocity = (2.0f * Vector3(-sin(time), 0.0f, cos(time)));
					source3D->Update(settings);
				}
			}
		}


		namespace {
			class Circler : public virtual Component, public virtual Updatable {
			private:
				const Vector3 m_center;
				const float m_radius;
				const float m_rotationSpeed;
				const Stopwatch m_timer;

			public:
				inline Circler(Component* parent, const std::string_view& name, Vector3 center, float radius, float rotationSpeed)
					: Component(parent, name), m_center(center), m_radius(radius), m_rotationSpeed(rotationSpeed) {}

				virtual void Update()override {
					Rigidbody* body = GetComponentInParents<Rigidbody>();
					Transform* transform = (body == nullptr) ? GetTransfrom() : body->GetTransfrom();
					float elapsed = m_timer.Elapsed();
					float time = elapsed * m_rotationSpeed;
					if (transform != nullptr) transform->SetWorldPosition(m_radius * Vector3(cos(time), 0.0f, sin(time)));
					if (body != nullptr) body->SetVelocity(m_rotationSpeed * m_radius * Vector3(-sin(time), 0.0f, cos(time)));
				}
			};
		}

		TEST(AudioPlayground, Circling) {
			Jimara::Test::TestEnvironment environment("AudioPlayground: Circling");

			auto createMaterial = [&](uint32_t color = 0xFFFFFFFF) -> Reference<Material> {
				Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
				(*static_cast<uint32_t*>(texture->Map())) = color;
				texture->Unmap(true);
				return Jimara::Test::SampleDiffuseShader::CreateMaterial(texture);
			};

			environment.ExecuteOnUpdateNow([&]() {
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, 2.0f)), "Light", Vector3(2.0f, 0.25f, 0.25f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, -2.0f)), "Light", Vector3(0.25f, 2.0f, 0.25f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, 2.0f)), "Light", Vector3(0.25f, 0.25f, 2.0f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, -2.0f)), "Light", Vector3(2.0f, 4.0f, 1.0f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 2.0f, 0.0f)), "Light", Vector3(1.0f, 4.0f, 2.0f));
				});

			Reference<Material> material = createMaterial();

			environment.ExecuteOnUpdateNow([&] {
				Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Center Transform");
				Reference<Rigidbody> transformBody = Object::Instantiate<Rigidbody>(transform, "Center Body");
				transformBody->SetKinematic(true);
				Reference<TriMesh> sphere = TriMesh::Sphere(Vector3(0.0f), 0.25f, 32, 16);
				Object::Instantiate<MeshRenderer>(transformBody, "Center Renderer", sphere, material);

				Reference<TriMesh> forward = TriMesh::Box(Vector3(-0.05f), Vector3(0.05f));
				Reference<Transform> forwardTransform = Object::Instantiate<Transform>(transform, "Forward Transform");
				forwardTransform->SetWorldPosition(transform->WorldPosition() + transform->Forward() * 0.35f);
				forwardTransform->SetLocalScale(Vector3(1.0f, 1.0f, 2.5f));
				Object::Instantiate<MeshRenderer>(forwardTransform, "Forward Renderer", forward, material);

				Object::Instantiate<Jimara::AudioListener>(transformBody, "Center Listener");
				});

			Reference<TriMesh> speaker = TriMesh::Box(Vector3(-0.5f), Vector3(0.5f));

			auto addSpeakerRenderer = [&](Vector3 position, Vector3 scale, bool look = true) {
				Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Speaker Transform", position, Vector3(0.0f), scale);
				if(look) transform->LookAt(Vector3(0.0f));
				Object::Instantiate<MeshRenderer>(transform, "Speaker Renderer", speaker, material);
			};

			environment.ExecuteOnUpdateNow([&] {
				addSpeakerRenderer(Vector3(-1.5f, 0.25f, 1.5f), Vector3(0.15f, 0.3f, 0.2f));
				addSpeakerRenderer(Vector3(1.5f, 0.25f, 1.5f), Vector3(0.15f, 0.3f, 0.2f));
				addSpeakerRenderer(Vector3(0.0f, 0.25f, 1.5f), Vector3(0.3f, 0.15f, 0.2f));
				addSpeakerRenderer(Vector3(0.5f, 0.0f, 1.5f), Vector3(0.4f, 0.4f, 0.4f), false);
				addSpeakerRenderer(Vector3(-1.5f, 0.5f, -1.0f), Vector3(0.15f, 0.3f, 0.2f));
				addSpeakerRenderer(Vector3(1.5f, 0.5f, -1.0f), Vector3(0.15f, 0.3f, 0.2f));
				});


			Reference<SineBuffer> buffer = Object::Instantiate<SineBuffer>(256.0f, 48000u, 240000u);
			Reference<AudioClip> clip = environment.RootObject()->Context()->AudioScene()->Device()->CreateAudioClip(buffer, false);
			ASSERT_NE(clip, nullptr);

			environment.ExecuteOnUpdateNow([&] {
				Reference<TriMesh> mesh = TriMesh::Sphere(Vector3(0.0f), 0.1f, 16, 8);
				Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Moving Transform");
				Reference<Rigidbody> transformBody = Object::Instantiate<Rigidbody>(transform, "Moving Body");
				transformBody->SetKinematic(true);
				Object::Instantiate<MeshRenderer>(transformBody, "Moving Renderer", mesh, material);
				Reference<Jimara::AudioSource3D> source = Object::Instantiate<Jimara::AudioSource3D>(transformBody, "Moving source", clip);
				source->SetLooping(true);
				source->Play();
				Object::Instantiate<Circler>(transformBody, "Moving Circler", Vector3(0.0f, 0.0f, 0.25f), 2.0f, 1.0f);
				});
		}
	}
}
