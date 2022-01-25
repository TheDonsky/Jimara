#include "../../GtestHeaders.h"
#include "../../Memory.h"
#include "../TestEnvironment/TestEnvironment.h"
#include "Data/Materials/SampleDiffuse/SampleDiffuseShader.h"
#include "Data/Generators/MeshGenerator.h"
#include "Components/Lights/PointLight.h"
#include "Components/Lights/DirectionalLight.h"
#include "Components/GraphicsObjects/MeshRenderer.h"
#include "Components/GraphicsObjects/SkinnedMeshRenderer.h"



namespace Jimara {
	TEST(SkinnedMeshRendererTest, Playground) {
		Jimara::Test::TestEnvironment environment("Playground", 10);

		environment.ExecuteOnUpdateNow([&]() {
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(1.0f, 1.0f, 1.0f)), "Light", Vector3(2.5f, 2.5f, 2.5f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-1.0f, 1.0f, 1.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(1.0f, 1.0f, -1.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
			Object::Instantiate<PointLight>(Object::Instantiate<Transform>(environment.RootObject(), "PointLight", Vector3(-1.0f, 1.0f, -1.0f)), "Light", Vector3(1.0f, 1.0f, 1.0f));
			Object::Instantiate<DirectionalLight>(Object::Instantiate<Transform>(environment.RootObject(), "Sun", Vector3(0.0f), Vector3(45.0f, 60.0f, 0.0f)), "Light", Vector3(0.25f, 0.25f, 0.25f));
			});

		auto createMaterial = [&](uint32_t color) -> Reference<Material> {
			Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
			(*static_cast<uint32_t*>(texture->Map())) = color;
			texture->Unmap(true);
			return SampleDiffuseShader::CreateMaterial(texture);
		};

		Reference<TriMesh> box = GenerateMesh::Tri::Box(Vector3(-0.5f, -0.5f, -0.5f), Vector3(0.5f, 0.5f, 0.5f));

		environment.ExecuteOnUpdateNow([&]() {
			Transform* skeletonRoot = Object::Instantiate<Transform>(environment.RootObject(), "SkeletonRoot");
			Transform* headBone = Object::Instantiate<Transform>(skeletonRoot, "HeadBone", Math::Up() * 2.0f);
			Reference<SkinnedTriMesh> capsule = ToSkinnedTriMesh(GenerateMesh::Tri::Capsule(Math::Up(), 0.25f, 1.5f, 32, 16, 32));
			{
				SkinnedTriMesh::Writer writer(capsule);
				writer.AddBone(skeletonRoot->LocalMatrix());
				writer.AddBone(headBone->WorldMatrix());
				for (uint32_t vertId = 0; vertId < writer.VertCount(); vertId++) {
					const float h = writer.Vert(vertId).position.y * 0.5f;
					writer.Weight(vertId, 0) = 1.0f - h;
					writer.Weight(vertId, 1) = h;
				}
			}
			headBone->SetLocalEulerAngles(Vector3(20.0f, 0.0f, 0.0f));

			Reference<Material> material = createMaterial(0xFFFFFFFF);
			const Transform* bones[2] = { skeletonRoot, headBone };

			for (size_t count = 5; count <= 100; count += 5) {
				const float RADIUS = (static_cast<float>(count) / 3.0f);
				const float STEP = (360.0f / static_cast<float>(count));
				const float BASE_ANGLE = (STEP / static_cast<float>(count));
				for (size_t i = 0; i < count; i++) {
					Reference<Transform> transform = Object::Instantiate<Transform>(
						environment.RootObject(), "", Vector3(0.0f), Vector3(0.0f, BASE_ANGLE + (static_cast<float>(i) * STEP), 0.0f));
					transform->SetLocalPosition(transform->Forward() * RADIUS);
					Object::Instantiate<SkinnedMeshRenderer>(transform, "", capsule, material, true, false, bones, 2, skeletonRoot);
				}
			}

			static Stopwatch stopwatch;
			stopwatch.Reset();
			void(*updateHeadBone)(Transform*) = [](Transform* headBone) {
				float time = stopwatch.Elapsed();
				headBone->SetLocalEulerAngles(Vector3(Math::Degrees(cos(time) * Math::Pi() * 0.5f), 0.0f, 0.0f));
				headBone->SetLocalPosition(Math::Up() * 1.0f + headBone->LocalUp());
			};
			environment.RootObject()->Context()->Graphics()->OnGraphicsSynch() += Callback<>(updateHeadBone, headBone);
			});
	}
}
