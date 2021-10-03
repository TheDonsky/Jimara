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

		typedef Reference<Material>(*CreateMaterialFn)(Component*);
		inline static Reference<Material> CreateDefaultMaterial(Component* rootObject) { return CreateMaterial(rootObject, 0xFFFFFFFF); }

		typedef std::unordered_map<std::string_view, CreateMaterialFn> CreateMaterialByPath;

		inline static void RenderFBXDataOnTestEnvironment(const FBXData* data, const std::string_view& testName, 
			const CreateMaterialByPath& meshTextures = CreateMaterialByPath(), float windowTimeout = 5.0f) {
			Jimara::Test::TestEnvironment environment(testName, windowTimeout);
			environment.ExecuteOnUpdateNow([&]() {
				Reference<Transform> sun = Object::Instantiate<Transform>(environment.RootObject(), "Sun", Vector3(1.0f, 1.0f, 1.0f));
				sun->LookAt(Vector3(0.0f));
				Object::Instantiate<DirectionalLight>(sun, "Sun Light", Vector3(0.85f, 0.85f, 0.856f));
				Reference<Transform> back = Object::Instantiate<Transform>(environment.RootObject(), "Sun");
				back->LookTowards(-sun->Forward());
				Object::Instantiate<DirectionalLight>(back, "Back Light", Vector3(0.125f, 0.125f, 0.125f));
				});
			environment.ExecuteOnUpdateNow([&]() {
				typedef void(*CreateTransformMeshesFn)(const FBXData::FBXNode*, Component*, const FBXData*, std::string, const CreateMaterialByPath&, void*);
				CreateTransformMeshesFn createTransformMeshes = [](const FBXData::FBXNode* node, Component* parent, const FBXData* data, std::string path, const CreateMaterialByPath& textures, void* recurse) {
					path += node->name + "/";
					Reference<Transform> transform = Object::Instantiate<Transform>(parent, node->name, node->position, node->rotation, node->scale);
					for (size_t i = 0; i < node->meshIndices.Size(); i++) {
						Reference<TriMesh> mesh = ToTriMesh(data->GetMesh(node->meshIndices[i])->mesh);
						const std::string meshPath = path + TriMesh::Reader(mesh).Name();
						const CreateMaterialByPath::const_iterator it = textures.find(meshPath);
						Reference<Material> material = (it == textures.end()) ? CreateDefaultMaterial(parent) : it->second(parent);
						Object::Instantiate<MeshRenderer>(transform, TriMesh::Reader(mesh).Name(), mesh, material);
					}
					for (size_t i = 0; i < node->children.size(); i++)
						reinterpret_cast<CreateTransformMeshesFn>(recurse)(node->children[i], transform.operator->(), data, path, textures, recurse);
				};
				createTransformMeshes(data->RootNode(), environment.RootObject(), data, "", meshTextures, createTransformMeshes);
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
		Reference<const PolyMesh> polyMesh = data->GetMesh(0)->mesh;
		ASSERT_NE(polyMesh, nullptr);
		{
			PolyMesh::Reader reader(polyMesh);
			EXPECT_EQ(reader.Name(), "Cube.001");
			EXPECT_EQ(reader.VertCount(), 24);
			EXPECT_EQ(reader.FaceCount(), 6);
			for (size_t i = 0; i < reader.FaceCount(); i++)
				EXPECT_EQ(reader.Face(i).Size(), 4);
		}

		RenderFBXDataOnTestEnvironment(data, "Playground");
	}

	namespace {
		static const char* XYZ_TEXTURE_PATH() { return "Assets/Meshes/FBX/XYZ/XYZ.png"; }
		static const CreateMaterialByPath XYZ_MATERIALS_BY_PATH = {
			std::make_pair(std::string_view("/X_Transform/X_Mesh"), [](Component* rootObject) -> Reference<Material> { return CreateMaterial(rootObject, XYZ_TEXTURE_PATH()); }),
			std::make_pair(std::string_view("/Y_Transform/Y_Mesh"), [](Component* rootObject) -> Reference<Material> { return CreateMaterial(rootObject, XYZ_TEXTURE_PATH()); }),
			std::make_pair(std::string_view("/Z_Transform/Z_Mesh"), [](Component* rootObject) -> Reference<Material> { return CreateMaterial(rootObject, XYZ_TEXTURE_PATH()); }),
			std::make_pair(std::string_view("/DirectionThingie_X/DirectionThingie"), [](Component* rootObject) -> Reference<Material> { return CreateMaterial(rootObject, 0xFF0000FF); }),
			std::make_pair(std::string_view("/DirectionThingie_Y/DirectionThingie"), [](Component* rootObject) -> Reference<Material> { return CreateMaterial(rootObject, 0xFF00FF00); }),
			std::make_pair(std::string_view("/DirectionThingie_Z/DirectionThingie"), [](Component* rootObject) -> Reference<Material> { return CreateMaterial(rootObject, 0xFFFF0000); })
		};
	}

	TEST(FBXTest, Axis) {
		Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
		const char AXIS[] = { 'X', 'Y', 'Z' };
		const size_t AXIS_COUNT = sizeof(AXIS) / sizeof(char);
		const Vector3 AXIS_VALUES[AXIS_COUNT] = { Vector3(1.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f), Vector3(0.0f, 0.0f, 1.0f) };

		const char SIGNS[] = { '-', '+' };
		const size_t SIGN_COUNT = sizeof(SIGNS) / sizeof(char);
		const float SIGN_VALUES[SIGN_COUNT] = { -1.0f, 1.0f };

		const char* MESH_NAMES[] = { "X_Mesh", "Y_Mesh", "Z_Mesh", "DirectionThingie", "RodThing" };
		const size_t MESH_NAME_COUNT = sizeof(MESH_NAMES) / sizeof(char*);

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

						Reference<FBXData> data = FBXData::Extract(content, logger);
						ASSERT_NE(data, nullptr);

						EXPECT_EQ(data->Settings().forwardAxis, AXIS_VALUES[forwardAxis] * SIGN_VALUES[forwardSign]);
						EXPECT_EQ(data->Settings().upAxis, AXIS_VALUES[upAxis] * SIGN_VALUES[upSign]);

						EXPECT_EQ(data->MeshCount(), MESH_NAME_COUNT);
						bool MESH_PRESENT[MESH_NAME_COUNT];
						for (size_t nameId = 0; nameId < MESH_NAME_COUNT; nameId++)
							MESH_PRESENT[nameId] = false;
						for (size_t meshId = 0; meshId < data->MeshCount(); meshId++) {
							const std::string name = PolyMesh::Reader(data->GetMesh(meshId)->mesh).Name();
							for (size_t nameId = 0; nameId < MESH_NAME_COUNT; nameId++)
								if (name == MESH_NAMES[nameId])
									MESH_PRESENT[nameId] = true;
						}
						for (size_t nameId = 0; nameId < MESH_NAME_COUNT; nameId++)
							EXPECT_TRUE(MESH_PRESENT[nameId]);

						const FBXData::FBXNode* xNode = nullptr;
						const FBXData::FBXNode* yNode = nullptr;
						const FBXData::FBXNode* zNode = nullptr;
						for (size_t i = 0; i < data->RootNode()->children.size(); i++) {
							const FBXData::FBXNode* node = data->RootNode()->children[i];
							if (node->name == "DirectionThingie_X") xNode = node;
							else if (node->name == "DirectionThingie_Y") yNode = node;
							else if (node->name == "DirectionThingie_Z") zNode = node;
						}
						ASSERT_NE(xNode, nullptr);
						ASSERT_NE(yNode, nullptr);
						ASSERT_NE(zNode, nullptr);
						EXPECT_TRUE(
							(Math::SqrMagnitude(xNode->position - Math::Right() * 2.0f) < 0.001f) &&
							(Math::SqrMagnitude(yNode->position - Math::Up() * 2.0f) < 0.001f) &&
							(Math::SqrMagnitude(zNode->position - Math::Forward() * 2.0f) < 0.001f));
						
						RenderFBXDataOnTestEnvironment(data, filePath, XYZ_MATERIALS_BY_PATH, 2.0f);
					}
	}


	TEST(FBXTest, AxisTMP) {
		Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();

		const std::string_view filePath = "Assets/Meshes/FBX/XYZ/XYZ_Forward(-Z)_Up(+Y).fbx";
		Reference<OS::MMappedFile> fileMapping = OS::MMappedFile::Create(filePath, logger);
		ASSERT_NE(fileMapping, nullptr);

		Reference<FBXContent> content = FBXContent::Decode(*fileMapping, logger);
		ASSERT_NE(content, nullptr);
		logger->Info(*content);

		Reference<FBXData> data = FBXData::Extract(content, logger);
		ASSERT_NE(data, nullptr);

		RenderFBXDataOnTestEnvironment(data, filePath, XYZ_MATERIALS_BY_PATH);
	}
}
