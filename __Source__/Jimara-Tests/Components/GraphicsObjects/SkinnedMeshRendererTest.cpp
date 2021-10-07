#include "../../GtestHeaders.h"
#include "../../Memory.h"
#include "../Shaders/SampleDiffuseShader.h"
#include "../TestEnvironment/TestEnvironment.h"
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
			});

		auto createMaterial = [&](uint32_t color) -> Reference<Material> {
			Reference<Graphics::ImageTexture> texture = environment.RootObject()->Context()->Graphics()->Device()->CreateTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
			(*static_cast<uint32_t*>(texture->Map())) = color;
			texture->Unmap(true);
			return Jimara::Test::SampleDiffuseShader::CreateMaterial(texture);
		};

		Reference<TriMesh> box = GenerateMesh::Tri::Box(Vector3(-0.5f, -0.5f, -0.5f), Vector3(0.5f, 0.5f, 0.5f));

		environment.ExecuteOnUpdateNow([&]() {
			Transform* skeletonRoot = Object::Instantiate<Transform>(environment.RootObject(), "SkeletonRoot");
			Transform* headBone = Object::Instantiate<Transform>(skeletonRoot, "HeadBone", Math::Up() * 2.0f);
			Reference<SkinnedTriMesh> capsule = ToSkinnedTriMesh(GenerateMesh::Tri::Capsule(Math::Up(), 0.25f, 1.5f, 32, 8, 8));
			{
				SkinnedTriMesh::Writer writer(capsule);
				writer.AddBone(skeletonRoot->LocalMatrix());
				writer.AddBone(headBone->WorldMatrix());
				for (uint32_t vertId = 0; vertId < writer.VertCount(); vertId++) {
					const float h = writer.Vert(vertId).position.y * writer.Vert(vertId).position.y * 0.25f;
					writer.Weight(vertId, 0) = 1.0f - h;
					writer.Weight(vertId, 1) = h;
				}
			}
			headBone->SetLocalEulerAngles(Vector3(20.0f, 0.0f, 0.0f));

			Reference<Material> material = createMaterial(0xFFFFFFFF);
			const Transform* bones[2] = { skeletonRoot, headBone };

			Object::Instantiate<SkinnedMeshRenderer>(
				Object::Instantiate<Transform>(skeletonRoot, "", Vector3(0.0f), Vector3(0.0f), Vector3(0.25f))
				, "RootRenderer", box, material, true);
			Object::Instantiate<SkinnedMeshRenderer>(
				Object::Instantiate<Transform>(headBone, "", Vector3(0.0f), Vector3(0.0f), Vector3(0.25f))
				, "BoneRenderer", box, material, true);

			Object::Instantiate<SkinnedMeshRenderer>(
				skeletonRoot, "SkinnedRenderer", capsule, material, false, false, skeletonRoot, bones, 2);

			Object::Instantiate<SkinnedMeshRenderer>(
				Object::Instantiate<Transform>(environment.RootObject(), "", Vector3(1.0f), Vector3(90.0f, 0.0f, 0.0f))
				, "", capsule, material, true, false, skeletonRoot, bones, 2);

			Object::Instantiate<SkinnedMeshRenderer>(
				Object::Instantiate<Transform>(environment.RootObject(), "", Vector3(-1.0f), Vector3(0.0f, 90.0f, 0.0f))
				, "", capsule, material, true, false, skeletonRoot, bones, 2);

			static Stopwatch stopwatch;
			stopwatch.Reset();
			void(*updateHeadBone)(Transform*) = [](Transform* headBone) {
				float time = stopwatch.Elapsed();
				headBone->SetLocalEulerAngles(Vector3(Math::Degrees(cos(time) * Math::Pi() * 0.5f), 0.0f, 0.0f));
			};
			environment.RootObject()->Context()->Graphics()->OnPostGraphicsSynch() += Callback<>(updateHeadBone, headBone);
			});

		//*

		{
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Center");
			Reference<Material> material = createMaterial(0xFF888888);
			Reference<TriMesh> sphere = GenerateMesh::Tri::Sphere(Vector3(0.0f, 0.0f, 0.0f), 0.1f, 32, 16);
			Object::Instantiate<SkinnedMeshRenderer>(transform, "Center_Renderer", sphere, material);
		}
		{
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "X");
			transform->SetLocalPosition(Vector3(0.5f, 0.0f, 0.0f));
			transform->SetLocalScale(Vector3(1.0f, 0.075f, 0.075f));
			Reference<Material> material = createMaterial(0xFF0000FF);
			Object::Instantiate<SkinnedMeshRenderer>(transform, "X_Renderer", box, material);
		}
		{
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Y");
			transform->SetLocalPosition(Vector3(0.0f, 0.5f, 0.0f));
			transform->SetLocalScale(Vector3(0.075f, 1.0f, 0.075f));
			Reference<Material> material = createMaterial(0xFF00FF00);
			Object::Instantiate<SkinnedMeshRenderer>(transform, "Y_Renderer", box, material);
		}
		{
			Transform* transform = Object::Instantiate<Transform>(environment.RootObject(), "Z");
			transform->SetLocalPosition(Vector3(0.0f, 0.0f, 0.5f));
			transform->SetLocalScale(Vector3(0.075f, 0.075f, 1.0f));
			Reference<Material> material = createMaterial(0xFFFF0000);
			Object::Instantiate<SkinnedMeshRenderer>(transform, "Z_Renderer", box, material);
		}
		//*/
	}
}
