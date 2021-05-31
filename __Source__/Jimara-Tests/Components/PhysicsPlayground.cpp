#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Shaders/SampleDiffuseShader.h"
#include "TestEnvironment/TestEnvironment.h"
#include "Components/MeshRenderer.h"
#include "Components/Lights/PointLight.h"
#include "Components/Lights/DirectionalLight.h"
#include "Components/Interfaces/Updatable.h"
#include <sstream>

#include "Physics/PhysX/PhysXScene.h"


namespace Jimara {
	namespace Physics {
		namespace {
			class ColliderObject : public virtual Component, public virtual Updatable {
			private:
				const Reference<PhysicsBody> m_body;
				const Reference<PhysicsCollider> m_collder;

			public:
				inline ColliderObject(Component* parent, const std::string_view& name, PhysicsBody* body, PhysicsCollider* collider)
					: Component(parent, name), m_body(body), m_collder(collider) {}

				virtual void Update()override {
					Transform* transform = GetTransfrom();
					if (transform == nullptr) return;
					Matrix4 pose = m_body->GetPose();
					transform->SetWorldPosition(pose[3]);
					pose[3] = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
					transform->SetWorldEulerAngles(Math::EulerAnglesFromMatrix(pose));
				}
			};

			class PhysicsUpdater : public virtual Component, public virtual Updatable {
			private:
				const Reference<PhysicsScene> m_scene;
				Stopwatch m_stopwatch;
				std::atomic<bool> m_simulated = false;
				EventInstance<float> m_preUpdate;
				EventInstance<float> m_onUpdate;

			public:
				inline PhysicsUpdater(Component* parent, const std::string_view& name, PhysicsScene* scene)
					: Component(parent, name), m_scene(scene) {}

				inline virtual void Update()override {
					if (m_stopwatch.Elapsed() < 0.01f) return;
					float elapsed = m_stopwatch.Reset();
					m_preUpdate(elapsed);
					if (m_simulated) m_scene->SynchSimulation();
					else m_simulated = true;
					m_scene->SimulateAsynch(elapsed);
					m_onUpdate(elapsed);
				}

				inline PhysicsScene* Scene()const { return m_scene; }

				inline Event<float>& PreUpdate() { return m_preUpdate; }
				inline Event<float>& OnUpdate() { return m_onUpdate; }
			};

			inline Reference<Material> CreateMaterial(Jimara::Test::TestEnvironment* testEnvironment, uint32_t color) {
				Reference<Graphics::ImageTexture> texture = testEnvironment->RootObject()->Context()->Graphics()->Device()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
				(*static_cast<uint32_t*>(texture->Map())) = color;
				texture->Unmap(true);
				return Jimara::Test::SampleDiffuseShader::CreateMaterial(texture);
			};

			class Spowner : public virtual Component {
			private:
				const Reference<PhysicsUpdater> m_updater;
				const Reference<Material> m_material;
				const Reference<TriMesh> m_mesh;
				Stopwatch m_stopwatch;
				std::queue<Reference<Transform>> m_transformQueue;

				void PreUpdate(float deltaTime) {

				}

				void OnUpdate(float deltaTime) {
					if (m_stopwatch.Elapsed() < 0.125f) return;
					m_stopwatch.Reset();
					Reference<Transform> rigidTransform = Object::Instantiate<Transform>(RootObject(), "Rigid Transform", Vector3(0.0f, 1.0f, 0.0f));
					Reference<RigidBody> rigidBody = m_updater->Scene()->AddRigidBody(rigidTransform->WorldMatrix());
					Reference<PhysicsCollider> rigidCollider = rigidBody->AddCollider(BoxShape(Vector3(0.5f, 0.5f, 0.5f)), nullptr);
					Object::Instantiate<ColliderObject>(rigidTransform, "RigidBody Object", rigidBody, rigidCollider);
					Object::Instantiate<MeshRenderer>(rigidTransform, "RigidBody Renderer", m_mesh, m_material);
					m_transformQueue.push(rigidTransform);
					while (m_transformQueue.size() > 512) {
						Reference<Transform> t = m_transformQueue.front();
						m_transformQueue.pop();
						t->Destroy();
					}
				}


			public:
				inline Spowner(Component* parent, const std::string_view& name, PhysicsUpdater* updater, Material* material)
					: Component(parent, name), m_updater(updater), m_material(material)
					, m_mesh(TriMesh::Box(Vector3(-0.25f, -0.25f, -0.25f), Vector3(0.25f, 0.25f, 0.25f))) {
					m_updater->PreUpdate() += Callback(&Spowner::PreUpdate, this);
					m_updater->OnUpdate() += Callback(&Spowner::OnUpdate, this);
				}

				inline virtual ~Spowner() {
					m_updater->PreUpdate() -= Callback(&Spowner::PreUpdate, this);
					m_updater->OnUpdate() -= Callback(&Spowner::OnUpdate, this);
				}
			};
		}

		TEST(PhysicsPlayground, Playground) {
			Jimara::Test::TestEnvironment environment("PhysicsPlayground");
			Reference<PhysicsInstance> instance = PhysicsInstance::Create(environment.RootObject()->Context()->Log());
			ASSERT_NE(instance, nullptr);
			Reference<PhysicsScene> scene = instance->CreateScene(std::thread::hardware_concurrency() / 4);
			ASSERT_NE(scene, nullptr);
			ASSERT_EQ(scene->APIInstance(), instance);
			ASSERT_EQ(scene->Gravity(), PhysicsInstance::DefaultGravity());
			environment.RootObject()->Context()->Log()->Info("Gravity: <", scene->Gravity().x, "; ", scene->Gravity().y, "; ", scene->Gravity().z, ">");

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
				Reference<StaticBody> surface = scene->AddStaticBody(baseTransform->WorldMatrix());
				const Vector3 extents(8.0f, 0.1f, 16.0f);
				Reference<PhysicsCollider> surfaceCollider = surface->AddCollider(BoxShape(extents), nullptr);
				Object::Instantiate<ColliderObject>(baseTransform, "Surface Object", surface, surfaceCollider);
				Reference<TriMesh> cube = TriMesh::Box(extents * -0.5f, extents * 0.5f);
				Reference<Material> material = CreateMaterial(&environment, 0xFFFFFFFF);
				Object::Instantiate<MeshRenderer>(baseTransform, "Surface Renderer", cube, material);
			}

			Object::Instantiate<Spowner>(
				environment.RootObject(), "Spowner", Object::Instantiate<PhysicsUpdater>(environment.RootObject(), "Physics Updater", scene), CreateMaterial(&environment, 0x00FFFFFF));
		}
	}
}

