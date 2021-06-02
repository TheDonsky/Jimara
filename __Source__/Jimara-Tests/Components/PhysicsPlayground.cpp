#include "../GtestHeaders.h"
#include "../Memory.h"
#include "Shaders/SampleDiffuseShader.h"
#include "TestEnvironment/TestEnvironment.h"
#include "Components/MeshRenderer.h"
#include "Components/Lights/PointLight.h"
#include "Components/Lights/DirectionalLight.h"
#include "Components/Physics/BoxCollider.h"
#include <sstream>

#include "Physics/PhysX/PhysXScene.h"


namespace Jimara {
	namespace Physics {
		namespace {
			inline Reference<Material> CreateMaterial(Jimara::Test::TestEnvironment* testEnvironment, uint32_t color) {
				Reference<Graphics::ImageTexture> texture = testEnvironment->RootObject()->Context()->Graphics()->Device()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
				(*static_cast<uint32_t*>(texture->Map())) = color;
				texture->Unmap(true);
				return Jimara::Test::SampleDiffuseShader::CreateMaterial(texture);
			};

			class Spowner : public virtual Component, public virtual PostPhysicsSynchUpdater {
			private:
				const Reference<Material> m_material;
				const Reference<TriMesh> m_mesh;
				Stopwatch m_stopwatch;
				std::queue<Reference<Transform>> m_transformQueue;
				float m_totalTime = 0.0f;

			public:
				inline virtual void PostPhysicsSynch()override {
					if (m_stopwatch.Elapsed() < 0.125f) return;
					m_totalTime += m_stopwatch.Reset();
					Reference<Transform> rigidTransform = Object::Instantiate<Transform>(RootObject(), "Rigid Transform", Vector3(0.0f, 1.0f, 0.0f));
					Reference<Rigidbody> rigidBody = Object::Instantiate<Rigidbody>(rigidTransform);
					rigidBody->SetVelocity(Vector3(3.0f * cos(m_totalTime * 2.0f), 7.0f, 3.0f * sin(m_totalTime * 2.0f)));
					rigidBody->SetLockFlags(Physics::DynamicBody::LockFlags(
						Physics::DynamicBody::LockFlag::ROTATION_X, Physics::DynamicBody::LockFlag::ROTATION_Z));
					Object::Instantiate<BoxCollider>(rigidBody, "RigidBody Object", Vector3(0.5f, 0.5f, 0.5f));
					Object::Instantiate<MeshRenderer>(rigidTransform, "RigidBody Renderer", m_mesh, m_material);
					m_transformQueue.push(rigidTransform);
					while (m_transformQueue.size() > 512) {
						Reference<Transform> t = m_transformQueue.front();
						m_transformQueue.pop();
						t->Destroy();
					}
				}

				inline Spowner(Component* parent, const std::string_view& name, Material* material)
					: Component(parent, name), m_material(material)
					, m_mesh(TriMesh::Box(Vector3(-0.25f, -0.25f, -0.25f), Vector3(0.25f, 0.25f, 0.25f))) {}
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
		}

		TEST(PhysicsPlayground, Playground) {
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
				Reference<Material> material = CreateMaterial(&environment, 0xFFFFFFFF);
				Object::Instantiate<MeshRenderer>(baseTransform, "Surface Renderer", cube, material);
				Object::Instantiate<Platform>(baseTransform, "Platform");
			}
			Object::Instantiate<Spowner>(environment.RootObject(), "Spowner", CreateMaterial(&environment, 0x00FFFFFF));
		}
	}
}

