#include "../GtestHeaders.h"
#include "../Components/TestEnvironment/TestEnvironment.h"
#include "../Components/Shaders/SampleDiffuseShader.h"
#include "OS/IO/MMappedFile.h"
#include "OS/Logging/StreamLogger.h"
#include "Data/Formats/FBX/FBXData.h"
#include "Components/MeshRenderer.h"
#include "Components/Lights/DirectionalLight.h"
#include <sstream>

namespace Jimara {
	namespace {
		inline static Reference<Material> CreateMaterial(Component* rootObject, uint32_t color) {
			Reference<Graphics::ImageTexture> texture = rootObject->Context()->Graphics()->Device()->CreateTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
			(*static_cast<uint32_t*>(texture->Map())) = color;
			texture->Unmap(true);
			return Jimara::Test::SampleDiffuseShader::CreateMaterial(texture);
		};

		inline static void RenderFBXMeshesOnTestEnvironment(const FBXData* data, const std::string_view& testName) {
			Jimara::Test::TestEnvironment environment(testName);
			environment.ExecuteOnUpdateNow([&]() {
				Reference<Transform> sun = Object::Instantiate<Transform>(environment.RootObject(), "Sun", Vector3(0.0f), Vector3(64.0f, 32.0f, 0.0f));
				Object::Instantiate<DirectionalLight>(sun, "Sun Light", Vector3(0.85f, 0.85f, 0.856f));
				Reference<Transform> back = Object::Instantiate<Transform>(environment.RootObject(), "Sun");
				back->LookTowards(-sun->Forward());
				Object::Instantiate<DirectionalLight>(back, "Back Light", Vector3(0.125f, 0.125f, 0.125f));
				});
			for (size_t i = 0; i < data->MeshCount(); i++) environment.ExecuteOnUpdateNow([&]() {
				Reference<Material> material = CreateMaterial(environment.RootObject(), 0xFFFFFFFF);
				Reference<TriMesh> mesh = ToTriMesh(data->GetMesh(i).mesh);
				Reference<Transform> parent = Object::Instantiate<Transform>(environment.RootObject(), TriMesh::Reader(mesh).Name());
				Object::Instantiate<MeshRenderer>(parent, "Renderer", mesh, material);
				});
		}
	}

	TEST(FBXTest, Playground) {
		Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
		//Reference<OS::MMappedFile> fileMapping = OS::MMappedFile::Create("C:/Users/Donsky/Desktop/DefaultGuy_0.fbx", logger);
		Reference<OS::MMappedFile> fileMapping = OS::MMappedFile::Create("Assets/Meshes/FBX/Cube.fbx", logger);
		ASSERT_NE(fileMapping, nullptr);
		MemoryBlock block = *fileMapping;

		Reference<FBXContent> content = FBXContent::Decode(block, logger);
		ASSERT_NE(content, nullptr);
		logger->Info(*content);

		Reference<FBXData> data = FBXData::Extract(content, logger);
		ASSERT_NE(data, nullptr);

		ASSERT_EQ(data->MeshCount(), 1);
		Reference<PolyMesh> polyMesh = data->GetMesh(0).mesh;
		ASSERT_NE(polyMesh, nullptr);
		{
			PolyMesh::Reader reader(polyMesh);
			EXPECT_EQ(reader.Name(), "Cube.001");
			EXPECT_EQ(reader.VertCount(), 24);
			EXPECT_EQ(reader.FaceCount(), 6);
			for (size_t i = 0; i < reader.FaceCount(); i++)
				EXPECT_EQ(reader.Face(i).Size(), 4);
		}

		RenderFBXMeshesOnTestEnvironment(data, "Playground");
	}
}
