#include "../GtestHeaders.h"
#include "../Components/TestEnvironment/TestEnvironment.h"
#include "../Components/Shaders/SampleDiffuseShader.h"
#include "OS/IO/MMappedFile.h"
#include "OS/Logging/StreamLogger.h"
#include "Data/Formats/FBX/FBXData.h"
#include "Components/MeshRenderer.h"
#include "Components/Lights/DirectionalLight.h"
#include <sstream>
#include <cassert>

namespace Jimara {
	namespace {
		inline static Reference<Material> CreateMaterial(Component* rootObject, uint32_t color) {
			Reference<Graphics::ImageTexture> texture = rootObject->Context()->Graphics()->Device()->CreateTexture(
				Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_UNORM, Size3(1, 1, 1), 1, true);
			(*static_cast<uint32_t*>(texture->Map())) = color;
			texture->Unmap(true);
			return Jimara::Test::SampleDiffuseShader::CreateMaterial(texture);
		};

		inline static Reference<Material> CreateMaterial(Component* rootObject, const std::string_view& texturePath) {
			Reference<Graphics::ImageTexture> texture = Graphics::ImageTexture::LoadFromFile(rootObject->Context()->Graphics()->Device(), texturePath, true);
			assert(texture != nullptr);
			return Jimara::Test::SampleDiffuseShader::CreateMaterial(texture);
		}

		typedef std::unordered_map<std::string_view, std::string_view> TexturePathByMeshName;
		inline static void RenderFBXMeshesOnTestEnvironment(const FBXData* data, const std::string_view& testName, 
			const TexturePathByMeshName& meshTextures = TexturePathByMeshName(), float windowTimeout = 5.0f) {
			Jimara::Test::TestEnvironment environment(testName, windowTimeout);
			environment.ExecuteOnUpdateNow([&]() {
				Reference<Transform> sun = Object::Instantiate<Transform>(environment.RootObject(), "Sun", Vector3(1.0f, 1.0f, 1.0f));
				sun->LookAt(Vector3(0.0f));
				Object::Instantiate<DirectionalLight>(sun, "Sun Light", Vector3(0.85f, 0.85f, 0.856f));
				Reference<Transform> back = Object::Instantiate<Transform>(environment.RootObject(), "Sun");
				back->LookTowards(-sun->Forward());
				Object::Instantiate<DirectionalLight>(back, "Back Light", Vector3(0.125f, 0.125f, 0.125f));
				});
			for (size_t i = 0; i < data->MeshCount(); i++) environment.ExecuteOnUpdateNow([&]() {
				Reference<TriMesh> mesh = ToTriMesh(data->GetMesh(i).mesh);
				TexturePathByMeshName::const_iterator it = meshTextures.find(TriMesh::Reader(mesh).Name());
				Reference<Material> material = (it == meshTextures.end()) ? CreateMaterial(environment.RootObject(), 0xFFFFFFFF) : CreateMaterial(environment.RootObject(), it->second);
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

	TEST(FBXTest, Axis) {
		Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
		const char AXIS[] = { 'X', 'Y', 'Z' };
		const size_t AXIS_COUNT = sizeof(AXIS) / sizeof(char);
		const char SIGNS[] = { '-', '+' };
		const size_t SIGN_COUNT = sizeof(SIGNS) / sizeof(char);

		const char* MESH_NAMES[] = { "X_Mesh", "Y_Mesh", "Z_Mesh" };
		const size_t MESH_NAME_COUNT = sizeof(MESH_NAMES) / sizeof(char*);
		const TexturePathByMeshName TEXTURE_PATH_BY_MESH_NAME = [&]() -> TexturePathByMeshName {
			const std::string_view TEXTURE_PATH = "Assets/Meshes/FBX/XYZ/XYZ.png";
			TexturePathByMeshName paths;
			for (size_t i = 0; i < MESH_NAME_COUNT; i++)
				paths[MESH_NAMES[i]] = TEXTURE_PATH;
			return paths;
		}();

		for (size_t forwardAxis = 0; forwardAxis < AXIS_COUNT; forwardAxis++)
			for (size_t upAxis = 0; upAxis < AXIS_COUNT; upAxis++)
				if (forwardAxis != upAxis) for (size_t forwardSign = 0; forwardSign < SIGN_COUNT; forwardSign++)
					for (size_t upSign = 0; upSign < SIGN_COUNT; upSign++) {
						const std::string filePath = [&]() -> std::string {
							std::stringstream stream;
							stream << "Assets/Meshes/FBX/XYZ/XYZ_Forward(" << SIGNS[forwardSign] << AXIS[forwardAxis] << ")_Up(" << SIGNS[upSign] << AXIS[upAxis] << ").fbx";
							return stream.str();
						}();
						Reference<OS::MMappedFile> fileMapping = OS::MMappedFile::Create(filePath, logger);
						ASSERT_NE(fileMapping, nullptr);

						Reference<FBXContent> content = FBXContent::Decode(*fileMapping, logger);
						ASSERT_NE(content, nullptr);
						//logger->Info(*content);

						logger->Info(filePath);
						Reference<FBXData> data = FBXData::Extract(content, logger);
						ASSERT_NE(data, nullptr);

						ASSERT_EQ(data->MeshCount(), 3);
						bool MESH_PRESENT[MESH_NAME_COUNT];
						for (size_t nameId = 0; nameId < MESH_NAME_COUNT; nameId++)
							MESH_PRESENT[nameId] = false;
						for (size_t meshId = 0; meshId < data->MeshCount(); meshId++) {
							const std::string name = PolyMesh::Reader(data->GetMesh(meshId).mesh).Name();
							for (size_t nameId = 0; nameId < MESH_NAME_COUNT; nameId++)
								if (name == MESH_NAMES[nameId])
									MESH_PRESENT[nameId] = true;
						}
						for (size_t nameId = 0; nameId < MESH_NAME_COUNT; nameId++)
							EXPECT_TRUE(MESH_PRESENT[nameId]);
						
						//RenderFBXMeshesOnTestEnvironment(data, filePath, TEXTURE_PATH_BY_MESH_NAME, 1.0f);
					}
	}
}
