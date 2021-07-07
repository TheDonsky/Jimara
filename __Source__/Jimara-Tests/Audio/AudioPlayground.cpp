#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Audio/AudioInstance.h"
#include "Audio/Buffers/SineBuffer.h"
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

			const SineBuffer::ChannelSettings frequencies[] = { 
				SineBuffer::ChannelSettings(256.0f),
				SineBuffer::ChannelSettings(256.0f),
				SineBuffer::ChannelSettings(256.0f),
				SineBuffer::ChannelSettings(256.0f),
				SineBuffer::ChannelSettings(256.0f),
				SineBuffer::ChannelSettings(256.0f)
			};
			Reference<SineBuffer> buffer = Object::Instantiate<SineBuffer>(frequencies, sizeof(frequencies) / sizeof(SineBuffer::ChannelSettings), 48000u, 240000u, AudioFormat::SURROUND_5_1);
			Reference<AudioClip> clip = device->CreateAudioClip(buffer, false);
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
					if (elapsed > 32.0f) break;
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
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(1.0f, 1.0f, 1.0f)), "Light", Vector3(2.5f, 2.5f, 2.5f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-1.0f, 1.0f, 1.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(1.0f, 1.0f, -1.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-1.0f, 1.0f, -1.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
				});

			Reference<Material> material = createMaterial();

			environment.ExecuteOnUpdateNow([&] {
				Reference<TriMesh> mesh = TriMesh::Sphere(Vector3(0.0f), 0.25f, 32.0f, 16.0f);
				Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Center Transform");
				Reference<Rigidbody> transformBody = Object::Instantiate<Rigidbody>(transform, "Center Body");
				transformBody->SetKinematic(true);
				Object::Instantiate<MeshRenderer>(transformBody, "Center Renderer", mesh, material);
				Object::Instantiate<Jimara::AudioListener>(transformBody, "Center Listener");
				});

			Reference<TriMesh> speaker = TriMesh::Box(Vector3(-0.5f), Vector3(0.5f));

			auto addSpeakerRenderer = [&](Vector3 position, Vector3 scale) {
				Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Speaker Transform", position, Vector3(0.0f), scale);
				transform->LookAt(Vector3(0.0f));
				Object::Instantiate<MeshRenderer>(transform, "Speaker Renderer", speaker, material);
			};

			environment.ExecuteOnUpdateNow([&] {
				addSpeakerRenderer(Vector3(-1.5f, 0.25f, 1.5f), Vector3(0.15f, 0.3f, 0.2f));
				addSpeakerRenderer(Vector3(1.5f, 0.25f, 1.5f), Vector3(0.15f, 0.3f, 0.2f));
				addSpeakerRenderer(Vector3(0.0f, 0.25f, 1.5f), Vector3(0.3f, 0.15f, 0.2f));
				addSpeakerRenderer(Vector3(0.5f, 0.0f, 1.5f), Vector3(0.4f, 0.4f, 0.4f));
				addSpeakerRenderer(Vector3(-1.5f, 0.5f, -1.0f), Vector3(0.15f, 0.3f, 0.2f));
				addSpeakerRenderer(Vector3(1.5f, 0.5f, -1.0f), Vector3(0.15f, 0.3f, 0.2f));
				});


			Reference<SineBuffer> buffer = Object::Instantiate<SineBuffer>(100.0f, 48000u, 240000u);
			Reference<AudioClip> clip = environment.RootObject()->Context()->AudioScene()->Device()->CreateAudioClip(buffer, true);
			ASSERT_NE(clip, nullptr);

			environment.ExecuteOnUpdateNow([&] {
				Reference<TriMesh> mesh = TriMesh::Sphere(Vector3(0.0f), 0.1f, 16.0f, 8.0f);
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
