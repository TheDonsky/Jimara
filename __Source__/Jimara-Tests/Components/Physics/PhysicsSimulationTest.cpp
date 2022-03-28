#include "../../GtestHeaders.h"
#include "../../Memory.h"
#include "../TestEnvironment/TestEnvironment.h"
#include "Components/GraphicsObjects/MeshRenderer.h"
#include "Components/Lights/PointLight.h"
#include "Components/Lights/DirectionalLight.h"
#include "Components/Physics/BoxCollider.h"
#include "Components/Physics/SphereCollider.h"
#include "Components/Physics/CapsuleCollider.h"
#include "Components/Physics/MeshCollider.h"
#include "Data/Materials/SampleDiffuse/SampleDiffuseShader.h"
#include "Data/Generators/MeshGenerator.h"
#include <sstream>
#include <random>


namespace Jimara {
	namespace Physics {
		namespace {
			inline static Reference<Material> CreateMaterial(Component* rootObject, uint32_t color) {
				Reference<Graphics::ImageTexture> texture = rootObject->Context()->Graphics()->Device()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
				(*static_cast<uint32_t*>(texture->Map())) = color;
				texture->Unmap(true);
				return SampleDiffuseShader::CreateMaterial(texture, rootObject->Context()->Graphics()->Device());
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

			class Spowner : public virtual Scene::PhysicsContext::PostPhysicsSynchUpdatingComponent {
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

			class Platform : public virtual Scene::PhysicsContext::PostPhysicsSynchUpdatingComponent {
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
				const Reference<Material> m_material;
				const std::vector<Reference<TriMesh>> m_meshes;

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
					Material* material, const Reference<TriMesh>* meshes, size_t meshCount,
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
					Material* material, const Reference<TriMesh>* meshes, size_t meshCount,
					const Callback<Rigidbody*>& createCollider,
					const std::string_view& name, float interval = 0.125f, size_t maxCount = 512)
					: SimpleMeshSpowner(material, meshes, meshCount, Callback(&RadialMeshSpowner::Create, this), name, interval, maxCount)
					, m_create(createCollider) {}
			};

			class MeshDeformer : public virtual Scene::PhysicsContext::PostPhysicsSynchUpdatingComponent {
			private:
				const Reference<TriMesh> m_mesh;
				const Stopwatch m_stopwatch;

			public:
				inline MeshDeformer(Component* parent, const std::string& name, TriMesh* mesh)
					: Component(parent, name), m_mesh(mesh) {}

				inline virtual void PostPhysicsSynch() override {
					float time = m_stopwatch.Elapsed();
					TriMesh::Writer writer(m_mesh);
					for (uint32_t i = 0; i < writer.VertCount(); i++) {
						MeshVertex& vertex = writer.Vert(i);
						auto getY = [&](float x, float z) { return cos((time + ((x * x) + (z * z))) * 10.0f) * 0.05f; };
						vertex.position.y = getY(vertex.position.x, vertex.position.z);
						Vector3 dx = Vector3(vertex.position.x + 0.01f, 0.0f, vertex.position.z);
						dx.y = getY(dx.x, dx.z);
						Vector3 dz = Vector3(vertex.position.x, 0.0f, vertex.position.z + 0.01f);
						dz.y = getY(dz.x, dz.z);
						vertex.normal = Math::Normalize(Math::Cross(dz - vertex.position, dx - vertex.position));
					}
				}
			};
		}

		// Simple simulation and memory leak tests:
		TEST(PhysicsSimulationTest, Simulation) {
			typedef Reference<SpownerSettings>(*CreateSettings)(Component*);
			static auto createCollisionMesh = [](Component* root) {
				Reference<TriMesh> collisionMesh = GenerateMesh::Tri::Sphere(Vector3(0.0f), 2.0f, 5, 8);
				Reference<Material> material = CreateMaterial(root, 0xFFFFFFFF);
				Reference<Transform> meshColliderTransform = Object::Instantiate<Transform>(root, "MeshColliderTransform", Vector3(-3.0f, 0.0f, -2.0f));
				Object::Instantiate<MeshRenderer>(meshColliderTransform, "Mesh collider renderer", collisionMesh, material);
				Object::Instantiate<MeshCollider>(meshColliderTransform, "Mesh collider", collisionMesh);
				return meshColliderTransform;
			};
			const CreateSettings CREATE_SETTINGS[] = {
				[](Component* root) -> Reference<SpownerSettings> {
					// Simply spowns cubes at the center:
					createCollisionMesh(root);
					Reference<Material> material = CreateMaterial(root, 0xFFFFFFFF);
					Reference<TriMesh> mesh = GenerateMesh::Tri::Box(Vector3(-0.25f, -0.25f, -0.25f), Vector3(0.25f, 0.25f, 0.25f));
					Callback<Rigidbody*> createCollider = Callback<Rigidbody*>([](Rigidbody* rigidBody) {
						Object::Instantiate<BoxCollider>(rigidBody, "Box Collider", Vector3(0.5f, 0.5f, 0.5f));
						});
					Reference<SpownerSettings> settings = Object::Instantiate<SimpleMeshSpowner>(material, &mesh, 1, createCollider, "Spown Boxes");
					return settings;
				}, [](Component* root) -> Reference<SpownerSettings> {
					// Simply spowns capsules at the center:
					Reference<Material> material = CreateMaterial(root, 0xFFFFFFFF);
					Reference<TriMesh> mesh = GenerateMesh::Tri::Capsule(Vector3(0.0f), 0.15f, 0.7f, 16, 8, 4);
					Callback<Rigidbody*> createCollider = Callback<Rigidbody*>([](Rigidbody* rigidBody) {
						Object::Instantiate<CapsuleCollider>(rigidBody, "Capsule collider", 0.15f, 0.7f);
						});
					return Object::Instantiate<SimpleMeshSpowner>(material, &mesh, 1, createCollider, "Spown Capsules");
				}, [](Component* root) -> Reference<SpownerSettings> {
					// Spowns boxes and applies some velocity:
					Object::Instantiate<Platform>(createCollisionMesh(root), "Mesh collider");
					Reference<Material> material = CreateMaterial(root, 0xFFFFFFFF);
					Reference<TriMesh> mesh = GenerateMesh::Tri::Box(Vector3(-0.25f, -0.25f, -0.25f), Vector3(0.25f, 0.25f, 0.25f));
					Callback<Rigidbody*> createCollider = Callback<Rigidbody*>([](Rigidbody* rigidBody) {
						Object::Instantiate<BoxCollider>(rigidBody, "Box Collider", Vector3(0.5f, 0.5f, 0.5f));
						});
					return Object::Instantiate<RadialMeshSpowner>(material, &mesh, 1, createCollider, "Spown Boxes Radially");
				}, [](Component* root) -> Reference<SpownerSettings> {
					// Simply spowns spheres at the center and applies some velocity:
					Reference<Material> material = CreateMaterial(root, 0xFFFFFFFF);
					{
						Reference<TriMesh> collisionMesh = GenerateMesh::Tri::Plane(Vector3(0.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 2.0f), Size2(32, 32));
						Reference<Transform> meshColliderTransform = Object::Instantiate<Transform>(root, "MeshColliderTransform");
						meshColliderTransform->SetLocalScale(Vector3(16.0f));
						Object::Instantiate<MeshRenderer>(meshColliderTransform, "Mesh collider renderer", collisionMesh, material);
						Object::Instantiate<MeshCollider>(meshColliderTransform, "Mesh collider", collisionMesh);
						Object::Instantiate<MeshDeformer>(meshColliderTransform, "Mesh deformer", collisionMesh);
					}
					Reference<TriMesh> mesh = GenerateMesh::Tri::Sphere(Vector3(0.0f), 0.5f, 16, 8);
					Callback<Rigidbody*> createCollider = Callback<Rigidbody*>([](Rigidbody* rigidBody) {
						Object::Instantiate<SphereCollider>(rigidBody, "Sphere collider", 0.5f);
						});
					return Object::Instantiate<RadialMeshSpowner>(material, &mesh, 1, createCollider, "Spown Spheres");
				}, [](Component* root) -> Reference<SpownerSettings> {
					// Spowns capsules, applies some velocity and lcoks XZ rotation:
					Reference<Material> material = CreateMaterial(root, 0xFFFFFFFF);
					Reference<TriMesh> mesh = GenerateMesh::Tri::Capsule(Vector3(0.0f), 0.15f, 0.7f, 16, 8, 4);
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
						GenerateMesh::Tri::Box(Vector3(-0.25f, -0.25f, -0.25f), Vector3(0.25f, 0.25f, 0.25f)),
						GenerateMesh::Tri::Capsule(CAPSULE_OFFSET, 0.15f, 0.7f, 16, 8, 4),
						GenerateMesh::Tri::Sphere(SPHERE_OFFSET, 0.25f, 16, 8)
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
					Reference<TriMesh> mesh = GenerateMesh::Tri::Box(Vector3(-0.25f, -0.25f, -0.25f), Vector3(0.25f, 0.25f, 0.25f));
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
#ifndef NDEBUG
			size_t snapshot;
			auto updateSnapshot = [&]() { snapshot = Object::DEBUG_ActiveInstanceCount(); };
			auto compareSnapshot = [&]() -> bool { return snapshot == Object::DEBUG_ActiveInstanceCount(); };
#else
			auto updateSnapshot = [&]() {};
			auto compareSnapshot = [&]() -> bool { return true; };
#endif
			const size_t NUM_SETTINGS(sizeof(CREATE_SETTINGS) / sizeof(CreateSettings));
			for (size_t i = 0; i < NUM_SETTINGS; i++) {
				updateSnapshot();
				std::thread([&]() {
					Jimara::Test::TestEnvironment environment("Simulation", 2.0f);
					environment.ExecuteOnUpdateNow([&] {
						CreateLights(environment.RootObject());
						{
							Reference<Transform> baseTransform = Object::Instantiate<Transform>(environment.RootObject(), "Base Transform");
							const Vector3 extents(8.0f, 0.1f, 16.0f);
							Object::Instantiate<BoxCollider>(baseTransform, "Surface Object", extents);
							Reference<TriMesh> cube = GenerateMesh::Tri::Box(extents * -0.5f, extents * 0.5f);
							Reference<Material> material = CreateMaterial(environment.RootObject(), 0xFFFFFFFF);
							Object::Instantiate<MeshRenderer>(baseTransform, "Surface Renderer", cube, material);
							Object::Instantiate<Platform>(baseTransform, "Platform");
						}
						Reference<SpownerSettings> settings = CREATE_SETTINGS[i](environment.RootObject());
						environment.SetWindowName(settings->caseName);
						Object::Instantiate<Spowner>(environment.RootObject(), settings);
						});
					}).join();
					if (i > 0) { EXPECT_TRUE(compareSnapshot()); }
			}
			EXPECT_TRUE(compareSnapshot());
		}



		namespace {
			inline static Reference<Collider> CreateStaticBox(Jimara::Test::TestEnvironment& environment, PhysicsMaterial* physMaterial, const Vector3& position, const Vector3& size) {
				Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Box Transform", position);
				Reference<BoxCollider> collider = Object::Instantiate<BoxCollider>(transform, "Box Collider", size, physMaterial);
				Reference<TriMesh> mesh = GenerateMesh::Tri::Box(-collider->Size() * 0.5f, collider->Size() * 0.5f);
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
						float dt = info.ReportingCollider()->Context()->Physics()->Time()->ScaledDeltaTime();
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

		// Dynamic rigidbody collision event reporting:
		TEST(PhysicsSimulationTest, CollisionEvents_Dynamic) {
			Jimara::Test::TestEnvironment environment("Contact reporting with dynamic rigidbodies");
			environment.ExecuteOnUpdateNow([&]() { CreateLights(environment.RootObject()); });
			Reference<PhysicsMaterial> physMaterial = environment.RootObject()->Context()->Physics()->APIInstance()->CreateMaterial(0.5, 0.5f, 0.0f);

			environment.ExecuteOnUpdateNow([&]() {
				Object::Instantiate<ColorChanger>(
					CreateStaticBox(environment, physMaterial, Vector3(0.0f, -1.0f, 0.0f), Vector3(4.0f, 0.1f, 4.0f)),
					"Platform Color Changer", Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(1.0f, -1.0f, 0.0f));
				Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Rigidbody Transform", Vector3(0.0f, 2.0f, 0.0f));
				Reference<Rigidbody> rigidbody = Object::Instantiate<Rigidbody>(transform);
				rigidbody->SetLockFlags(DynamicBody::LockFlags(DynamicBody::LockFlag::ROTATION_X, DynamicBody::LockFlag::ROTATION_Z));
				Reference<CapsuleCollider> collider = Object::Instantiate<CapsuleCollider>(rigidbody, "Rigidbody Collider", 0.25f, 0.5f, physMaterial);
				Reference<TriMesh> mesh = GenerateMesh::Tri::Capsule(Vector3(0.0f), collider->Radius(), collider->Height(), 32, 8, 2);
				Reference<Material> material = CreateMaterial(environment.RootObject(), 0xFFFFFFFF);
				Object::Instantiate<MeshRenderer>(transform, "Rigidbody Renderer", mesh, material);
				Object::Instantiate<ColorChanger>(collider, "Color Changer");
				static const auto jump = [](const Collider::ContactInfo& info) {
					if (info.EventType() == Collider::ContactType::ON_COLLISION_PERSISTS && info.ReportingCollider()->GetComponentInChildren<ColorChanger>()->Color().g >= 1.0f)
						info.ReportingCollider()->GetComponentInParents<Rigidbody>()->SetVelocity(8.0f * info.TouchPoint(0).normal);
				};
				collider->OnContact() += Callback<const Collider::ContactInfo&>(jump);
				});
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		// Test for what happens when one of the touching bodies gets destroyed
		TEST(PhysicsSimulationTest, CollisionEvents_Dynamic_DestroyOnTouch) {
			Jimara::Test::TestEnvironment environment("Contact reporting with dynamic rigidbodies (destroy on touch)");
			environment.ExecuteOnUpdateNow([&]() { CreateLights(environment.RootObject()); });
			Reference<PhysicsMaterial> physMaterial = environment.RootObject()->Context()->Physics()->APIInstance()->CreateMaterial(0.5, 0.5f, 0.0f);

			environment.ExecuteOnUpdateNow([&]() {
				Object::Instantiate<ColorChanger>(
					CreateStaticBox(environment, physMaterial, Vector3(0.0f, -1.0f, 0.0f), Vector3(4.0f, 0.1f, 4.0f)),
					"Platform Color Changer", Vector3(1.0f, 1.0f, 1.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(1.0f, -1.0f, 0.0f));
				static void(*recreateOnTouch)(const Collider::ContactInfo&);
				static auto create = [](Component* rootObject, PhysicsMaterial* physMaterial) {
					Reference<Transform> transform = Object::Instantiate<Transform>(rootObject, "Rigidbody Transform", Vector3(0.0f, 2.0f, 0.0f));
					Reference<Rigidbody> rigidbody = Object::Instantiate<Rigidbody>(transform);
					rigidbody->SetLockFlags(DynamicBody::LockFlags(DynamicBody::LockFlag::ROTATION_X, DynamicBody::LockFlag::ROTATION_Z));
					Reference<CapsuleCollider> collider = Object::Instantiate<CapsuleCollider>(rigidbody, "Rigidbody Collider", 0.25f, 0.5f, physMaterial);
					Reference<TriMesh> mesh = GenerateMesh::Tri::Capsule(Vector3(0.0f), collider->Radius(), collider->Height(), 32, 8, 2);
					Reference<Material> material = CreateMaterial(rootObject, 0xFFFFFFFF);
					Object::Instantiate<MeshRenderer>(transform, "Rigidbody Renderer", mesh, material);
					collider->OnContact() += Callback<const Collider::ContactInfo&>(recreateOnTouch);
				};
				recreateOnTouch = [](const Collider::ContactInfo& info) {
					Reference<PhysicsMaterial> physMaterial = dynamic_cast<CapsuleCollider*>(info.ReportingCollider())->Material();
					info.ReportingCollider()->GetTransfrom()->Destroy();
					create(info.OtherCollider()->RootObject(), physMaterial);
				};
				create(environment.RootObject(), physMaterial);
				});
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		// Contact reporting with dynamic rigidbodies, moved manually
		TEST(PhysicsSimulationTest, CollisionEvents_Dynamic_MoveManually) {
			Jimara::Test::TestEnvironment environment("Contact reporting with dynamic rigidbodies, moved manually");
			environment.ExecuteOnUpdateNow([&]() { CreateLights(environment.RootObject()); });
			Reference<PhysicsMaterial> physMaterial = environment.RootObject()->Context()->Physics()->APIInstance()->CreateMaterial(0.5, 0.5f, 0.0f);

			environment.ExecuteOnUpdateNow([&]() {
				CreateStaticBox(environment, physMaterial, Vector3(0.0f, -1.0f, 0.0f), Vector3(2.0f, 0.1f, 2.0f));
				Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Rigidbody Transform", Vector3(0.0f, 2.0f, 0.0f));
				Reference<Rigidbody> rigidbody = Object::Instantiate<Rigidbody>(transform);
				rigidbody->SetLockFlags(DynamicBody::LockFlags(
					DynamicBody::LockFlag::MOVEMENT_X, DynamicBody::LockFlag::MOVEMENT_Y, DynamicBody::LockFlag::MOVEMENT_Z,
					DynamicBody::LockFlag::ROTATION_X, DynamicBody::LockFlag::ROTATION_Y, DynamicBody::LockFlag::ROTATION_Z));
				Reference<CapsuleCollider> collider = Object::Instantiate<CapsuleCollider>(rigidbody, "Rigidbody Collider", 0.25f, 0.5f, physMaterial);
				Reference<TriMesh> mesh = GenerateMesh::Tri::Capsule(Vector3(0.0f), collider->Radius(), collider->Height(), 32, 8, 2);
				Reference<Material> material = CreateMaterial(environment.RootObject(), 0xFFFFFFFF);
				Object::Instantiate<MeshRenderer>(transform, "Rigidbody Renderer", mesh, material);
				Object::Instantiate<ColorChanger>(collider, "Color Changer");
				static Transform* trans;
				static Stopwatch stop;
				trans = transform;
				stop.Reset();
				collider->Context()->Graphics()->OnGraphicsSynch() += Callback<>([]() {
					trans->SetWorldPosition(Vector3(0.0f, sin(stop.Elapsed()) * 1.5f - 1.0f, 0.0f));
					});
				});
		}

		// Contact reporting with kinematic rigidbodies, moved manually (kinematic-kinematic contacts)
		TEST(PhysicsSimulationTest, CollisionEvents_Kinematic_MoveManually) {
			Jimara::Test::TestEnvironment environment("Contact reporting with kinematic rigidbodies, moved manually");
			environment.ExecuteOnUpdateNow([&]() { CreateLights(environment.RootObject()); });
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
				Reference<TriMesh> mesh = GenerateMesh::Tri::Capsule(Vector3(0.0f), collider->Radius(), collider->Height(), 32, 8, 2);
				Reference<Material> material = CreateMaterial(environment.RootObject(), 0xFFFFFFFF);
				Object::Instantiate<MeshRenderer>(transform, "Rigidbody Renderer", mesh, material);
				Object::Instantiate<ColorChanger>(collider, "Color Changer");
				static Transform* trans;
				static Stopwatch stop;
				trans = transform;
				stop.Reset();
				collider->Context()->Graphics()->OnGraphicsSynch() += Callback<>([]() {
					trans->SetWorldPosition(Vector3(0.0f, sin(stop.Elapsed()) * 1.5f - 1.0f, 0.0f));
					});
				});
		}

		// Trigger contact event reporting with dynamic rigidbodies
		TEST(PhysicsSimulationTest, TriggerEvents_Dynamic) {
			Jimara::Test::TestEnvironment environment("Trigger contact reporting with dynamic rigidbodies");
			environment.ExecuteOnUpdateNow([&]() { CreateLights(environment.RootObject()); });
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
				Reference<TriMesh> mesh = GenerateMesh::Tri::Capsule(Vector3(0.0f), collider->Radius(), collider->Height(), 32, 8, 2);
				Reference<Material> material = CreateMaterial(environment.RootObject(), 0xFFFFFFFF);
				Object::Instantiate<MeshRenderer>(transform, "Rigidbody Renderer", mesh, material);
				Object::Instantiate<ColorChanger>(collider, "Color Changer", Vector3(1.0f, 1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(-1.0f, 1.0f, 0.0f), true);
				static const auto jump = [](const Collider::ContactInfo& info) {
					if (info.EventType() == Collider::ContactType::ON_TRIGGER_PERSISTS) {
						Rigidbody* body = info.ReportingCollider()->GetComponentInParents<Rigidbody>();
						body->SetVelocity(body->Velocity() + Vector3(0.0f, 16.0f, 0.0f) * body->Context()->Physics()->Time()->ScaledDeltaTime());
					}
				};
				collider->OnContact() += Callback<const Collider::ContactInfo&>(jump);
				});
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}

		// Trigger contact event reporting with dynamic rigidbodies, moved manually
		TEST(PhysicsSimulationTest, TriggerEvents_Dynamic_MoveManually) {
			Jimara::Test::TestEnvironment environment("Trigger contact reporting with dynamic rigidbodies, moved manually");
			environment.ExecuteOnUpdateNow([&]() { CreateLights(environment.RootObject()); });
			Reference<PhysicsMaterial> physMaterial = environment.RootObject()->Context()->Physics()->APIInstance()->CreateMaterial(0.5, 0.5f, 0.0f);

			environment.ExecuteOnUpdateNow([&]() {
				CreateStaticBox(environment, physMaterial, Vector3(0.0f, -1.0f, 0.0f), Vector3(2.0f, 0.1f, 2.0f))->SetTrigger(true);
				Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Rigidbody Transform", Vector3(0.0f, 2.0f, 0.0f));
				Reference<Rigidbody> rigidbody = Object::Instantiate<Rigidbody>(transform);
				rigidbody->SetLockFlags(DynamicBody::LockFlags(
					DynamicBody::LockFlag::MOVEMENT_X, DynamicBody::LockFlag::MOVEMENT_Y, DynamicBody::LockFlag::MOVEMENT_Z,
					DynamicBody::LockFlag::ROTATION_X, DynamicBody::LockFlag::ROTATION_Y, DynamicBody::LockFlag::ROTATION_Z));
				Reference<CapsuleCollider> collider = Object::Instantiate<CapsuleCollider>(rigidbody, "Rigidbody Collider", 0.25f, 0.5f, physMaterial);
				collider->SetTrigger(true);
				Reference<TriMesh> mesh = GenerateMesh::Tri::Capsule(Vector3(0.0f), collider->Radius(), collider->Height(), 32, 8, 2);
				Reference<Material> material = CreateMaterial(environment.RootObject(), 0xFFFFFFFF);
				Object::Instantiate<MeshRenderer>(transform, "Rigidbody Renderer", mesh, material);
				Object::Instantiate<ColorChanger>(collider, "Color Changer", Vector3(1.0f, 1.0f, 1.0f), Vector3(1.0f, 0.0f, 0.0f), Vector3(-1.0f, 1.0f, 0.0f), true);
				static Transform* trans;
				static Stopwatch stop;
				trans = transform;
				stop.Reset();
				collider->Context()->Graphics()->OnGraphicsSynch() += Callback<>([]() {
					trans->SetWorldPosition(Vector3(0.0f, sin(stop.Elapsed()) * 1.5f - 1.0f, 0.0f));
					});
				});
		}



		namespace {
			class TimeBomb : public virtual Scene::PhysicsContext::PostPhysicsSynchUpdatingComponent {
			private:
				const float m_timeout;
				Stopwatch m_stopwatch;

			public:
				inline virtual void PostPhysicsSynch() override{ 
					if (m_stopwatch.Elapsed() < m_timeout) return;
					Component* component = GetTransfrom();
					if (component == nullptr) component = this;
					component->Destroy();
				}

				inline TimeBomb(Component* parent, const std::string_view& name, float timeout = 1.0f)
					: Component(parent, name), m_timeout(timeout) {}
			};

			enum class Layers : uint8_t {
				GROUND = 0,
				DETONATOR = 1,
				BOMB = 2,
				SPARKS = 3
			};
		}

		// Basic filtering test 
		// ('ground' should not interact with 'bombs', 'bombs' should explode if they touch 'detonators' and become blue if they touch anything else; 
		// 'sparks' do not interact with each other and the 'detonators')
		TEST(PhysicsSimulationTest, Filtering) {
			Jimara::Test::TestEnvironment environment("Filtering");
			environment.RootObject()->Context()->Physics()->FilterLayerInteraction(Layers::GROUND, Layers::BOMB, false);
			environment.RootObject()->Context()->Physics()->FilterLayerInteraction(Layers::DETONATOR, Layers::SPARKS, false);
			environment.RootObject()->Context()->Physics()->FilterLayerInteraction(Layers::SPARKS, Layers::SPARKS, false);
			environment.ExecuteOnUpdateNow([&]() { CreateLights(environment.RootObject()); });
			Reference<PhysicsMaterial> physMaterial = environment.RootObject()->Context()->Physics()->APIInstance()->CreateMaterial(0.5, 0.5f, 0.75f);
			environment.ExecuteOnUpdateNow([&] {
				CreateStaticBox(environment, physMaterial, Vector3(0.0f, -1.0f, 0.0f), Vector3(24.0f, 0.1f, 24.0f))->SetLayer(Layers::GROUND);
				
				const float DETONATOR_RADIUS = 0.75f;
				const Reference<TriMesh> detonatorMesh = GenerateMesh::Tri::Sphere(Vector3(0.0f), DETONATOR_RADIUS, 32, 16);
				const Reference<Material> detonatorMaterial = CreateMaterial(environment.RootObject(), 0xFF00FF00);
				const size_t DETONATOR_COUNT = 8;
				for (size_t i = 0; i < DETONATOR_COUNT; i++) {
					float angle = Math::Radians(360.0f / static_cast<float>(DETONATOR_COUNT) * i);
					Reference<Transform> transform = Object::Instantiate<Transform>(environment.RootObject(), "Detonator", Vector3(cos(angle), 0.0f, sin(angle)) * 5.0f);
					Reference<Rigidbody> rigidbody = Object::Instantiate<Rigidbody>(transform, "Detonator Body");
					rigidbody->SetKinematic(true);
					Reference<Collider> collider = Object::Instantiate<SphereCollider>(rigidbody, "Detonator Collider", DETONATOR_RADIUS);
					collider->SetLayer(Layers::DETONATOR);
					Object::Instantiate<MeshRenderer>(transform, "Detonator Renderer", detonatorMesh, detonatorMaterial);
				}
				
				const Reference<Material> bombMaterial = CreateMaterial(environment.RootObject(), 0xFF0000FF);
				static const Vector3 BOMB_CAPSULE_OFFSET(0.0f, -0.3f, 0.0f);
				static const Vector3 BOMB_SPHERE_OFFSET(0.0f, 0.5f, 0.0f);
				Reference<TriMesh> meshes[] = {
					GenerateMesh::Tri::Box(Vector3(-0.25f, -0.25f, -0.25f), Vector3(0.25f, 0.25f, 0.25f)),
					GenerateMesh::Tri::Capsule(BOMB_CAPSULE_OFFSET, 0.15f, 0.7f, 16, 8, 4),
					GenerateMesh::Tri::Sphere(BOMB_SPHERE_OFFSET, 0.25f, 16, 8)
				};
				static MeshRenderer* sparkMaterialHolder = nullptr;
				static const Vector3 SPARK_SIZE = Vector3(0.1f);
				sparkMaterialHolder = Object::Instantiate<MeshRenderer>(environment.RootObject(), "Detonator Renderer", 
					GenerateMesh::Tri::Box(-SPARK_SIZE * 0.5f, SPARK_SIZE * 0.5f), CreateMaterial(environment.RootObject(), 0xFFFF0000));
				Callback<Rigidbody*> createCollider = Callback<Rigidbody*>([](Rigidbody* rigidBody) {
					const Reference<Collider> colliders[] = {
						Object::Instantiate<BoxCollider>(rigidBody, "Box Collider", Vector3(0.5f, 0.5f, 0.5f)),
						Object::Instantiate<CapsuleCollider>(Object::Instantiate<Transform>(rigidBody, "Capsule Transform", BOMB_CAPSULE_OFFSET), "Capsule collider", 0.15f, 0.7f),
						Object::Instantiate<SphereCollider>(Object::Instantiate<Transform>(rigidBody, "Sphere Transform", BOMB_SPHERE_OFFSET), "Sphere collider", 0.25f)
					};
					for (size_t i = 0; i < (sizeof(colliders) / sizeof(const Reference<Collider>)); i++) {
						Collider* collider = colliders[i];
						collider->SetLayer(Layers::BOMB);
						collider->OnContact() += Callback<const Collider::ContactInfo&>([](const Collider::ContactInfo& info) {
							if (info.EventType() != Collider::ContactType::ON_COLLISION_BEGIN) return;
							const Reference<Rigidbody> body = info.ReportingCollider()->GetComponentInParents<Rigidbody>();
							if (body == nullptr) return;
							const Reference<Transform> transform = body->GetTransfrom();
							if (transform == nullptr) return;
							else if (info.OtherCollider()->GetLayer() != static_cast<Physics::PhysicsCollider::Layer>(Layers::DETONATOR)) {
								std::vector<MeshRenderer*> renderers = transform->GetComponentsInChildren<MeshRenderer>();
								for (size_t i = 0; i < renderers.size(); i++) renderers[i]->SetMaterial(sparkMaterialHolder->Material());
								return;
							}
							else {
								Vector3 center = transform->WorldPosition();
								transform->Destroy();
								static std::mt19937 generator;
								std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
								for (size_t i = 0; i < 24; i++) {
									Reference<Transform> sparkTransform = Object::Instantiate<Transform>(info.OtherCollider()->RootObject(), "Spark", center);
									Reference<Rigidbody> sparkBody = Object::Instantiate<Rigidbody>(sparkTransform, "Spark Body");
									float theta = 2 * Math::Pi() * distribution(generator);
									float phi = acos(1 - 2 * distribution(generator));
									sparkBody->SetVelocity(Vector3(sin(phi) * cos(theta), sin(phi) * sin(theta), cos(phi)) * 12.0f);
									Reference<Collider> collider = Object::Instantiate<BoxCollider>(sparkBody, "Spark Collider", SPARK_SIZE);
									collider->SetLayer(Layers::SPARKS);
									Object::Instantiate<MeshRenderer>(sparkTransform, "Spark Renderer", sparkMaterialHolder->Mesh(), sparkMaterialHolder->Material());
									Object::Instantiate<TimeBomb>(sparkTransform, "Spark Time Bomb", 3.0f);
								}
							}
							});
					}
					});
				Object::Instantiate<Spowner>(environment.RootObject(), Object::Instantiate<RadialMeshSpowner>(bombMaterial, meshes, 3, createCollider, "Filtering", 0.2f));
				});
		}
	}
}

