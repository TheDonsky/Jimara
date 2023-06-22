#include "../GtestHeaders.h"
#include "../CountingLogger.h"
#include "Data/Mesh.h"
#include "Core/TypeRegistration/TypeRegistartion.h"
#include "Data/AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"


namespace Jimara {
	// Test basic FileSystemDatabase construction and queries (for static state)
	TEST(FileSystemDatabaseTest, Basics) {
		Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();

		Reference<Graphics::GraphicsDevice> graphicsDevice = [&]() -> Reference<Graphics::GraphicsDevice> {
			Reference<Application::AppInformation> appInformation =
				Object::Instantiate<Application::AppInformation>("FileSystemDatabaseTest", Application::AppVersion(0, 0, 1));
			Reference<Graphics::GraphicsInstance> graphicsInstance = Graphics::GraphicsInstance::Create(logger, appInformation);
			if (graphicsInstance == nullptr)
				return nullptr;
			for (size_t i = 0u; i < graphicsInstance->PhysicalDeviceCount(); i++)
				if (graphicsInstance->GetPhysicalDevice(i)->Type() == Graphics::PhysicalDevice::DeviceType::DESCRETE) {
					Reference<Graphics::GraphicsDevice> device = graphicsInstance->GetPhysicalDevice(i)->CreateLogicalDevice();
					if (device != nullptr) return device;
				}
			for (size_t i = 0u; i < graphicsInstance->PhysicalDeviceCount(); i++)
				if (graphicsInstance->GetPhysicalDevice(i)->Type() == Graphics::PhysicalDevice::DeviceType::INTEGRATED) {
					Reference<Graphics::GraphicsDevice> device = graphicsInstance->GetPhysicalDevice(i)->CreateLogicalDevice();
					if (device != nullptr) return device;
				}
			for (size_t i = 0u; i < graphicsInstance->PhysicalDeviceCount(); i++) {
				Reference<Graphics::GraphicsDevice> device = graphicsInstance->GetPhysicalDevice(i)->CreateLogicalDevice();
				if (device != nullptr) return device;
			}
			return nullptr;
		}();
		ASSERT_NE(graphicsDevice, nullptr);

		const Reference<Graphics::ShaderLoader> shaderLoader = Graphics::ShaderDirectoryLoader::Create("Shaders/", logger);
		ASSERT_NE(shaderLoader, nullptr);

		Reference<Physics::PhysicsInstance> physicsInstance = Physics::PhysicsInstance::Create(logger);
		ASSERT_NE(physicsInstance, nullptr);

		Reference<Audio::AudioDevice> audioDevice = [&]() -> Reference<Audio::AudioDevice> {
			Reference<Audio::AudioInstance> audioInstance = Audio::AudioInstance::Create(logger);
			if (audioInstance == nullptr) return nullptr;
			if (audioInstance->DefaultDevice() != nullptr) {
				Reference<Audio::AudioDevice> device = audioInstance->DefaultDevice()->CreateLogicalDevice();
				if (device != nullptr) return device;
			}
			for (size_t i = 0u; i < audioInstance->PhysicalDeviceCount(); i++) {
				Reference<Audio::AudioDevice> device = audioInstance->PhysicalDevice(i)->CreateLogicalDevice();
				if (device != nullptr) return device;
			}
			return nullptr;
		}();
		ASSERT_NE(audioDevice, nullptr);

		Reference<BuiltInTypeRegistrator> typeRegistrator = BuiltInTypeRegistrator::Instance();
		Reference<FileSystemDatabase> database = FileSystemDatabase::Create(
			graphicsDevice, shaderLoader, physicsInstance, audioDevice, "Assets");
		ASSERT_NE(database, nullptr);
		
		logger->Info(database->AssetCount(), " Assets found!");
		void(*reportedAssetCounter)(size_t*, const FileSystemDatabase::AssetInformation&) = [](size_t* count, const FileSystemDatabase::AssetInformation&) { (*count)++; };
		{
			size_t assetCount = 0u;
			database->GetAssetsOfType(TypeId::Of<Object>(), Callback(reportedAssetCounter, &assetCount));
			logger->Info("database->GetAssetsOfType<Resource>(TypeId::Of<Object>(), callback) reported ", assetCount, " Assets!");
			EXPECT_EQ(database->AssetCount(), assetCount);
		}
		{
			size_t assetCount = 0u;
			database->GetAssetsOfType(TypeId::Of<Object>(), [&](const FileSystemDatabase::AssetInformation&) { assetCount++; });
			logger->Info("database->GetAssetsOfType<Resource>(TypeId::Of<Object>(), lambda) reported ", assetCount, " Assets!");
			EXPECT_EQ(database->AssetCount(), assetCount);
		}
		{
			size_t assetCount = 0u;
			database->GetAssetsOfType<Resource>(Callback(reportedAssetCounter, &assetCount));
			logger->Info("database->GetAssetsOfType<Resource>(callback) reported ", assetCount, " Assets!");
			EXPECT_EQ(database->AssetCount(), assetCount);
		}
		{
			size_t assetCount = 0u;
			database->GetAssetsOfType<Resource>([&](const FileSystemDatabase::AssetInformation&) { assetCount++; });
			logger->Info("database->GetAssetsOfType<Resource>(lambda) reported ", assetCount, " Assets!");
			EXPECT_EQ(database->AssetCount(), assetCount);
		}
		{
			size_t assetCount = 0u;
			database->GetAssetsOfType<Resource>([&](const FileSystemDatabase::AssetInformation&) { assetCount++; }, true);
			logger->Info("database->GetAssetsOfType<Resource>(lambda, true) reported ", assetCount, " Assets!");
			EXPECT_EQ(assetCount, 0u);
		}
		{
			size_t assetCount = 0u;
			database->GetAssetsOfType<FileSystemDatabase>([&](const FileSystemDatabase::AssetInformation&) { assetCount++; });
			logger->Info("database->GetAssetsOfType<FileSystemDatabase>(lambda) reported ", assetCount, " Assets!");
			EXPECT_EQ(assetCount, 0u);
		}
		{
			size_t triMeshCount = 0u;
			size_t polyMeshCount = 0u;
			database->GetAssetsOfType<TriMesh>([&](const FileSystemDatabase::AssetInformation&) { triMeshCount++; });
			database->GetAssetsOfType<PolyMesh>([&](const FileSystemDatabase::AssetInformation&) { polyMeshCount++; });
			logger->Info("database->GetAssetsOfType<(Poly/Tri)Mesh>(lambda) reported ", triMeshCount, " TriMesh and ", polyMeshCount, " PolyMesh Assets!");
			EXPECT_GT(triMeshCount, 0u);
			EXPECT_LT(polyMeshCount, database->AssetCount());
			EXPECT_EQ(triMeshCount, polyMeshCount);
		}
		{
			size_t bearCountCallback = 0u;
			size_t bearCountLambda = 0u;
			database->GetAssetsByName("bear", Callback(reportedAssetCounter, &bearCountCallback));
			database->GetAssetsByName("bear", [&](const FileSystemDatabase::AssetInformation&) { bearCountLambda++; });
			EXPECT_EQ(bearCountCallback, bearCountLambda);
			EXPECT_GT(bearCountCallback, 0u);

			size_t beCountCallback = 0u;
			size_t beCountLambda = 0u;
			database->GetAssetsByName("be", Callback(reportedAssetCounter, &beCountCallback));
			database->GetAssetsByName("be", [&](const FileSystemDatabase::AssetInformation&) { beCountLambda++; });
			EXPECT_EQ(beCountCallback, beCountLambda);
			EXPECT_GT(beCountCallback, 0u);
			EXPECT_GE(beCountCallback, bearCountCallback);

			size_t bearrrrCountCallback = 0u;
			size_t bearrrrCountLambda = 0u;
			database->GetAssetsByName("bearrrr", Callback(reportedAssetCounter, &bearrrrCountCallback));
			database->GetAssetsByName("bearrrr", [&](const FileSystemDatabase::AssetInformation&) { bearrrrCountLambda++; });
			EXPECT_EQ(bearrrrCountCallback, bearrrrCountLambda);
			EXPECT_EQ(bearrrrCountCallback, 0u);

			size_t beCountCallbackExact = 0u;
			size_t beCountLambdaExact = 0u;
			database->GetAssetsByName("be", Callback(reportedAssetCounter, &beCountCallbackExact), true);
			database->GetAssetsByName("be", [&](const FileSystemDatabase::AssetInformation&) { beCountLambdaExact++; }, true);
			EXPECT_EQ(beCountCallbackExact, beCountLambdaExact);
			EXPECT_EQ(beCountCallbackExact, 0u);

			size_t bearCountCallbackTri = 0u;
			size_t bearCountLambdaTri = 0u;
			database->GetAssetsByName<TriMesh>("bear", Callback(reportedAssetCounter, &bearCountCallbackTri));
			database->GetAssetsByName<TriMesh>("bear", [&](const FileSystemDatabase::AssetInformation&) { bearCountLambdaTri++; });
			EXPECT_EQ(bearCountCallbackTri, bearCountLambdaTri);
			EXPECT_GT(bearCountCallbackTri, 0u);

			size_t bearCountCallbackPoly = 0u;
			size_t bearCountLambdaPoly = 0u;
			database->GetAssetsByName<PolyMesh>("bear", Callback(reportedAssetCounter, &bearCountCallbackPoly));
			database->GetAssetsByName<PolyMesh>("bear", [&](const FileSystemDatabase::AssetInformation&) { bearCountLambdaPoly++; });
			EXPECT_EQ(bearCountCallbackPoly, bearCountLambdaPoly);
			EXPECT_EQ(bearCountCallbackTri, bearCountLambdaPoly);
			EXPECT_GT(bearCountCallbackPoly, 0u);

			size_t bearCountCallbackExact = 0u;
			size_t bearCountLambdaExact = 0u;
			database->GetAssetsByName<PolyMesh>("bear", Callback(reportedAssetCounter, &bearCountCallbackExact), true);
			database->GetAssetsByName<PolyMesh>("bear", [&](const FileSystemDatabase::AssetInformation&) { bearCountLambdaExact++; }, true);
			EXPECT_EQ(bearCountCallbackExact, bearCountLambdaExact);
			EXPECT_GE(bearCountCallbackTri, bearCountLambdaExact);
			EXPECT_GT(bearCountCallbackExact, 0u);

			size_t bearCallbackExactType = 0u;
			size_t bearLambdaExactType = 0u;
			database->GetAssetsByName<Resource>("bear", Callback(reportedAssetCounter, &bearCallbackExactType), false, true);
			database->GetAssetsByName<Resource>("bear", [&](const FileSystemDatabase::AssetInformation&) { bearLambdaExactType++; }, false, true);
			EXPECT_EQ(bearCallbackExactType, bearLambdaExactType);
			EXPECT_EQ(bearLambdaExactType, 0u);

			logger->Info("database->GetAssetsByName(\"bear\", callback/lambda) reported:", 
				"\n    bearCountCallback:      ", bearCountCallback,      "; bearCountLambda:      ", bearCountLambda, ";",
				"\n    beCountCallback:        ", beCountCallback,        "; beCountLambda:        ", beCountLambda, ";",
				"\n    bearrrrCountCallback:   ", bearrrrCountCallback,   "; bearrrrCountLambda:   ", bearrrrCountLambda, ";",
				"\n    beCountCallbackExact:   ", beCountCallbackExact,   "; beCountLambdaExact:   ", beCountLambdaExact, ";",
				"\n    bearCountCallbackTri:   ", bearCountCallbackTri,   "; bearCountLambdaTri:   ", bearCountLambdaTri, ";"
				"\n    bearCountCallbackPoly:  ", bearCountCallbackPoly,  "; bearCountLambdaPoly:  ", bearCountLambdaPoly, ";"
				"\n    bearCountCallbackExact: ", bearCountCallbackExact, "; bearCountLambdaExact: ", bearCountLambdaExact, ";"
				"\n    bearCallbackExactType:  ", bearCallbackExactType,  "; bearLambdaExactType:  ", bearLambdaExactType, "!");
		}
		{
			size_t assetCountCallback = 0u;
			size_t assetCountLambda = 0u;
			const char* PATH = "Assets/random_path_that_does_not_exist.file";
			database->GetAssetsFromFile(PATH, Callback(reportedAssetCounter, &assetCountCallback));
			database->GetAssetsFromFile(PATH, [&](const FileSystemDatabase::AssetInformation&) { assetCountLambda++; });
			logger->Info("database->GetAssetsFromFile(\"", PATH, "\", callback;lambda) reported (", assetCountCallback, ";", assetCountLambda, ") Assets!");
			EXPECT_EQ(assetCountCallback, assetCountLambda);
			EXPECT_EQ(assetCountCallback, 0u);
		}
		{
			size_t assetCountCallback = 0u;
			size_t assetCountLambda = 0u;
			const char* PATH = "Assets/Meshes/OBJ/Bear/bear_diffuse.png";
			database->GetAssetsFromFile(PATH, Callback(reportedAssetCounter, &assetCountCallback));
			database->GetAssetsFromFile(PATH, [&](const FileSystemDatabase::AssetInformation&) { assetCountLambda++; });
			logger->Info("database->GetAssetsFromFile(\"", PATH, "\", callback;lambda) reported (", assetCountCallback, ";", assetCountLambda, ") Assets!");
			EXPECT_EQ(assetCountCallback, assetCountLambda);
			EXPECT_EQ(assetCountCallback, 1u);
		}
		{
			size_t assetCountCallback = 0u;
			size_t assetCountLambda = 0u;
			const char* PATH = "Assets/Meshes/OBJ/Bear/bear_diffuse.png.jado";
			database->GetAssetsFromFile(PATH, Callback(reportedAssetCounter, &assetCountCallback));
			database->GetAssetsFromFile(PATH, [&](const FileSystemDatabase::AssetInformation&) { assetCountLambda++; });
			logger->Info("database->GetAssetsFromFile(\"", PATH, "\", callback;lambda) reported (", assetCountCallback, ";", assetCountLambda, ") Assets!");
			EXPECT_EQ(assetCountCallback, assetCountLambda);
			EXPECT_EQ(assetCountCallback, 0u);
		}
		{
			const char* PATH = "Assets/Meshes/OBJ/Bear/ursus_proximus.obj";

			size_t assetCountCallback = 0u;
			size_t assetCountLambda = 0u;
			database->GetAssetsFromFile(PATH, Callback(reportedAssetCounter, &assetCountCallback));
			database->GetAssetsFromFile(PATH, [&](const FileSystemDatabase::AssetInformation&) { assetCountLambda++; });
			EXPECT_EQ(assetCountCallback, assetCountLambda);
			EXPECT_EQ(assetCountCallback, 16);

			size_t assetCountCallbackTri = 0u;
			size_t assetCountLambdaTri = 0u;
			database->GetAssetsFromFile<TriMesh>(PATH, Callback(reportedAssetCounter, &assetCountCallbackTri));
			database->GetAssetsFromFile<TriMesh>(PATH, [&](const FileSystemDatabase::AssetInformation&) { assetCountLambdaTri++; });
			EXPECT_EQ(assetCountCallbackTri, assetCountLambdaTri);
			EXPECT_EQ(assetCountCallbackTri * 3u + 1u, assetCountLambda);
			EXPECT_EQ(assetCountCallbackTri, 5u);

			size_t assetCountCallbackPoly = 0u;
			size_t assetCountLambdaPoly = 0u;
			database->GetAssetsFromFile(PATH, Callback(reportedAssetCounter, &assetCountCallbackPoly), TypeId::Of<PolyMesh>());
			database->GetAssetsFromFile(PATH, [&](const FileSystemDatabase::AssetInformation&) { assetCountLambdaPoly++; }, TypeId::Of<PolyMesh>(), true);
			EXPECT_EQ(assetCountCallbackPoly, assetCountLambdaPoly);
			EXPECT_EQ(assetCountCallbackPoly * 3u + 1u, assetCountLambda);
			EXPECT_EQ(assetCountCallbackPoly, 5u);

			size_t assetCountWrongType = 0u;
			size_t assetCountStrinctType = 0u;
			database->GetAssetsFromFile(PATH, Callback(reportedAssetCounter, &assetCountWrongType), TypeId::Of<FileSystemDatabase>());
			database->GetAssetsFromFile(PATH, [&](const FileSystemDatabase::AssetInformation&) { assetCountStrinctType++; }, TypeId::Of<Resource>(), true);
			EXPECT_EQ(assetCountWrongType, assetCountStrinctType);
			EXPECT_EQ(assetCountWrongType, 0u);

			logger->Info("database->GetAssetsFromFile(\"", PATH, "\", callback;lambda) reported:", 
				"\n    assetCountCallback:     ", assetCountCallback,     "; assetCountLambda:      ", assetCountLambda, ";"
				"\n    assetCountCallbackTri:  ", assetCountCallbackTri,  "; assetCountLambdaTri:   ", assetCountLambdaTri, ";"
				"\n    assetCountCallbackPoly: ", assetCountCallbackPoly, "; assetCountLambdaPoly:  ", assetCountLambdaPoly, ";"
				"\n    assetCountWrongType:    ", assetCountWrongType,    "; assetCountStrinctType: ", assetCountStrinctType, "!");
		}
		{
			size_t assetCountCallback = 0u;
			size_t assetCountLambda = 0u;
			const char* REL_PATH = "Assets/Meshes/OBJ/Bear/bear_diffuse.png";
			const OS::Path PATH = std::filesystem::absolute(REL_PATH);
			EXPECT_FALSE(std::string(PATH) == REL_PATH);
			database->GetAssetsFromFile(PATH, Callback(reportedAssetCounter, &assetCountCallback));
			database->GetAssetsFromFile(PATH, [&](const FileSystemDatabase::AssetInformation&) { assetCountLambda++; });
			logger->Info("database->GetAssetsFromFile(\"", PATH, "\", callback;lambda) reported (", assetCountCallback, ";", assetCountLambda, ") Assets!");
			EXPECT_EQ(assetCountCallback, assetCountLambda);
			EXPECT_EQ(assetCountCallback, 1u);
		}
	}
}
