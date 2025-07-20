#include "../../GtestHeaders.h"
#include "../../Memory.h"

#include "../TestEnvironment/TestEnvironment.h"
#include "Data/Materials/SampleDiffuse/SampleDiffuseShader.h"

#include "Audio/Buffers/SineBuffer.h"
#include "Audio/Buffers/WaveBuffer.h"

#include "Components/Audio/AudioListener.h"
#include "Components/Audio/AudioSource.h"
#include "Components/Physics/Rigidbody.h"
#include "Components/Physics/BoxCollider.h"
#include "Components/Physics/SphereCollider.h"
#include "Components/Lights/PointLight.h"
#include "Components/GraphicsObjects/MeshRenderer.h"
#include "Data/Geometry/MeshGenerator.h"

#include <random>


namespace Jimara {
	namespace Audio {
		namespace {
			inline static Reference<AudioClip> LoadWavClip(SceneContext* context, const std::string_view& filename, bool streamed) {
				Reference<AudioBuffer> buffer = WaveBuffer(filename, context->Log());
				if (buffer == nullptr) return nullptr;
				else return context->Audio()->AudioScene()->Device()->CreateAudioClip(buffer, streamed);
			}

			inline static Reference<Material> CreateMaterial(SceneContext* context, uint32_t color = 0xFFFFFFFF) {
				Reference<Graphics::ImageTexture> texture = context->Graphics()->Device()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true, 
					Graphics::ImageTexture::AccessFlags::NONE);
				(*static_cast<uint32_t*>(texture->Map())) = color;
				texture->Unmap(true);
				return Jimara::SampleDiffuseShader::CreateMaterial(context, texture);
			}

			inline static Reference<Component> Add_5_1_Representation(Component* parentObject, uint32_t color = 0xFFFFFFFF) {
				Reference<TriMesh> speaker = GenerateMesh::Tri::Box(Vector3(-0.5f), Vector3(0.5f));
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
				Reference<TriMesh> sphere = GenerateMesh::Tri::Sphere(Vector3(0.0f), 0.25f, 32, 16);

				Reference<Material> material = CreateMaterial(parentObject->Context());
				Object::Instantiate<MeshRenderer>(transformBody, "Listener Center Renderer", sphere, material);

				Reference<TriMesh> forward = GenerateMesh::Tri::Box(Vector3(-0.05f), Vector3(0.05f));
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


			class Circler : public virtual Scene::LogicContext::UpdatingComponent {
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
					Transform* transform = (body == nullptr) ? GetTransform() : body->GetTransform();
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
			Reference<AudioClip> clip = environment.RootObject()->Context()->Audio()->AudioScene()->Device()->CreateAudioClip(buffer, false);
			ASSERT_NE(clip, nullptr);

			environment.ExecuteOnUpdateNow([&] {
				Reference<TriMesh> mesh = GenerateMesh::Tri::Sphere(Vector3(0.0f), 0.1f, 16, 8);
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


		namespace {
			enum class Layers : uint8_t {
				DEFAULT = 0,
				BULLET = 1,
				OBSTACLE = 2,
				BULLET_SPARK = 3
			};

			class BulletSparks : public virtual Scene::LogicContext::UpdatingComponent {
			private:
				const Reference<AudioClip> m_obstacleCollisionSound;
				const Stopwatch m_time;

			public:
				inline BulletSparks(Transform* origin, AudioClip* explosionClip)
					: Component(origin->RootObject(), "Sparks")
					, m_obstacleCollisionSound(LoadWavClip(origin->Context(), "Assets/Audio/Effects/Ah_176.4_Stereo.wav", false)) {
					const Reference<Transform> center = Object::Instantiate<Transform>(this, "Sparks Transform");
					center->SetWorldPosition(origin->WorldPosition());

					const Reference<Jimara::AudioSource> source = Object::Instantiate<Jimara::AudioSource3D>(center, "Sparks Audio", explosionClip);
					source->SetVolume(4.0f);
					source->Play();

					const float SPARK_SIZE = 0.1f;
					const Reference<TriMesh> sparkShape = GenerateMesh::Tri::Box(Vector3(-SPARK_SIZE * 0.5f), Vector3(SPARK_SIZE * 0.5f));
					const Reference<Material> sparkMaterial = CreateMaterial(Context(), 0xFFFFFFFF);

					static std::mt19937 generator;
					std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
					for (size_t i = 0; i < 32; i++) {
						Reference<Transform> sparkTransform = Object::Instantiate<Transform>(center, "Spark");
						Object::Instantiate<MeshRenderer>(sparkTransform, "Spark Renderer", sparkShape, sparkMaterial);
						Reference<Rigidbody> sparkBody = Object::Instantiate<Rigidbody>(sparkTransform, "Spark Body");
						float theta = 2 * Math::Pi() * distribution(generator);
						float phi = acos(1 - 2 * distribution(generator));
						sparkBody->SetVelocity(Vector3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi)) * 12.0f);
						sparkTransform->SetLocalPosition(Math::Normalize(sparkBody->Velocity()) * SPARK_SIZE);
						const Reference<Collider> sparkCollider = Object::Instantiate<BoxCollider>(sparkBody, "Spark Collider", Vector3(SPARK_SIZE));
						sparkCollider->SetLayer(Layers::BULLET_SPARK);
						sparkCollider->OnContact() += Callback<const Jimara::Collider::ContactInfo&>([](const Jimara::Collider::ContactInfo& info) {
							if (info.OtherCollider()->GetLayer() == (Jimara::Collider::Layer)Layers::OBSTACLE)
								info.ReportingCollider()->GetComponentInChildren<Jimara::AudioSource3D>()->PlayOneShot(
									info.ReportingCollider()->GetComponentInParents<BulletSparks>()->m_obstacleCollisionSound);
							});
						Object::Instantiate<Jimara::AudioSource3D>(sparkCollider, "Spark Source")->SetVolume(0.25f);
					}
				}

				inline virtual void Update() override {
					if (m_time.Elapsed() > 1.0f && (!GetComponentInChildren<Jimara::AudioSource>()->Playing())) Destroy();
				}
			};

			class Bullet : public virtual Scene::LogicContext::UpdatingComponent {
			private:
				const Reference<AudioClip> m_explosionClip;
				const Stopwatch m_time;

				inline void OnContact(const Jimara::Collider::ContactInfo& info) {
					if (info.OtherCollider()->GetLayer() != (Jimara::Collider::Layer)Layers::OBSTACLE) return;
					Object::Instantiate<BulletSparks>(info.ReportingCollider()->GetTransform(), m_explosionClip);
					Destroy();
				}

			public:
				inline static constexpr float Radius() { return 0.1f; }

				inline Bullet(Transform* root, TriMesh* shape, Material* material, AudioClip* startClip, AudioClip* flyingClip, AudioClip* explosionClip) 
					: Component(root->RootObject(), "Bullet"), m_explosionClip(explosionClip) {
					const Reference<Transform> bulletTransform = Object::Instantiate<Transform>(this, "Bullet Transform");
					bulletTransform->SetWorldPosition(root->LocalToWorldPosition(Vector3(0.0f, 0.0f, -2.0f)));

					Object::Instantiate<MeshRenderer>(bulletTransform, "Bullet Renderer", shape, material);

					const Reference<Rigidbody> bulletBody = Object::Instantiate<Rigidbody>(bulletTransform, "Bullet Body");
					bulletBody->SetVelocity(root->Forward() * 7.0f);

					const Reference<Collider> bulletCollider = Object::Instantiate<SphereCollider>(bulletBody, "Bullet Collider", Radius());
					bulletCollider->SetLayer(Layers::BULLET);
					bulletCollider->OnContact() += Callback(&Bullet::OnContact, this);

					const Reference<Jimara::AudioSource> bulletSource = Object::Instantiate<Jimara::AudioSource3D>(bulletCollider, "Bullet Source", flyingClip);
					bulletSource->SetLooping(true);
					bulletSource->Play();
					bulletSource->PlayOneShot(startClip);
				}

				inline virtual void Update() override {
					if (m_time.Elapsed() > 5.0f) Destroy();
				}
			};

			class Gun : public virtual Scene::LogicContext::UpdatingComponent {
			private:
				const Reference<Transform> m_gunRoot;
				const Reference<TriMesh> m_bulletMesh;
				const Reference<Material> m_bulletMaterial;
				const Reference<AudioClip> m_bulletFireSound;
				const Reference<AudioClip> m_bulletFlyingSound;
				const Reference<AudioClip> m_bulletExplosionSound;
				const Stopwatch m_totalTime;
				Stopwatch m_timer;

			public:
				inline Gun(Component* root) 
					: Component(root, "Gun")
					, m_gunRoot(Object::Instantiate<Transform>(root, "Gun Root"))
					, m_bulletMesh(GenerateMesh::Tri::Sphere(Vector3(0.0f), Bullet::Radius(), 16, 8))
					, m_bulletMaterial(CreateMaterial(root->Context(), 0xFFFF0000))
					, m_bulletFireSound(LoadWavClip(root->Context(), "Assets/Audio/Effects/DumbChild_88.2_Mono.wav", false))
					, m_bulletFlyingSound(LoadWavClip(root->Context(), "Assets/Audio/Effects/Tuva_192_Stereo.wav", false))
					, m_bulletExplosionSound(LoadWavClip(root->Context(), "Assets/Audio/Effects/Fart_96_Mono.wav", false)) {
					m_gunRoot->SetLocalPosition(Vector3(0.0f, 1.0f, 0.0f));

					const Reference<Transform> gunTransform = Object::Instantiate<Transform>(m_gunRoot, "Gun Transform");
					gunTransform->SetLocalEulerAngles(Vector3(90.0f, 0.0f, 0.0f));
					gunTransform->SetLocalPosition(Vector3(0.0f, 0.0f, -2.5f));
					gunTransform->SetLocalScale(Vector3(0.5f, 0.5f, 0.5f));

					const Reference<TriMesh> barrelShape = GenerateMesh::Tri::Capsule(Vector3(0.0f), 0.15f, 1.0f, 24, 8);
					const Reference<TriMesh> tipShape = GenerateMesh::Tri::Box(Vector3(-0.25f, 0.25f, -0.25f), Vector3(0.25f, 0.75f, 0.25f));
					const Reference<Material> barrelMaterial = CreateMaterial(root->Context(), 0xFF00FF00);
					Object::Instantiate<MeshRenderer>(gunTransform, "Gun Barrel Renderer", barrelShape, barrelMaterial);
					Object::Instantiate<MeshRenderer>(gunTransform, "Gun Tip Renderer", tipShape, barrelMaterial);
				}

				inline virtual void Update() override {
					m_gunRoot->SetLocalEulerAngles(Vector3(-30.0f, 30.0f * m_totalTime.Elapsed(), 0.0f));

					if (m_timer.Elapsed() < 2.5f) return;
					m_timer.Reset();

					Object::Instantiate<Bullet>(m_gunRoot, m_bulletMesh, m_bulletMaterial, m_bulletFireSound, m_bulletFlyingSound, m_bulletExplosionSound);
				}
			};

			class BackgroundSoundMixer : public virtual Scene::LogicContext::UpdatingComponent {
			private:
				const Reference<Jimara::AudioSource2D> m_sources[2];
				size_t m_activeSource = SourceCount();

			public:
				inline static constexpr size_t SourceCount() { return sizeof(((BackgroundSoundMixer*)nullptr)->m_sources) / sizeof(const Reference<Jimara::AudioSource2D>); }

				inline BackgroundSoundMixer(Component* root) 
					: Component(root, "Background Sound Mixer")
					, m_sources{
						Object::Instantiate<Jimara::AudioSource2D>(root, "Track 1"),
						Object::Instantiate<Jimara::AudioSource2D>(root, "Track 2") 
				} {
					const Reference<AudioClip> tracks[SourceCount()] = {
						LoadWavClip(root->Context(), "Assets/Audio/Tracks/Track 1 Stereo 96KHz 16 bit.wav", true),
						LoadWavClip(root->Context(), "Assets/Audio/Tracks/Track 2 Stereo 88.2KHz 16 bit.wav", true)
					};
					for (size_t i = 0; i < SourceCount(); i++) {
						AudioClip* clip = tracks[i];
						assert(clip != nullptr);

						Jimara::AudioSource* source = m_sources[i];
						source->SetLooping(true);
						source->SetVolume(0.0f);
						source->SetClip(clip);
						source->Play();
					}
				}

				inline virtual void Update() override {
					const float deltaTime = Context()->Time()->ScaledDeltaTime();
					float weight = min(deltaTime * 0.75f, 1.0f);
					for (size_t i = 0; i < SourceCount(); i++) {
						float target = (m_activeSource == i ? 0.25f : 0.0f);
						Jimara::AudioSource* source = m_sources[i];
						source->SetVolume((source->Volume() * (1.0f - weight)) + (target * weight));
					}
				}

				inline void SetTrackId(size_t newId) { m_activeSource = newId; }
			};

			class Obstacle : public virtual Component {
			private:
				const Reference<BackgroundSoundMixer> m_mixer;
				const size_t m_trackId;

				inline void OnHit(const Jimara::Collider::ContactInfo& info) {
					if (info.OtherCollider()->GetLayer() != (Jimara::Collider::Layer)Layers::BULLET) return;
					m_mixer->SetTrackId(m_trackId);
				}

			public:
				inline Obstacle(Component* root, float rotation, TriMesh* mesh, Material* material
					, BackgroundSoundMixer* mixer, size_t trackId)
					: Component(root, "Obstacle"), m_mixer(mixer), m_trackId(trackId) {
					Reference<Transform> parent = Object::Instantiate<Transform>(root, "Obstacle Parent");
					parent->SetLocalEulerAngles(Vector3(0.0f, rotation, 0.0f));

					Reference<Transform> transform = Object::Instantiate<Transform>(parent, "Obstacle");
					transform->SetLocalPosition(Vector3(0.0f, 0.0f, 3.0f));
					transform->SetLocalScale(Vector3(2.0f, 1.0f, 0.1f));

					Object::Instantiate<MeshRenderer>(transform, "Obstacle Renderer", mesh, material);
					Reference<BoxCollider> obstacleCollider = Object::Instantiate<BoxCollider>(transform, "Obstacle Collider");
					obstacleCollider->SetLayer(Layers::OBSTACLE);
					obstacleCollider->OnContact() += Callback<const Jimara::Collider::ContactInfo&>(&Obstacle::OnHit, this);
				}

				inline static void Create(Component* root) {
					const Reference<TriMesh> obstacleGeometry = GenerateMesh::Tri::Box(Vector3(-0.5f), Vector3(0.5f));
					const Reference<Material> obstacleMaterials[BackgroundSoundMixer::SourceCount()] = {
						CreateMaterial(root->Context(), 0xFF0000FF),
						CreateMaterial(root->Context(), 0xFF00FFFF)
					};
					for (size_t i = 0; i < BackgroundSoundMixer::SourceCount(); i++)
						assert(obstacleMaterials[i] != nullptr);
					
					const Reference<BackgroundSoundMixer> mixer = Object::Instantiate<BackgroundSoundMixer>(root);

					const size_t NUM_OBSTACLES = 8;
					for (size_t i = 0; i < NUM_OBSTACLES; i++) {
						size_t trackId = i % BackgroundSoundMixer::SourceCount();
						Object::Instantiate<Obstacle>(root, (360.0f / (float)NUM_OBSTACLES) * i, obstacleGeometry, obstacleMaterials[trackId], mixer, trackId);
					}
				}
			};
		}

		TEST(AudioComponentTest, GunThing) {
			Jimara::Test::TestEnvironment environment("AudioPlayground: GunThing");

			CreateListenerRepresentation(environment, nullptr, true);

			environment.ExecuteOnUpdateNow([&]() {
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, 2.0f)), "Light", Vector3(2.0f, 0.25f, 0.25f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, -2.0f)), "Light", Vector3(0.25f, 2.0f, 0.25f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, 2.0f)), "Light", Vector3(0.25f, 0.25f, 2.0f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, -2.0f)), "Light", Vector3(2.0f, 4.0f, 1.0f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 2.0f, 0.0f)), "Light", Vector3(1.0f, 4.0f, 2.0f));
				});

			environment.ExecuteOnUpdateNow([&]() {
				Reference<AudioClip> subClip = LoadWavClip(environment.RootObject()->Context(), "Assets/Audio/Mono_sub/Mono_sub_192_16.wav", true);
				assert(subClip != nullptr);
				Reference<Jimara::AudioSource2D> source = Object::Instantiate<Jimara::AudioSource2D>(environment.RootObject(), "SubSource", subClip); 
				source->SetLooping(true);
				source->Play();
				});

			environment.ExecuteOnUpdateNow([&]() { 
				Obstacle::Create(environment.RootObject());
				Object::Instantiate<Gun>(environment.RootObject());
				});
		}
	}
}
