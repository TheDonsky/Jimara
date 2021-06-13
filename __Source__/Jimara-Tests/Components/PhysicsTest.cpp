#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Shaders/SampleDiffuseShader.h"
#include "TestEnvironment/TestEnvironment.h"
#include "Components/MeshRenderer.h"
#include "Components/Lights/PointLight.h"
#include "Components/Lights/DirectionalLight.h"
#include "Components/Physics/BoxCollider.h"
#include "Components/Physics/SphereCollider.h"
#include "Components/Physics/CapsuleCollider.h"
#include <sstream>


namespace Jimara {
	namespace Physics {
		namespace {
			inline static Reference<Material> CreateMaterial(Component* rootObject, uint32_t color) {
				Reference<Graphics::ImageTexture> texture = rootObject->Context()->Graphics()->Device()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
				(*static_cast<uint32_t*>(texture->Map())) = color;
				texture->Unmap(true);
				return Jimara::Test::SampleDiffuseShader::CreateMaterial(texture);
			};

			inline static void CreateLights(Component* rootObject) {
				Reference<Transform> sun = Object::Instantiate<Transform>(rootObject, "Sun", Vector3(0.0f), Vector3(64.0f, 32.0f, 0.0f));
				Object::Instantiate<DirectionalLight>(sun, "Sun Light", Vector3(0.85f, 0.85f, 0.856f));
				Reference<Transform> back = Object::Instantiate<Transform>(rootObject, "Sun");
				back->LookTowards(-sun->Forward());
				Object::Instantiate<DirectionalLight>(back, "Back Light", Vector3(0.125f, 0.125f, 0.125f));
			}

			inline static uint32_t ColorFromVector(const Vector3& color) {
				auto channel = [](float channel) { return static_cast<uint32_t>(max(channel, 0.0f) * 255.0f) & 255; };
				return channel(color.r) | (channel(color.g) << 8) | (channel(color.b) << 16) | (channel(1.0f) << 24);
			};

			struct SpownerSettings : public virtual Object {
				const std::string caseName;
				const float spownInterval;
				const size_t maxSpownedObjects;
				
				virtual Reference<Transform> Create(Component* root, float warmupTime) = 0;

				inline SpownerSettings(const std::string_view& name, float interval, size_t maxCount)
					: caseName(name), spownInterval(interval), maxSpownedObjects(maxCount) {}
			};

			class Spowner : public virtual Component, public virtual PostPhysicsSynchUpdater {
			private:
				const Reference<SpownerSettings> m_settings;
				Stopwatch m_stopwatch;
				float m_timeLeft = 0.0f;
				std::queue<Reference<Transform>> m_transformQueue;

			public:
				inline virtual void PostPhysicsSynch()override {
					m_timeLeft += m_stopwatch.Reset();
					while (m_timeLeft >= m_settings->spownInterval) {
						m_timeLeft -= m_settings->spownInterval;
						m_transformQueue.push(m_settings->Create(RootObject(), m_timeLeft));
						while (m_transformQueue.size() > m_settings->maxSpownedObjects) {
							Reference<Transform> t = m_transformQueue.front();
							m_transformQueue.pop();
							t->Destroy();
						}
					}
				}

				inline Spowner(Component* parent, SpownerSettings* settings)
					: Component(parent, settings->caseName), m_settings(settings) {}
			};

			class Platform : public virtual Component, public virtual PostPhysicsSynchUpdater {
			private:
				Stopwatch m_stopwatch;

			public:
				inline Platform(Component* parent, const std::string_view& name) : Component(parent, name) {}

				inline virtual void PostPhysicsSynch()override {
					Transform* transform = GetTransfrom();
					if (transform == nullptr) return;
					Vector3 pos = transform->LocalPosition();
					float elapsed = m_stopwatch.Elapsed();
					pos.y = sin(elapsed) * 0.5f;
					transform->SetLocalPosition(pos);
					transform->SetLocalScale(Vector3(1.5f + 0.5f * cos(elapsed * 2.0f)));
				}
			};

			class SimpleMeshSpowner : public SpownerSettings {
			private:
				const Callback<Rigidbody*> m_createCollider;
				const Reference<const Material> m_material;
				const std::vector<Reference<const TriMesh>> m_meshes;

			public:
				inline virtual Reference<Transform> Create(Component* root, float)override {
					Reference<Transform> rigidTransform = Object::Instantiate<Transform>(root, "Rigid Transform", Vector3(0.0f, 1.0f, 0.0f));
					Reference<Rigidbody> rigidBody = Object::Instantiate<Rigidbody>(rigidTransform);
					for (size_t i = 0; i < m_meshes.size(); i++)
						Object::Instantiate<MeshRenderer>(rigidBody, "RigidBody Renderer", m_meshes[i], m_material);
					m_createCollider(rigidBody);
					return rigidTransform;
				}

				inline SimpleMeshSpowner(
					const Material* material, const Reference<TriMesh>* meshes, size_t meshCount,
					const Callback<Rigidbody*>& createCollider,
					const std::string_view& name, float interval = 0.125f, size_t maxCount = 512)
					: SpownerSettings(name, interval, maxCount)
					, m_createCollider(createCollider), m_material(material), m_meshes(meshes, meshes + meshCount) {}
			};

			class RadialMeshSpowner : public SimpleMeshSpowner {
			private:
				const Callback<Rigidbody*> m_create;
				Stopwatch m_stopwatch;

				void Create(Rigidbody* rigidbody)const {
					m_create(rigidbody);
					float totalTime = m_stopwatch.Elapsed();
					rigidbody->SetVelocity(Vector3(3.0f * cos(totalTime * 2.0f), 7.0f, 3.0f * sin(totalTime * 2.0f)));
				}

			public:
				inline RadialMeshSpowner(
					const Material* material, const Reference<TriMesh>* meshes, size_t meshCount,
					const Callback<Rigidbody*>& createCollider,
					const std::string_view& name, float interval = 0.125f, size_t maxCount = 512)
					: SimpleMeshSpowner(material, meshes, meshCount, Callback(&RadialMeshSpowner::Create, this), name, interval, maxCount)
					, m_create(createCollider) {}
			};
		}

		TEST(PhysicsTest, Simulation) {
			typedef Reference<SpownerSettings>(*CreateSettings)(Component*);
			const CreateSettings CREATE_SETTINGS[] = {
				[](Component* root) -> Reference<SpownerSettings> {
					// Simply spowns cubes at the center:
					Reference<Material> material = CreateMaterial(root, 0xFFFFFFFF);
					Reference<TriMesh> mesh = TriMesh::Box(Vector3(-0.25f, -0.25f, -0.25f), Vector3(0.25f, 0.25f, 0.25f));
					Callback<Rigidbody*> createCollider = Callback<Rigidbody*>([](Rigidbody* rigidBody) {
						Object::Instantiate<BoxCollider>(rigidBody, "Box Collider", Vector3(0.5f, 0.5f, 0.5f));
						});
					Reference<SpownerSettings> settings = Object::Instantiate<SimpleMeshSpowner>(material, &mesh, 1, createCollider, "Spown Boxes");
					return settings;
				}, [](Component* root) -> Reference<SpownerSettings> {
					// Simply spowns capsules at the center:
					Reference<Material> material = CreateMaterial(root, 0xFFFFFFFF);
					Reference<TriMesh> mesh = TriMesh::Capsule(Vector3(0.0f), 0.15f, 0.7f, 16, 8, 4);
					Callback<Rigidbody*> createCollider = Callback<Rigidbody*>([](Rigidbody* rigidBody) {
						Object::Instantiate<CapsuleCollider>(rigidBody, "Capsule collider", 0.15f, 0.7f);
						});
					return Object::Instantiate<SimpleMeshSpowner>(material, &mesh, 1, createCollider, "Spown Capsules");
				}, [](Component* root) -> Reference<SpownerSettings> {
					// Spowns boxes and applies some velocity:
					Reference<Material> material = CreateMaterial(root, 0xFFFFFFFF);
					Reference<TriMesh> mesh = TriMesh::Box(Vector3(-0.25f, -0.25f, -0.25f), Vector3(0.25f, 0.25f, 0.25f));
					Callback<Rigidbody*> createCollider = Callback<Rigidbody*>([](Rigidbody* rigidBody) {
						Object::Instantiate<BoxCollider>(rigidBody, "Box Collider", Vector3(0.5f, 0.5f, 0.5f));
						});
					return Object::Instantiate<RadialMeshSpowner>(material, &mesh, 1, createCollider, "Spown Boxes Radially");
				}, [](Component* root) -> Reference<SpownerSettings> {
					// Simply spowns spheres at the center and applies some velocity:
					Reference<Material> material = CreateMaterial(root, 0xFFFFFFFF);
					Reference<TriMesh> mesh = TriMesh::Sphere(Vector3(0.0f), 0.5f, 16, 8);
					Callback<Rigidbody*> createCollider = Callback<Rigidbody*>([](Rigidbody* rigidBody) {
						Object::Instantiate<SphereCollider>(rigidBody, "Sphere collider", 0.5f);
						});
					return Object::Instantiate<RadialMeshSpowner>(material, &mesh, 1, createCollider, "Spown Spheres");
				}, [](Component* root) -> Reference<SpownerSettings> {
					// Spowns capsules, applies some velocity and lcoks XZ rotation:
					Reference<Material> material = CreateMaterial(root, 0xFFFFFFFF);
					Reference<TriMesh> mesh = TriMesh::Capsule(Vector3(0.0f), 0.15f, 0.7f, 16, 8, 4);
					Callback<Rigidbody*> createCollider = Callback<Rigidbody*>([](Rigidbody* rigidBody) {
						Object::Instantiate<CapsuleCollider>(rigidBody, "Capsule collider", 0.15f, 0.7f);
						rigidBody->SetLockFlags(Physics::DynamicBody::LockFlags(
							Physics::DynamicBody::LockFlag::ROTATION_X, Physics::DynamicBody::LockFlag::ROTATION_Z));
						});
					return Object::Instantiate<RadialMeshSpowner>(material, &mesh, 1, createCollider, "Lock Rotation XZ");
				}, [](Component* root) -> Reference<SpownerSettings> {
					// Spowns capsules & boxes, applies some velocity:
					Reference<Material> material = CreateMaterial(root, 0xFFFFFFFF);
					static const Vector3 CAPSULE_OFFSET(0.0f, -0.3f, 0.0f);
					static const Vector3 SPHERE_OFFSET(0.0f, 0.5f, 0.0f);
					Reference<TriMesh> meshes[] = {
						TriMesh::Box(Vector3(-0.25f, -0.25f, -0.25f), Vector3(0.25f, 0.25f, 0.25f)),
						TriMesh::Capsule(CAPSULE_OFFSET, 0.15f, 0.7f, 16, 8, 4),
						TriMesh::Sphere(SPHERE_OFFSET, 0.25f, 16, 8)
					};
					Callback<Rigidbody*> createCollider = Callback<Rigidbody*>([](Rigidbody* rigidBody) {
						Object::Instantiate<BoxCollider>(rigidBody, "Box Collider", Vector3(0.5f, 0.5f, 0.5f));
						Object::Instantiate<CapsuleCollider>(Object::Instantiate<Transform>(rigidBody, "Capsule Transform", CAPSULE_OFFSET), "Capsule collider", 0.15f, 0.7f);
						Object::Instantiate<SphereCollider>(Object::Instantiate<Transform>(rigidBody, "Sphere Transform", SPHERE_OFFSET), "Sphere collider", 0.25f);
						});
					return Object::Instantiate<RadialMeshSpowner>(material, meshes, 3, createCollider, "Multi-collider");
				}, [](Component* root) -> Reference<SpownerSettings> {
					// Simply spowns cubes at the center and limits simulation to XY:
					Reference<Material> material = CreateMaterial(root, 0xFFFFFFFF);
					Reference<TriMesh> mesh = TriMesh::Box(Vector3(-0.25f, -0.25f, -0.25f), Vector3(0.25f, 0.25f, 0.25f));
					Callback<Rigidbody*> createCollider = Callback<Rigidbody*>([](Rigidbody* rigidBody) {
						Object::Instantiate<BoxCollider>(rigidBody, "Box Collider", Vector3(0.5f, 0.5f, 0.5f));
						rigidBody->SetLockFlags(Physics::DynamicBody::LockFlags(
							Physics::DynamicBody::LockFlag::MOVEMENT_Z,
							Physics::DynamicBody::LockFlag::ROTATION_X, Physics::DynamicBody::LockFlag::ROTATION_Y));
						});
					Reference<SpownerSettings> settings = Object::Instantiate<SimpleMeshSpowner>(material, &mesh, 1, createCollider, "Lock Rotation XY, Lock movement Z");
					return settings;
				}
			};
#ifdef _WIN32
			Jimara::Test::Memory::MemorySnapshot snapshot;
			auto updateSnapshot = [&]() { snapshot = Jimara::Test::Memory::MemorySnapshot(); };
			auto compareSnapshot = [&]() -> bool { return snapshot.Compare(); };
#else
#ifndef NDEBUG
			size_t snapshot;
			auto updateSnapshot = [&]() { snapshot = Object::DEBUG_ActiveInstanceCount(); };
			auto compareSnapshot = [&]() -> bool { return snapshot == Object::DEBUG_ActiveInstanceCount(); };
#else
			auto updateSnapshot = [&]() {};
			auto compareSnapshot = [&]() -> bool { return true; };
#endif 
#endif
			const size_t NUM_SETTINGS(sizeof(CREATE_SETTINGS) / sizeof(CreateSettings));
			for (size_t i = 0; i < NUM_SETTINGS; i++) {
				updateSnapshot();
				{
					Jimara::Test::TestEnvironment environment("Simulation");
					CreateLights(environment.RootObject());
					{
						Reference<Transform> baseTransform = Object::Instantiate<Transform>(environment.RootObject(), "Base Transform");
						const Vector3 extents(8.0f, 0.1f, 16.0f);
						Object::Instantiate<BoxCollider>(baseTransform, "Surface Object", extents);
						Reference<TriMesh> cube = TriMesh::Box(extents * -0.5f, extents * 0.5f);
						Reference<Material> material = CreateMaterial(environment.RootObject(), 0xFFFFFFFF);
						Object::Instantiate<MeshRenderer>(baseTransform, "Surface Renderer", cube, material);
						Object::Instantiate<Platform>(baseTransform, "Platform");
					}
					Reference<SpownerSettings> settings = CREATE_SETTINGS[i](environment.RootObject());
					environment.SetWindowName(settings->caseName);
					Object::Instantiate<Spowner>(environment.RootObject(), settings);
				}
				if (i > 0) { EXPECT_TRUE(compareSnapshot()); }
			}
			EXPECT_TRUE(compareSnapshot());
		}



		namespace {
			inline static Reference<Collider> CreateStaticBox(Jimara::Test::TestEnvironment& environment, PhysicsMaterial* physMaterial, const Vector3& position, const Vector3& size) {
				Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Box Transform", position);
				Reference<BoxCollider> collider = Object::Instantiate<BoxCollider>(transform, "Box Collider", size, physMaterial);
				Reference<TriMesh> mesh = TriMesh::Box(-collider->Size() * 0.5f, collider->Size() * 0.5f);
				Reference<Material> material = CreateMaterial(environment.RootObject(), 0xFFFFFFFF);
				Object::Instantiate<MeshRenderer>(transform, "Surface Renderer", mesh, material);
				return collider;
			}

			class ColorChanger : public virtual Component {
			private:
				const Vector3 m_colorWhenNotTouching;
				const Vector3 m_colorOnTouch;
				const Vector3 m_colorDeltaOverTime;
				Vector3 m_color;
				Reference<Collider> m_curCollider;
				const Collider::ContactType m_beginEvent;
				const Collider::ContactType m_persistsEvent;
				const Collider::ContactType m_endEvent;


				inline void ChangeColor(const Collider::ContactInfo& info) {
					if (info.EventType() == m_beginEvent) {
						m_color = m_colorOnTouch;
					}
					else if (info.EventType() == m_persistsEvent) {
						float dt = info.ReportingCollider()->Context()->Physics()->ScaledDeltaTime();
						m_color += m_colorDeltaOverTime * dt;
					}
					else if (info.EventType() == m_endEvent) {
						m_color = m_colorWhenNotTouching;
					}
					Reference<Material> material = CreateMaterial(info.ReportingCollider(), ColorFromVector(m_color));
					MeshRenderer* renderer = info.ReportingCollider()->GetTransfrom()->GetComponentInChildren<MeshRenderer>();
					if (renderer != nullptr) renderer->SetMaterial(material);
				}

				inline void Reatach(const Component*) {
					Reference<Collider> collider = GetComponentInParents<Collider>();
					if (collider == m_curCollider) return;
					Callback<const Collider::ContactInfo&> callback(&ColorChanger::ChangeColor, this);
					if (m_curCollider != nullptr) m_curCollider->OnContact() -= callback;
					m_curCollider = collider;
					if (m_curCollider != nullptr) m_curCollider->OnContact() += callback;
				}

				inline void Detouch(Component*) { 
					if (m_curCollider != nullptr) m_curCollider->OnContact() -= Callback<const Collider::ContactInfo&>(&ColorChanger::ChangeColor, this);;
					m_curCollider = nullptr;
				}

			public:
				inline ColorChanger(Component* parent, const std::string_view& name, 
					const Vector3& whenNoTouch = Vector3(1.0f, 1.0f, 1.0f), const Vector3& onTouch = Vector3(1.0f, 0.0f, 0.0f), const Vector3& deltaOverTime = Vector3(-1.0f, 1.0f, 0.0f), 
					bool trigger = false)
					: Component(parent, name)
					, m_colorWhenNotTouching(whenNoTouch), m_colorOnTouch(onTouch), m_colorDeltaOverTime(deltaOverTime), m_color(whenNoTouch)
					, m_beginEvent(trigger ? Collider::ContactType::ON_TRIGGER_BEGIN : Collider::ContactType::ON_COLLISION_BEGIN)
					, m_persistsEvent(trigger ? Collider::ContactType::ON_TRIGGER_PERSISTS : Collider::ContactType::ON_COLLISION_PERSISTS)
					, m_endEvent(trigger ? Collider::ContactType::ON_TRIGGER_END : Collider::ContactType::ON_COLLISION_END) {
					OnParentChanged() += Callback(&ColorChanger::Reatach, this);
					OnDestroyed() += Callback(&ColorChanger::Detouch, this);
					Reatach(nullptr);
				}

				inline virtual ~ColorChanger() {
					OnParentChanged() -= Callback(&ColorChanger::Reatach, this);
					OnDestroyed() -= Callback(&ColorChanger::Detouch, this);
					Detouch(nullptr);
				}

				inline Vector3 Color()const { return m_color; }
			};
		}

		TEST(PhysicsTest, CollisionEvents_Dynamic) {
			Jimara::Test::TestEnvironment environment("Contact reporting with dynamic rigidbodies");
			CreateLights(environment.RootObject());
			Reference<PhysicsMaterial> physMaterial = environment.RootObject()->Context()->Physics()->APIInstance()->CreateMaterial(0.5, 0.5f, 0.0f);

			environment.ExecuteOnUpdateNow([&]() {
				Object::Instantiate<ColorChanger>(
					CreateStaticBox(environment, physMaterial, Vector3(0.0f, -1.0f, 0.0f), Vector3(4.0f, 0.1f, 4.0f)),
					"Platform Color Changer", Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(1.0f, -1.0f, 0.0f));
				Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Rigidbody Transform", Vector3(0.0f, 2.0f, 0.0f));
				Reference<Rigidbody> rigidbody = Object::Instantiate<Rigidbody>(transform);
				rigidbody->SetLockFlags(DynamicBody::LockFlags(DynamicBody::LockFlag::ROTATION_X, DynamicBody::LockFlag::ROTATION_Z));
				Reference<CapsuleCollider> collider = Object::Instantiate<CapsuleCollider>(rigidbody, "Rigidbody Collider", 0.25f, 0.5f, physMaterial);
				Reference<TriMesh> mesh = TriMesh::Capsule(Vector3(0.0f), collider->Radius(), collider->Height(), 32, 8, 2);
				Reference<Material> material = CreateMaterial(environment.RootObject(), 0xFFFFFFFF);
				Object::Instantiate<MeshRenderer>(transform, "Rigidbody Renderer", mesh, material);
				Object::Instantiate<ColorChanger>(collider, "Color Changer");
				static const auto jump = [](const Collider::ContactInfo& info) {
					if (info.EventType() == Collider::ContactType::ON_COLLISION_PERSISTS && info.ReportingCollider()->GetComponentInChildren<ColorChanger>()->Color().g >= 1.0f)
						info.ReportingCollider()->GetComponentInParents<Rigidbody>()->SetVelocity(Vector3(0.0f, 8.0f, 0.0f));
				};
				collider->OnContact() += Callback<const Collider::ContactInfo&>(jump);
				});
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		TEST(PhysicsTest, CollisionEvents_Dynamic_MoveManually) {
			Jimara::Test::TestEnvironment environment("Contact reporting with dynamic rigidbodies, moved manually");
			CreateLights(environment.RootObject());
			Reference<PhysicsMaterial> physMaterial = environment.RootObject()->Context()->Physics()->APIInstance()->CreateMaterial(0.5, 0.5f, 0.0f);

			environment.ExecuteOnUpdateNow([&]() {
				CreateStaticBox(environment, physMaterial, Vector3(0.0f, -1.0f, 0.0f), Vector3(2.0f, 0.1f, 2.0f));
				Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Rigidbody Transform", Vector3(0.0f, 2.0f, 0.0f));
				Reference<Rigidbody> rigidbody = Object::Instantiate<Rigidbody>(transform);
				rigidbody->SetLockFlags(DynamicBody::LockFlags(
					DynamicBody::LockFlag::MOVEMENT_X, DynamicBody::LockFlag::MOVEMENT_Y, DynamicBody::LockFlag::MOVEMENT_Z,
					DynamicBody::LockFlag::ROTATION_X, DynamicBody::LockFlag::ROTATION_Y, DynamicBody::LockFlag::ROTATION_Z));
				Reference<CapsuleCollider> collider = Object::Instantiate<CapsuleCollider>(rigidbody, "Rigidbody Collider", 0.25f, 0.5f, physMaterial);
				Reference<TriMesh> mesh = TriMesh::Capsule(Vector3(0.0f), collider->Radius(), collider->Height(), 32, 8, 2);
				Reference<Material> material = CreateMaterial(environment.RootObject(), 0xFFFFFFFF);
				Object::Instantiate<MeshRenderer>(transform, "Rigidbody Renderer", mesh, material);
				Object::Instantiate<ColorChanger>(collider, "Color Changer");
				static Transform* trans;
				static Stopwatch stop;
				trans = transform;
				stop.Reset();
				collider->Context()->Graphics()->OnPostGraphicsSynch() += Callback<>([]() {
					trans->SetWorldPosition(Vector3(0.0f, sin(stop.Elapsed()) * 1.5f - 1.0f, 0.0f));
					});
				});
		}

		TEST(PhysicsTest, CollisionEvents_Kinematic_MoveManually) {
			Jimara::Test::TestEnvironment environment("Contact reporting with kinematic rigidbodies, moved manually");
			CreateLights(environment.RootObject());
			Reference<PhysicsMaterial> physMaterial = environment.RootObject()->Context()->Physics()->APIInstance()->CreateMaterial(0.5, 0.5f, 0.0f);

			environment.ExecuteOnUpdateNow([&]() {
				CreateStaticBox(environment, physMaterial, Vector3(0.0f, -1.0f, 0.0f), Vector3(2.0f, 0.1f, 2.0f));
				Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Rigidbody Transform", Vector3(0.0f, 2.0f, 0.0f));
				Reference<Rigidbody> rigidbody = Object::Instantiate<Rigidbody>(transform);
				rigidbody->SetLockFlags(DynamicBody::LockFlags(
					DynamicBody::LockFlag::MOVEMENT_X, DynamicBody::LockFlag::MOVEMENT_Y, DynamicBody::LockFlag::MOVEMENT_Z,
					DynamicBody::LockFlag::ROTATION_X, DynamicBody::LockFlag::ROTATION_Y, DynamicBody::LockFlag::ROTATION_Z));
				rigidbody->SetKinematic(true);
				Reference<CapsuleCollider> collider = Object::Instantiate<CapsuleCollider>(rigidbody, "Rigidbody Collider", 0.25f, 0.5f, physMaterial);
				Reference<TriMesh> mesh = TriMesh::Capsule(Vector3(0.0f), collider->Radius(), collider->Height(), 32, 8, 2);
				Reference<Material> material = CreateMaterial(environment.RootObject(), 0xFFFFFFFF);
				Object::Instantiate<MeshRenderer>(transform, "Rigidbody Renderer", mesh, material);
				Object::Instantiate<ColorChanger>(collider, "Color Changer");
				static Transform* trans;
				static Stopwatch stop;
				trans = transform;
				stop.Reset();
				collider->Context()->Graphics()->OnPostGraphicsSynch() += Callback<>([]() {
					trans->SetWorldPosition(Vector3(0.0f, sin(stop.Elapsed()) * 1.5f - 1.0f, 0.0f));
					});
				});
		}

		TEST(PhysicsTest, TriggerEvents_Dynamic) {
			Jimara::Test::TestEnvironment environment("Trigger contact reporting with dynamic rigidbodies");
			CreateLights(environment.RootObject());
			Reference<PhysicsMaterial> physMaterial = environment.RootObject()->Context()->Physics()->APIInstance()->CreateMaterial(0.5, 0.5f, 0.0f);

			environment.ExecuteOnUpdateNow([&]() {
				Object::Instantiate<ColorChanger>(
					CreateStaticBox(environment, physMaterial, Vector3(0.0f, -4.0f, 0.0f), Vector3(4.0f, 8.0f, 4.0f)),
					"Platform Color Changer", Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(1.0f, -1.0f, 0.0f), true);
				Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Rigidbody Transform", Vector3(0.0f, 2.0f, 0.0f));
				Reference<Rigidbody> rigidbody = Object::Instantiate<Rigidbody>(transform);
				rigidbody->SetLockFlags(DynamicBody::LockFlags(DynamicBody::LockFlag::ROTATION_X, DynamicBody::LockFlag::ROTATION_Z));
				Reference<CapsuleCollider> collider = Object::Instantiate<CapsuleCollider>(rigidbody, "Rigidbody Collider", 0.25f, 0.5f, physMaterial);
				collider->SetTrigger(true);
				Reference<TriMesh> mesh = TriMesh::Capsule(Vector3(0.0f), collider->Radius(), collider->Height(), 32, 8, 2);
				Reference<Material> material = CreateMaterial(environment.RootObject(), 0xFFFFFFFF);
				Object::Instantiate<MeshRenderer>(transform, "Rigidbody Renderer", mesh, material);
				Object::Instantiate<ColorChanger>(collider, "Color Changer", Vector3(1.0f, 1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(-1.0f, 1.0f, 0.0f), true);
				static const auto jump = [](const Collider::ContactInfo& info) {
					if (info.EventType() == Collider::ContactType::ON_TRIGGER_PERSISTS) {
						Rigidbody* body = info.ReportingCollider()->GetComponentInParents<Rigidbody>();
						body->SetVelocity(body->Velocity() + Vector3(0.0f, 16.0f, 0.0f) * body->Context()->Physics()->ScaledDeltaTime());
					}
				};
				collider->OnContact() += Callback<const Collider::ContactInfo&>(jump);
				});
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		TEST(PhysicsTest, TriggerEvents_Dynamic_MoveManually) {
			Jimara::Test::TestEnvironment environment("Trigger contact reporting with dynamic rigidbodies, moved manually");
			CreateLights(environment.RootObject());
			Reference<PhysicsMaterial> physMaterial = environment.RootObject()->Context()->Physics()->APIInstance()->CreateMaterial(0.5, 0.5f, 0.0f);

			environment.ExecuteOnUpdateNow([&]() {
				CreateStaticBox(environment, physMaterial, Vector3(0.0f, -1.0f, 0.0f), Vector3(2.0f, 0.1f, 2.0f))->SetTrigger(true);
				Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Rigidbody Transform", Vector3(0.0f, 2.0f, 0.0f));
				Reference<Rigidbody> rigidbody = Object::Instantiate<Rigidbody>(transform);
				rigidbody->SetLockFlags(DynamicBody::LockFlags(
					DynamicBody::LockFlag::MOVEMENT_X, DynamicBody::LockFlag::MOVEMENT_Y, DynamicBody::LockFlag::MOVEMENT_Z,
					DynamicBody::LockFlag::ROTATION_X, DynamicBody::LockFlag::ROTATION_Y, DynamicBody::LockFlag::ROTATION_Z));
				Reference<CapsuleCollider> collider = Object::Instantiate<CapsuleCollider>(rigidbody, "Rigidbody Collider", 0.25f, 0.5f, physMaterial);
				Reference<TriMesh> mesh = TriMesh::Capsule(Vector3(0.0f), collider->Radius(), collider->Height(), 32, 8, 2);
				Reference<Material> material = CreateMaterial(environment.RootObject(), 0xFFFFFFFF);
				Object::Instantiate<MeshRenderer>(transform, "Rigidbody Renderer", mesh, material);
				Object::Instantiate<ColorChanger>(collider, "Color Changer", Vector3(1.0f, 1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(-1.0f, 1.0f, 0.0f), true);
				static Transform* trans;
				static Stopwatch stop;
				trans = transform;
				stop.Reset();
				collider->Context()->Graphics()->OnPostGraphicsSynch() += Callback<>([]() {
					trans->SetWorldPosition(Vector3(0.0f, sin(stop.Elapsed()) * 1.5f - 1.0f, 0.0f));
					});
				});
		}
	}
}

