#include "../../GtestHeaders.h"
#include "../../Memory.h"
#include "../Shaders/SampleDiffuseShader.h"
#include "../TestEnvironment/TestEnvironment.h"
#include "Data/Generators/MeshGenerator.h"
#include "Components/Lights/PointLight.h"
#include "Components/Lights/DirectionalLight.h"
#include "Components/GraphicsObjects/SkinnedMeshRenderer.h"



namespace Jimara {
	TEST(SkinnedMeshRendererTest, AxisTest) {
		Jimara::Test::TestEnvironment environment("AxisTest <X-red, Y-green, Z-blue>", 45);

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
	}
}
