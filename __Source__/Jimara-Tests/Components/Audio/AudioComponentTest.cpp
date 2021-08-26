#include "../../GtestHeaders.h"
#include "../../Memory.h"

#include "../TestEnvironment/TestEnvironment.h"
#include "../Shaders/SampleDiffuseShader.h"

#include "Audio/Buffers/SineBuffer.h"

#include "Components/Interfaces/Updatable.h"
#include "Components/Audio/AudioListener.h"
#include "Components/Audio/AudioSource.h"
#include "Components/Physics/Rigidbody.h"
#include "Components/Lights/PointLight.h"
#include "Components/MeshRenderer.h"


namespace Jimara {
	namespace Audio {
		namespace {
			inline static Reference<Material> CreateMaterial(SceneContext* context, uint32_t color = 0xFFFFFFFF) {
				Reference<Graphics::ImageTexture> texture = context->Graphics()->Device()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
				(*static_cast<uint32_t*>(texture->Map())) = color;
				texture->Unmap(true);
				return Jimara::Test::SampleDiffuseShader::CreateMaterial(texture);
			}

			inline static Reference<Component> Add_5_1_Representation(Component* parentObject, uint32_t color = 0xFFFFFFFF) {
				Reference<TriMesh> speaker = TriMesh::Box(Vector3(-0.5f), Vector3(0.5f));
				Reference<Material> material = CreateMaterial(parentObject->Context());

				Reference<Component> repr5_1 = Object::Instantiate<Component>(parentObject, "5.1 Representation");

				auto addSpeakerRenderer = [&](Vector3 position, Vector3 scale, bool look = true) {
					Reference<Transform> transform = Object::Instantiate<Transform>(repr5_1, "Speaker Transform", position, Vector3(0.0f), scale);
					if (look) transform->LookAt(Vector3(0.0f));
					Object::Instantiate<MeshRenderer>(transform, "Speaker Renderer", speaker, material);
				};

				addSpeakerRenderer(Vector3(-1.5f, 0.25f, 1.5f), Vector3(0.15f, 0.3f, 0.2f));
				addSpeakerRenderer(Vector3(1.5f, 0.25f, 1.5f), Vector3(0.15f, 0.3f, 0.2f));
				addSpeakerRenderer(Vector3(0.0f, 0.25f, 1.5f), Vector3(0.3f, 0.15f, 0.2f));
				addSpeakerRenderer(Vector3(0.5f, 0.0f, 1.5f), Vector3(0.4f, 0.4f, 0.4f), false);
				addSpeakerRenderer(Vector3(-1.5f, 0.5f, -1.0f), Vector3(0.15f, 0.3f, 0.2f));
				addSpeakerRenderer(Vector3(1.5f, 0.5f, -1.0f), Vector3(0.15f, 0.3f, 0.2f));

				return repr5_1;
			}

			inline static Reference<Component> Add_5_1_Representation(Jimara::Test::TestEnvironment& environment, Component* parentObject = nullptr, uint32_t color = 0xFFFFFFFF) {
				Reference<Component> repr5_1;
				environment.ExecuteOnUpdateNow([&]() { repr5_1 = Add_5_1_Representation(parentObject == nullptr ? environment.RootObject() : parentObject, color); });
				return repr5_1;
			}

			inline static Reference<Transform> CreateListenerRepresentation(
				Component* parentObject, bool include_5_1_repr = false, uint32_t color = 0xFFFFFFFF, uint32_t color_5_1 = 0xFFFFFFFF) {
				Reference<Transform> transform = Object::Instantiate<Transform>(parentObject, "Listener Transform");
				Reference<Rigidbody> transformBody = Object::Instantiate<Rigidbody>(transform, "Listener Body");
				transformBody->SetKinematic(true);
				Reference<TriMesh> sphere = TriMesh::Sphere(Vector3(0.0f), 0.25f, 32, 16);

				Reference<Material> material = CreateMaterial(parentObject->Context());
				Object::Instantiate<MeshRenderer>(transformBody, "Listener Center Renderer", sphere, material);

				Reference<TriMesh> forward = TriMesh::Box(Vector3(-0.05f), Vector3(0.05f));
				Reference<Transform> forwardTransform = Object::Instantiate<Transform>(transform, "Listener Forward Transform");
				forwardTransform->SetWorldPosition(transform->WorldPosition() + transform->Forward() * 0.35f);
				forwardTransform->SetLocalScale(Vector3(1.0f, 1.0f, 2.5f));
				Object::Instantiate<MeshRenderer>(forwardTransform, "Listener Forward Renderer", forward, material);

				Object::Instantiate<Jimara::AudioListener>(transformBody, "Listener");

				if (include_5_1_repr) Add_5_1_Representation(transform, color_5_1);

				return transform;
			}

			inline static Reference<Transform> CreateListenerRepresentation(
				Jimara::Test::TestEnvironment& environment, Component* parentObject = nullptr, bool include_5_1_repr = false, uint32_t color = 0xFFFFFFFF, uint32_t color_5_1 = 0xFFFFFFFF) {
				Reference<Transform> repr;
				environment.ExecuteOnUpdateNow([&]() {
					repr = CreateListenerRepresentation(parentObject == nullptr ? environment.RootObject() : parentObject, include_5_1_repr, color, color_5_1); });
				return repr;
			}


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

		TEST(AudioComponentTest, Circling) {
			Jimara::Test::TestEnvironment environment("AudioPlayground: Circling");

			CreateListenerRepresentation(environment, nullptr, true);

			environment.ExecuteOnUpdateNow([&]() {
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, 2.0f)), "Light", Vector3(2.0f, 0.25f, 0.25f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, -2.0f)), "Light", Vector3(0.25f, 2.0f, 0.25f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, 2.0f)), "Light", Vector3(0.25f, 0.25f, 2.0f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, -2.0f)), "Light", Vector3(2.0f, 4.0f, 1.0f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 2.0f, 0.0f)), "Light", Vector3(1.0f, 4.0f, 2.0f));
				});

			Reference<Material> material = CreateMaterial(environment.RootObject()->Context());
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
