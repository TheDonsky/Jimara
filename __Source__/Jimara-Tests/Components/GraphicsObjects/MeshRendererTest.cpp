#include "../../GtestHeaders.h"
#include "../../Memory.h"
#include "../TestEnvironment/TestEnvironment.h"
#include "Data/Materials/SampleDiffuse/SampleDiffuseShader.h"
#include "Components/GraphicsObjects/MeshRenderer.h"
#include "Components/Lights/PointLight.h"
#include "Components/Lights/DirectionalLight.h"
#include "Data/Formats/WavefrontOBJ.h"
#include "Data/Generators/MeshGenerator.h"
#include "Math/Random.h"
#include <random>
#include <cmath>


namespace Jimara {
	// Renders axis-facing cubes to make sure our coordinate system behaves
	TEST(MeshRendererTest, AxisTest) {
		Jimara::Test::TestEnvironment environment("AxisTest <X-red, Y-green, Z-blue>");
		
		environment.ExecuteOnUpdateNow([&]() {
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(1.0f, 1.0f, 1.0f)), "Light", Vector3(2.5f, 2.5f, 2.5f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-1.0f, 1.0f, 1.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(1.0f, 1.0f, -1.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-1.0f, 1.0f, -1.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
			});
		
		auto createMaterial = [&](uint32_t color) -> Reference<Material> {
			Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true, 
				Graphics::ImageTexture::AccessFlags::NONE);
			(*static_cast<uint32_t*>(texture->Map())) = color;
			texture->Unmap(true);
			return Jimara::SampleDiffuseShader::CreateMaterial(texture, environment.RootObject()->Context()->Graphics()->Device());
		};
		
		Reference<TriMesh> box = GenerateMesh::Tri::Box(Vector3(-0.5f, -0.5f, -0.5f), Vector3(0.5f, 0.5f, 0.5f));

		environment.ExecuteOnUpdateNow([&]() {
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Center");
			Reference<Material> material = createMaterial(0xFF888888);
			Reference<TriMesh> sphere = GenerateMesh::Tri::Sphere(Vector3(0.0f, 0.0f, 0.0f), 0.1f, 32, 16);
			Object::Instantiate<MeshRenderer>(transform, "Center_Renderer", sphere, material);
			});
		environment.ExecuteOnUpdateNow([&]() {
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "X");
			transform->SetLocalPosition(Vector3(0.5f, 0.0f, 0.0f));
			transform->SetLocalScale(Vector3(1.0f, 0.075f, 0.075f));
			Reference<Material> material = createMaterial(0xFF0000FF);
			Object::Instantiate<MeshRenderer>(transform, "X_Renderer", box, material);
			});
		environment.ExecuteOnUpdateNow([&]() {
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Y");
			transform->SetLocalPosition(Vector3(0.0f, 0.5f, 0.0f));
			transform->SetLocalScale(Vector3(0.075f, 1.0f, 0.075f));
			Reference<Material> material = createMaterial(0xFF00FF00);
			Object::Instantiate<MeshRenderer>(transform, "Y_Renderer", box, material);
			});
		environment.ExecuteOnUpdateNow([&]() {
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Z");
			transform->SetLocalPosition(Vector3(0.0f, 0.0f, 0.5f));
			transform->SetLocalScale(Vector3(0.075f, 0.075f, 1.0f));
			Reference<Material> material = createMaterial(0xFFFF0000);
			Object::Instantiate<MeshRenderer>(transform, "Z_Renderer", box, material);
			});
	}





	// Creates a bounch of objects and makes them look at the center
	TEST(MeshRendererTest, CenterFacingInstances) {
		Jimara::Test::TestEnvironment environment("Center Facing Instances");
		
		{
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 0.25f, 0.0f)), "Light", Vector3(2.0f, 2.0f, 2.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, 2.0f)), "Light", Vector3(2.0f, 0.25f, 0.25f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, -2.0f)), "Light", Vector3(0.25f, 2.0f, 0.25f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, 2.0f)), "Light", Vector3(0.25f, 0.25f, 2.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, -2.0f)), "Light", Vector3(2.0f, 4.0f, 1.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 2.0f, 0.0f)), "Light", Vector3(1.0f, 4.0f, 2.0f));
		}

		std::uniform_real_distribution<float> dis(-4.0f, 4.0f);

		Reference<TriMesh> sphereMesh = GenerateMesh::Tri::Sphere(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 16, 8);
		Reference<TriMesh> cubeMesh = GenerateMesh::Tri::Box(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));

		Reference<Material> material = [&]() -> Reference<Material> {
			Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true,
				Graphics::ImageTexture::AccessFlags::NONE);
			(*static_cast<uint32_t*>(texture->Map())) = 0xFFFFFFFF;
			texture->Unmap(true);
			return Jimara::SampleDiffuseShader::CreateMaterial(texture, environment.RootObject()->Context()->Graphics()->Device());
		}();

		environment.ExecuteOnUpdateNow([&]() {
			Reference<TriMesh> mesh = GenerateMesh::Tri::Sphere(Vector3(0.0f, 0.0f, 0.0f), 1.0f, 64, 32, "Center");
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Center");
			transform->SetLocalScale(Vector3(0.35f));
			Object::Instantiate<MeshRenderer>(transform, "Center_Renderer", mesh, material);
			});

		environment.ExecuteOnUpdateNow([&]() { for (size_t i = 0; i < 2048; i++) {
			Transform* parent = Object::Instantiate<Transform>(environment.RootObject(), "Parent");
			{
				parent->SetLocalPosition(Vector3(dis(Random::ThreadRNG()), dis(Random::ThreadRNG()), dis(Random::ThreadRNG())));
				parent->SetLocalScale(Vector3(0.125f));
				parent->LookAt(Vector3(0.0f));
			}
			{
				Transform* sphereChild = Object::Instantiate<Transform>(parent, "Sphere");
				Reference<MeshRenderer> sphereRenderer = Object::Instantiate<MeshRenderer>(sphereChild, "Sphere_Renderer", sphereMesh, material);
				sphereChild->SetLocalScale(Vector3(0.35f));
				sphereRenderer->MarkStatic(true);
			}
			{
				Transform* cubeChild = Object::Instantiate<Transform>(parent, "Cube");
				Reference<MeshRenderer> cubeRenderer = Object::Instantiate<MeshRenderer>(cubeChild, "Box_Renderer", cubeMesh, material);
				cubeChild->SetLocalPosition(Vector3(0.0f, 0.0f, -1.0f));
				cubeChild->SetLocalScale(Vector3(0.25f, 0.25f, 1.0f));
				cubeRenderer->MarkStatic(true);
			}
			{
				Transform* upIndicator = Object::Instantiate<Transform>(parent, "UpIndicator");
				Reference<MeshRenderer> upRenderer = Object::Instantiate<MeshRenderer>(upIndicator, "UpIndicator_Renderer", cubeMesh, material);
				upIndicator->SetLocalPosition(Vector3(0.0f, 0.5f, -0.5f));
				upIndicator->SetLocalScale(Vector3(0.0625f, 0.5f, 0.0625f));
				upRenderer->MarkStatic(true);
			}
		}});
	}





	namespace {
		// Captures all transform fileds
		struct CapturedTransformState {
			Vector3 localPosition;
			Vector3 worldPosition;
			Vector3 localRotation;
			Vector3 worldRotation;
			Vector3 localScale;

			CapturedTransformState(Transform* transform)
				: localPosition(transform->LocalPosition()), worldPosition(transform->WorldPosition())
				, localRotation(transform->LocalEulerAngles()), worldRotation(transform->WorldEulerAngles())
				, localScale(transform->LocalScale()) {}
		};

		// Updates transform component each frame
		class TransformUpdater : public virtual Scene::LogicContext::UpdatingComponent {
		private:
			Jimara::Test::TestEnvironment* const m_environment;
			const Function<bool, const CapturedTransformState&, float, Jimara::Test::TestEnvironment*, Transform*> m_updateTransform;
			const CapturedTransformState m_initialTransform;
			const Stopwatch m_stopwatch;

		public:
			inline TransformUpdater(Component* parent, const std::string& name, Jimara::Test::TestEnvironment* environment
				, Function<bool, const CapturedTransformState&, float, Jimara::Test::TestEnvironment*, Transform*> updateTransform)
				: Component(parent, name), m_environment(environment), m_updateTransform(updateTransform)
				, m_initialTransform(parent->GetTransfrom()) {
			}

			inline virtual void Update()override {
				if (!m_updateTransform(m_initialTransform, m_stopwatch.Elapsed(), m_environment, GetTransfrom()))
					GetTransfrom()->Destroy();
			}
		};

		// Moves objects "in orbit" around some point
		bool Swirl(const CapturedTransformState& initialState, float totalTime, Jimara::Test::TestEnvironment*, Transform* transform) {
			const float RADIUS = sqrt(Math::Dot(initialState.worldPosition, initialState.worldPosition));
			if (RADIUS <= 0.0f) return true;
			const Vector3 X = initialState.worldPosition / RADIUS;
			const Vector3 UP = Math::Normalize((Vector3(0.0f, 1.0f, 0.0f) - X * X.y));
			const Vector3 Y = Math::Cross(X, UP);

			auto getPosition = [&](float timePoint) -> Vector3 {
				const float RELATIVE_TIME = timePoint / RADIUS;
				return (X * (float)cos(RELATIVE_TIME) + Y * (float)sin(RELATIVE_TIME)) * RADIUS + Vector3(0.0f, 0.25f, 0.0f);
			};

			const float MOVE_TIME = totalTime * 2.0f;
			transform->SetWorldPosition(getPosition(MOVE_TIME));
			transform->LookAt(getPosition(MOVE_TIME + 0.1f));
			transform->SetLocalScale(
				Vector3((cos(totalTime + initialState.worldPosition.x + initialState.worldPosition.y + initialState.worldPosition.z) + 1.0f) * 0.15f + 0.15f));

			return true;
		};
	}





	// Creates a bounch of objects and moves them around using "Swirl"
	TEST(MeshRendererTest, MovingTransforms) {
#ifdef _WIN32
		Jimara::Test::Memory::MemorySnapshot snapshot;
		auto updateSnapshot = [&]() { snapshot = Jimara::Test::Memory::MemorySnapshot(); };
		auto compareSnapshot = [&]() -> bool { return snapshot.Compare(); };
#else
#ifndef NDEBUG
		size_t snapshot;
		auto updateSnapshot = [&](){ snapshot = Object::DEBUG_ActiveInstanceCount(); };
		auto compareSnapshot = [&]() -> bool { return snapshot == Object::DEBUG_ActiveInstanceCount(); };
#else
		auto updateSnapshot = [&](){};
		auto compareSnapshot = [&]() -> bool { return true; };
#endif 
#endif
		for (size_t i = 0; i < 2; i++) {
			updateSnapshot();
			std::stringstream stream;
			const bool instanced = (i == 1);
			stream << "Moving Transforms [Run " << i << " - " << (instanced ? "INSTANCED" : "NOT_INSTANCED") << "]";
			Jimara::Test::TestEnvironment environment(stream.str().c_str());
			
			environment.ExecuteOnUpdateNow([&]() {
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, 2.0f)), "Light", Vector3(2.0f, 0.25f, 0.25f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, -2.0f)), "Light", Vector3(0.25f, 2.0f, 0.25f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, 2.0f)), "Light", Vector3(0.25f, 0.25f, 2.0f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, -2.0f)), "Light", Vector3(2.0f, 4.0f, 1.0f));
				Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 2.0f, 0.0f)), "Light", Vector3(1.0f, 4.0f, 2.0f));
				});

			Reference<TriMesh> sphereMesh = GenerateMesh::Tri::Sphere(Vector3(0.0f, 0.0f, 0.0f), 0.075f, 16, 8);
			Reference<TriMesh> cubeMesh = GenerateMesh::Tri::Box(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));

			Reference<Material> material = [&]() -> Reference<Material> {
				Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true,
					Graphics::ImageTexture::AccessFlags::NONE);
				(*static_cast<uint32_t*>(texture->Map())) = 0xFFFFFFFF;
				texture->Unmap(true);
				return Jimara::SampleDiffuseShader::CreateMaterial(texture, environment.RootObject()->Context()->Graphics()->Device());
			}();

			std::uniform_real_distribution<float> disH(-1.5f, 1.5f);
			std::uniform_real_distribution<float> disV(0.0f, 2.0f);
			std::uniform_real_distribution<float> disAngle(-180.0f, 180.0f);

			for (size_t i = 0; i < 512; i++) environment.ExecuteOnUpdateNow([&]() {
				Transform* parent = Object::Instantiate<Transform>(environment.RootObject(), "Parent");
				parent->SetLocalPosition(Vector3(disH(Random::ThreadRNG()), disV(Random::ThreadRNG()), disH(Random::ThreadRNG())));
				parent->SetLocalEulerAngles(Vector3(disAngle(Random::ThreadRNG()), disAngle(Random::ThreadRNG()), disAngle(Random::ThreadRNG())));
				{
					Transform* ball = Object::Instantiate<Transform>(parent, "Ball");
					Object::Instantiate<MeshRenderer>(ball, "Sphere_Renderer", sphereMesh, material, instanced);
				}
				{
					Transform* tail = Object::Instantiate<Transform>(parent, "Ball");
					tail->SetLocalPosition(Vector3(0.0f, 0.05f, -0.5f));
					tail->SetLocalScale(Vector3(0.025f, 0.025f, 0.5f));
					Object::Instantiate<MeshRenderer>(tail, "Tail_Renderer", cubeMesh, material, instanced);
				}
				Object::Instantiate<TransformUpdater>(parent, "Updater", &environment, Swirl);
				});
		}
		EXPECT_TRUE(compareSnapshot());
	}





	// Creates geometry, applies "Swirl" movement to them and marks some of the renderers static to let us make sure, the rendered positions are not needlessly updated
	TEST(MeshRendererTest, StaticTransforms) {
		Jimara::Test::TestEnvironment environment("Static transforms (Tailless balls will be locked in place, even though their transforms are alted as well, moving only with camera)");
		
		environment.ExecuteOnUpdateNow([&]() {
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, 2.0f)), "Light", Vector3(2.0f, 0.25f, 0.25f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 0.25f, -2.0f)), "Light", Vector3(0.25f, 2.0f, 0.25f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, 2.0f)), "Light", Vector3(0.25f, 0.25f, 2.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-2.0f, 0.25f, -2.0f)), "Light", Vector3(2.0f, 4.0f, 1.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 2.0f, 0.0f)), "Light", Vector3(1.0f, 4.0f, 2.0f));
			});

		Reference<TriMesh> sphereMesh = GenerateMesh::Tri::Sphere(Vector3(0.0f, 0.0f, 0.0f), 0.075f, 16, 8);
		Reference<TriMesh> cubeMesh = GenerateMesh::Tri::Box(Vector3(-1.0f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));

		Reference<Material> material = [&]() -> Reference<Material> {
			Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true,
				Graphics::ImageTexture::AccessFlags::NONE);
			(*static_cast<uint32_t*>(texture->Map())) = 0xFFAAAAAA;
			texture->Unmap(true);
			return SampleDiffuseShader::CreateMaterial(texture, environment.RootObject()->Context()->Graphics()->Device());
		}();

		std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

		environment.ExecuteOnUpdateNow([&]() {for (size_t i = 0; i < 128; i++) {
			Transform* parent = Object::Instantiate<Transform>(environment.RootObject(), "Parent");
			parent->SetLocalPosition(Vector3(dis(Random::ThreadRNG()), dis(Random::ThreadRNG()), dis(Random::ThreadRNG())));
			{
				Transform* ball = Object::Instantiate<Transform>(parent, "Ball");
				Object::Instantiate<MeshRenderer>(ball, "Sphere_Renderer", sphereMesh, material);
			}
			{
				Transform* tail = Object::Instantiate<Transform>(parent, "Ball");
				tail->SetLocalPosition(Vector3(0.0f, 0.05f, -0.5f));
				tail->SetLocalScale(Vector3(0.025f, 0.025f, 0.5f));
				Object::Instantiate<MeshRenderer>(tail, "Tail_Renderer", cubeMesh, material);
			}
			Object::Instantiate<TransformUpdater>(parent, "Updater", &environment, Swirl);
		}});
		environment.ExecuteOnUpdateNow([&]() {for (size_t i = 0; i < 128; i++) {
			Transform* parent = Object::Instantiate<Transform>(environment.RootObject(), "Parent");
			parent->SetLocalPosition(Vector3(dis(Random::ThreadRNG()), dis(Random::ThreadRNG()), dis(Random::ThreadRNG())));
			parent->SetLocalScale(Vector3(0.35f));
			{
				Transform* ball = Object::Instantiate<Transform>(parent, "Ball");
				Object::Instantiate<MeshRenderer>(ball, "Sphere_Renderer", sphereMesh, material)->MarkStatic(true);
			}
			Object::Instantiate<TransformUpdater>(parent, "Updater", &environment, Swirl);
		}});
	}





	namespace {
		// Deforms a planar mesh each frame, generating "moving waves"
		class MeshDeformer : public virtual Scene::LogicContext::UpdatingComponent {
		private:
			Jimara::Test::TestEnvironment* const m_environment;
			const Reference<TriMesh> m_mesh;
			const Stopwatch m_stopwatch;

		public:
			inline MeshDeformer(Component* parent, const std::string& name, Jimara::Test::TestEnvironment* env, TriMesh* mesh)
				: Component(parent, name), m_environment(env), m_mesh(mesh) {}

			inline virtual void Update() override {
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





	// Creates a planar mesh and applies per-frame deformation
	TEST(MeshRendererTest, MeshDeformation) {
		Jimara::Test::TestEnvironment environment("Mesh Deformation");
		
		environment.ExecuteOnUpdateNow([&]() {
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 1.0f, 0.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
			});

		Reference<TriMesh> planeMesh = GenerateMesh::Tri::Plane(Vector3(0.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 2.0f), Size2(100, 100));
		environment.ExecuteOnUpdateNow([&]() {
			Reference<Material> material = [&]() -> Reference<Material> {
				Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true,
					Graphics::ImageTexture::AccessFlags::NONE);
				(*static_cast<uint32_t*>(texture->Map())) = 0xFFFFFFFF;
				texture->Unmap(true);
				return SampleDiffuseShader::CreateMaterial(texture, environment.RootObject()->Context()->Graphics()->Device());
			}();

			Object::Instantiate<MeshRenderer>(Object::Instantiate<Transform>(environment.RootObject(), "Transform"), "MeshRenderer", planeMesh, material)->MarkStatic(true);
			Object::Instantiate<MeshDeformer>(environment.RootObject(), "Deformer", &environment, planeMesh);
			});
	}





	// Creates a planar mesh, applies per-frame deformation and moves the thing around
	TEST(MeshRendererTest, MeshDeformationAndTransform) {
		Jimara::Test::TestEnvironment environment("Mesh Deformation And Transform");
		
		environment.ExecuteOnUpdateNow([&]() {
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 1.0f, 0.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
			});

		Reference<TriMesh> planeMesh = GenerateMesh::Tri::Plane(Vector3(0.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 2.0f), Size2(100, 100));
		environment.ExecuteOnUpdateNow([&]() {
			Object::Instantiate<MeshDeformer>(environment.RootObject(), "Deformer", &environment, planeMesh);
			});

		Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Transform");
		environment.ExecuteOnUpdateNow([&]() {
			Reference<Material> material = [&]() -> Reference<Material> {
				Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true,
					Graphics::ImageTexture::AccessFlags::NONE);
				(*static_cast<uint32_t*>(texture->Map())) = 0xFFFFFFFF;
				texture->Unmap(true);
				return SampleDiffuseShader::CreateMaterial(texture, environment.RootObject()->Context()->Graphics()->Device());
			}();

			Object::Instantiate<MeshRenderer>(transform, "MeshRenderer", planeMesh, material);
			});

		auto move = [](const CapturedTransformState&, float totalTime, Jimara::Test::TestEnvironment*, Transform* transform) -> bool {
			transform->SetLocalPosition(Vector3(cos(totalTime), 0.0f, sin(totalTime)));
			transform->SetLocalScale(Vector3((cos(totalTime * 0.5f) + 1.0f) * 0.5f + 0.15f));
			return true;
		};

		environment.ExecuteOnUpdateNow([&]() {
			Object::Instantiate<TransformUpdater>(transform, "TransformUpdater", &environment,
				Function<bool, const CapturedTransformState&, float, Jimara::Test::TestEnvironment*, Transform*>(move));
			});
	}





	namespace {
		// Generates texture contents each frame
		class TextureGenerator : public virtual Scene::LogicContext::UpdatingComponent {
		private:
			Jimara::Test::TestEnvironment* const m_environment;
			const Reference<Graphics::ImageTexture> m_texture;
			const Stopwatch m_stopwatch;

		public:
			inline TextureGenerator(Component* parent, const std::string& name, Jimara::Test::TestEnvironment* env, Graphics::ImageTexture* texture)
				: Component(parent, name), m_environment(env), m_texture(texture) {
			}

			inline virtual void Update() override {
				float time = m_stopwatch.Elapsed();
				const Size3 TEXTURE_SIZE = m_texture->Size();
				uint32_t* data = static_cast<uint32_t*>(m_texture->Map());
				uint32_t timeOffsetX = (static_cast<uint32_t>(time * 16));
				uint32_t timeOffsetY = (static_cast<uint32_t>(time * 48));
				uint32_t timeOffsetZ = (static_cast<uint32_t>(time * 32));
				for (uint32_t y = 0; y < TEXTURE_SIZE.y; y++) {
					for (uint32_t x = 0; x < TEXTURE_SIZE.x; x++) {
						uint8_t red = static_cast<uint8_t>(x + timeOffsetX);
						uint8_t green = static_cast<uint8_t>(y - timeOffsetY);
						uint8_t blue = static_cast<uint8_t>((x + timeOffsetZ) ^ y);
						uint8_t alpha = static_cast<uint8_t>(255u);
						data[x] = (red << 24) + (green << 16) + (blue << 8) + alpha;
					}
					data += TEXTURE_SIZE.x;
				}
				m_texture->Unmap(true);
			}
		};
	}





	// Creates a planar mesh and applies a texture that changes each frame
	TEST(MeshRendererTest, DynamicTexture) {
		Jimara::Test::TestEnvironment environment("Dynamic Texture");
		
		environment.ExecuteOnUpdateNow([&]() {
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 1.0f, 0.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
			});

		Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
			Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(128, 128, 1), 1, true,
			Graphics::ImageTexture::AccessFlags::NONE);
		environment.ExecuteOnUpdateNow([&]() {
			texture->Map();
			texture->Unmap(true);
			Object::Instantiate<TextureGenerator>(environment.RootObject(), "TextureGenerator", &environment, texture);
			});

		environment.ExecuteOnUpdateNow([&]() {
			Reference<TriMesh> planeMesh = GenerateMesh::Tri::Plane(Vector3(0.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 2.0f));
			Reference<Material> material = SampleDiffuseShader::CreateMaterial(texture, environment.RootObject()->Context()->Graphics()->Device());
			Object::Instantiate<MeshRenderer>(Object::Instantiate<Transform>(environment.RootObject(), "Transform"), "MeshRenderer", planeMesh, material);
			});
	}





	// Creates a planar mesh, applies per-frame deformation, a texture that changes each frame and moves the thing around
	TEST(MeshRendererTest, DynamicTextureWithMovementAndDeformation) {
		Jimara::Test::TestEnvironment environment("Dynamic Texture With Movement And Mesh Deformation");
		
		environment.ExecuteOnUpdateNow([&]() {
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, 1.0f, 0.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
			});

		Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
			Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(128, 128, 1), 1, true,
			Graphics::ImageTexture::AccessFlags::NONE);
		environment.ExecuteOnUpdateNow([&]() {
			texture->Map();
			texture->Unmap(true);
			Object::Instantiate<TextureGenerator>(environment.RootObject(), "TextureGenerator", &environment, texture);
			});

		Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Transform");

		Reference<TriMesh> planeMesh = GenerateMesh::Tri::Plane(Vector3(0.0f, 0.0f, 0.0f), Vector3(2.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 2.0f), Size2(100, 100));
		environment.ExecuteOnUpdateNow([&]() {
			Reference<Material> material = SampleDiffuseShader::CreateMaterial(texture, environment.RootObject()->Context()->Graphics()->Device());
			Object::Instantiate<MeshRenderer>(transform, "MeshRenderer", planeMesh, material);
			Object::Instantiate<MeshDeformer>(environment.RootObject(), "Deformer", &environment, planeMesh);
			});

		auto move = [](const CapturedTransformState&, float totalTime, Jimara::Test::TestEnvironment*, Transform* transform) -> bool {
			transform->SetLocalPosition(Vector3(cos(totalTime), 0.0f, sin(totalTime)));
			return true;
		};

		environment.ExecuteOnUpdateNow([&]() {
			Object::Instantiate<TransformUpdater>(transform, "TransformUpdater", &environment,
				Function<bool, const CapturedTransformState&, float, Jimara::Test::TestEnvironment*, Transform*>(move));
			});
	}





	// Loads sample scene from .obj file
	TEST(MeshRendererTest, LoadedGeometry) {
		Jimara::Test::TestEnvironment environment("Loading Geometry...");
		
		{
			static auto baseMove = [](const CapturedTransformState&, float totalTime, Jimara::Test::TestEnvironment*, Transform* transform) -> bool {
				transform->SetLocalPosition(Vector3(cos(totalTime) * 4.0f, 1.0f, sin(totalTime) * 4.0f));
				return true;
			};
			static const float rotationSpeed = -1.25f;
			environment.ExecuteOnUpdateNow([&]() {
				auto move = [](const CapturedTransformState& state, float totalTime, Jimara::Test::TestEnvironment* env, Transform* transform) -> bool {
					transform->GetComponentInChildren<PointLight>()->SetColor(Vector3((sin(totalTime * 4.0f) + 1.0f) * 4.0f, cos(totalTime * 2.0f) + 1.0f, 2.0f));
					return baseMove(state, totalTime * rotationSpeed, env, transform);
				};
				Object::Instantiate<TransformUpdater>(
					Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(4.0f, 1.0f, 4.0f)), "Light", Vector3(8.0f, 2.0f, 2.0f)),
					"TransformUpdater", &environment, Function<bool, const CapturedTransformState&, float, Jimara::Test::TestEnvironment*, Transform*>(move));
				});
			environment.ExecuteOnUpdateNow([&]() {
				auto move = [](const CapturedTransformState& state, float totalTime, Jimara::Test::TestEnvironment* env, Transform* transform) -> bool {
					transform->GetComponentInChildren<PointLight>()->SetColor(Vector3(2.0f, (sin(totalTime * 2.0f) + 1.0f) * 4.0f, (cos(totalTime * 4.0f) + 1.0f) * 2.0f));
					return baseMove(state, totalTime * rotationSpeed + Math::Radians(90.0f), env, transform);
				};
				Object::Instantiate<TransformUpdater>(
					Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-4.0f, 1.0f, -4.0f)), "Light", Vector3(2.0f, 8.0f, 2.0f)),
					"TransformUpdater", &environment, Function<bool, const CapturedTransformState&, float, Jimara::Test::TestEnvironment*, Transform*>(move));
				});
			environment.ExecuteOnUpdateNow([&]() {
				auto move = [](const CapturedTransformState& state, float totalTime, Jimara::Test::TestEnvironment* env, Transform* transform) -> bool {
					transform->GetComponentInChildren<PointLight>()->SetColor(Vector3((cos(totalTime * 3.0f) + 1.0f) * 1.0f, 2.0f, (sin(totalTime * 2.5f) + 1.0f) * 4.0f));
					return baseMove(state, totalTime * rotationSpeed + Math::Radians(180.0f), env, transform);
				};
				Object::Instantiate<TransformUpdater>(
					Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(4.0f, 1.0f, -4.0f)), "Light", Vector3(2.0f, 2.0f, 8.0f)),
					"TransformUpdater", &environment, Function<bool, const CapturedTransformState&, float, Jimara::Test::TestEnvironment*, Transform*>(move));
				});
			environment.ExecuteOnUpdateNow([&]() {
				auto move = [](const CapturedTransformState& state, float totalTime, Jimara::Test::TestEnvironment* env, Transform* transform) -> bool {
					transform->GetComponentInChildren<PointLight>()->SetColor(Vector3((sin(totalTime * 4.25f) + 1.0f) * 4.0f, 2.0f, (cos(totalTime * 7.5f) + 1.0f) * 4.0f));
					return baseMove(state, totalTime * rotationSpeed + Math::Radians(270.0f), env, transform);
				};
				Object::Instantiate<TransformUpdater>(
					Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-4.0f, 1.0f, 4.0f)), "Light", Vector3(4.0f, 2.0f, 4.0f)),
					"TransformUpdater", &environment, Function<bool, const CapturedTransformState&, float, Jimara::Test::TestEnvironment*, Transform*>(move));
				});
			environment.ExecuteOnUpdateNow([&]() {
				Object::Instantiate<DirectionalLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(0.0f, -2.0f, 0.0f)), "Light", Vector3(1.5f, 0.0f, 0.0f))
					->GetTransfrom()->LookAt(Vector3(0.0f, 0.0f, 0.0f));
				Object::Instantiate<DirectionalLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(2.0f, 2.0f, 2.0f)), "Light", Vector3(0.0f, 0.125f, 0.125f))
					->GetTransfrom()->LookAt(Vector3(0.0f, 0.0f, 0.0f));
				});
		}

		Reference<Graphics::ImageTexture> whiteTexture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
			Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true,
			Graphics::ImageTexture::AccessFlags::NONE);
		{
			(*static_cast<uint32_t*>(whiteTexture->Map())) = 0xFFFFFFFF;
			whiteTexture->Unmap(true);
		}
		Reference<Material> whiteMaterial = SampleDiffuseShader::CreateMaterial(whiteTexture, environment.RootObject()->Context()->Graphics()->Device());

		std::vector<Reference<TriMesh>> geometry = TriMeshesFromOBJ("Assets/Meshes/OBJ/Bear/ursus_proximus.obj");
		std::vector<MeshRenderer*> renderers;

		environment.ExecuteOnUpdateNow([&]() {
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Transform");
			transform->SetLocalPosition(Vector3(0.0f, -0.5f, 0.0f));
			transform->SetLocalScale(Vector3(0.75f));
			for (size_t i = 0; i < geometry.size(); i++) {
				TriMesh* mesh = geometry[i];
				renderers.push_back(Object::Instantiate<MeshRenderer>(transform, TriMesh::Reader(mesh).Name(), mesh, whiteMaterial));
			}
			environment.SetWindowName("Loading texture...");
			});

		Reference<Graphics::ImageTexture> bearTexture = Graphics::ImageTexture::LoadFromFile(
			environment.RootObject()->Context()->Graphics()->Device(), "Assets/Meshes/OBJ/Bear/bear_diffuse.png", true);
		Reference<Material> bearMaterial = SampleDiffuseShader::CreateMaterial(bearTexture, environment.RootObject()->Context()->Graphics()->Device());
		environment.SetWindowName("Applying texture...");

		for (size_t i = 0; i < geometry.size(); i++)
			if (TriMesh::Reader(geometry[i]).Name() == "bear")
				renderers[i]->SetMaterial(bearMaterial);

		environment.SetWindowName("Loaded scene");
	}
}
