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
			inline Reference<Material> CreateMaterial(Component* rootObject, uint32_t color) {
				Reference<Graphics::ImageTexture> texture = rootObject->Context()->Graphics()->Device()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
				(*static_cast<uint32_t*>(texture->Map())) = color;
				texture->Unmap(true);
				return Jimara::Test::SampleDiffuseShader::CreateMaterial(texture);
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
					transform->SetLocalScale(Vector3(1.5f) + 0.5f * cos(elapsed * 2.0f));
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
					Jimara::Test::TestEnvironment environment("PhysicsPlayground");
					{
						Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 0.25f, 0.0f)), "Light", Vector3(2.0f, 2.0f, 2.0f));
						Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, 2.0f)), "Light", Vector3(2.0f, 0.25f, 0.25f));
						Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, -2.0f)), "Light", Vector3(0.25f, 2.0f, 0.25f));
						Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, 2.0f)), "Light", Vector3(0.25f, 0.25f, 2.0f));
						Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, -2.0f)), "Light", Vector3(2.0f, 4.0f, 1.0f));
						Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 2.0f, 0.0f)), "Light", Vector3(1.0f, 4.0f, 2.0f));
					}
					{
						Reference<Transform> baseTransform = Object::Instantiate<Transform>(environment.RootObject(), "Base Transform");
						Reference<StaticBody> surface = environment.RootObject()->Context()->Physics()->AddStaticBody(baseTransform->WorldMatrix());
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
	}
}

