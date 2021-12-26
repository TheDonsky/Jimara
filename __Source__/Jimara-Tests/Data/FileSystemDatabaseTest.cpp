#include "../GtestHeaders.h"
#include "../CountingLogger.h"
#include "Data/Mesh.h"
#include "Data/TypeRegistration/TypeRegistartion.h"
#include "Data/AssetDatabase/FileSystemDatabase/FileSystemDatabase.h"


namespace Jimara {
	TEST(FileSystemDatabaseTest, Basics) {
		Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();

		Reference<Graphics::GraphicsDevice> graphicsDevice = [&]() -> Reference<Graphics::GraphicsDevice> {
			Reference<Application::AppInformation> appInformation =
				Object::Instantiate<Application::AppInformation>("FileSystemDatabaseTest", Application::AppVersion(0, 0, 1));
			Reference<Graphics::GraphicsInstance> graphicsInstance = Graphics::GraphicsInstance::Create(logger, appInformation);
			if (graphicsInstance == nullptr)
				return nullptr;
			for (size_t i = 0; i < graphicsInstance->PhysicalDeviceCount(); i++)
				if (graphicsInstance->GetPhysicalDevice(i)->Type() == Graphics::PhysicalDevice::DeviceType::DESCRETE) {
					Reference<Graphics::GraphicsDevice> device = graphicsInstance->GetPhysicalDevice(i)->CreateLogicalDevice();
					if (device != nullptr) return device;
				}
			for (size_t i = 0; i < graphicsInstance->PhysicalDeviceCount(); i++)
				if (graphicsInstance->GetPhysicalDevice(i)->Type() == Graphics::PhysicalDevice::DeviceType::INTEGRATED) {
					Reference<Graphics::GraphicsDevice> device = graphicsInstance->GetPhysicalDevice(i)->CreateLogicalDevice();
					if (device != nullptr) return device;
				}
			for (size_t i = 0; i < graphicsInstance->PhysicalDeviceCount(); i++) {
				Reference<Graphics::GraphicsDevice> device = graphicsInstance->GetPhysicalDevice(i)->CreateLogicalDevice();
				if (device != nullptr) return device;
			}
			return nullptr;
		}();
		ASSERT_NE(graphicsDevice, nullptr);

		Reference<Audio::AudioDevice> audioDevice = [&]() -> Reference<Audio::AudioDevice> {
			Reference<Audio::AudioInstance> audioInstance = Audio::AudioInstance::Create(logger);
			if (audioInstance == nullptr) return nullptr;
			if (audioInstance->DefaultDevice() != nullptr) {
				Reference<Audio::AudioDevice> device = audioInstance->DefaultDevice()->CreateLogicalDevice();
				if (device != nullptr) return device;
			}
			for (size_t i = 0; i < audioInstance->PhysicalDeviceCount(); i++) {
				Reference<Audio::AudioDevice> device = audioInstance->PhysicalDevice(i)->CreateLogicalDevice();
				if (device != nullptr) return device;
			}
			return nullptr;
		}();
		ASSERT_NE(audioDevice, nullptr);

		Reference<BuiltInTypeRegistrator> typeRegistrator = BuiltInTypeRegistrator::Instance();
		Reference<FileSystemDatabase> database = FileSystemDatabase::Create(graphicsDevice, audioDevice, "Assets");
		ASSERT_NE(database, nullptr);
		
		logger->Info(database->AssetCount(), " Assets found!");
		void(*reportedAssetCounter)(size_t*, const FileSystemDatabase::AssetInformation&) = [](size_t* count, const FileSystemDatabase::AssetInformation&) { (*count)++; };
		{
			size_t assetCount = 0;
			database->GetAssetsOfType(TypeId::Of<Object>(), Callback(reportedAssetCounter, &assetCount));
			logger->Info("database->GetAssetsOfType<Resource>(TypeId::Of<Object>(), callback) reported ", assetCount, " Assets!");
			EXPECT_EQ(database->AssetCount(), assetCount);
		}
		{
			size_t assetCount = 0;
			database->GetAssetsOfType(TypeId::Of<Object>(), [&](const FileSystemDatabase::AssetInformation&) { assetCount++; });
			logger->Info("database->GetAssetsOfType<Resource>(TypeId::Of<Object>(), lambda) reported ", assetCount, " Assets!");
			EXPECT_EQ(database->AssetCount(), assetCount);
		}
		{
			size_t assetCount = 0;
			database->GetAssetsOfType<Resource>(Callback(reportedAssetCounter, &assetCount));
			logger->Info("database->GetAssetsOfType<Resource>(callback) reported ", assetCount, " Assets!");
			EXPECT_EQ(database->AssetCount(), assetCount);
		}
		{
			size_t assetCount = 0;
			database->GetAssetsOfType<Resource>([&](const FileSystemDatabase::AssetInformation&) { assetCount++; });
			logger->Info("database->GetAssetsOfType<Resource>(lambda) reported ", assetCount, " Assets!");
			EXPECT_EQ(database->AssetCount(), assetCount);
		}
		{
			size_t assetCount = 0;
			database->GetAssetsOfType<Resource>([&](const FileSystemDatabase::AssetInformation&) { assetCount++; }, true);
			logger->Info("database->GetAssetsOfType<Resource>(lambda, true) reported ", assetCount, " Assets!");
			EXPECT_EQ(assetCount, 0);
		}
		{
			size_t assetCount = 0;
			database->GetAssetsOfType<FileSystemDatabase>([&](const FileSystemDatabase::AssetInformation&) { assetCount++; });
			logger->Info("database->GetAssetsOfType<FileSystemDatabase>(lambda) reported ", assetCount, " Assets!");
			EXPECT_EQ(assetCount, 0);
		}
		{
			size_t triMeshCount = 0;
			size_t polyMeshCount = 0;
			database->GetAssetsOfType<TriMesh>([&](const FileSystemDatabase::AssetInformation&) { triMeshCount++; });
			database->GetAssetsOfType<PolyMesh>([&](const FileSystemDatabase::AssetInformation&) { polyMeshCount++; });
			logger->Info("database->GetAssetsOfType<(Poly/Tri)Mesh>(lambda) reported ", triMeshCount, " TriMesh and ", polyMeshCount, " PolyMesh Assets!");
			EXPECT_GT(triMeshCount, 0);
			EXPECT_LT(polyMeshCount, database->AssetCount());
			EXPECT_EQ(triMeshCount, polyMeshCount);
		}
		{
			size_t bearCountCallback = 0;
			size_t bearCountLambda = 0;
			database->GetAssetsByName("bear", Callback(reportedAssetCounter, &bearCountCallback));
			database->GetAssetsByName("bear", [&](const FileSystemDatabase::AssetInformation&) { bearCountLambda++; });
			EXPECT_EQ(bearCountCallback, bearCountLambda);
			EXPECT_GT(bearCountCallback, 0);

			size_t beCountCallback = 0;
			size_t beCountLambda = 0;
			database->GetAssetsByName("be", Callback(reportedAssetCounter, &beCountCallback));
			database->GetAssetsByName("be", [&](const FileSystemDatabase::AssetInformation&) { beCountLambda++; });
			EXPECT_EQ(beCountCallback, beCountLambda);
			EXPECT_GT(beCountCallback, 0);
			EXPECT_GE(beCountCallback, bearCountCallback);

			size_t bearrrrCountCallback = 0;
			size_t bearrrrCountLambda = 0;
			database->GetAssetsByName("bearrrr", Callback(reportedAssetCounter, &bearrrrCountCallback));
			database->GetAssetsByName("bearrrr", [&](const FileSystemDatabase::AssetInformation&) { bearrrrCountLambda++; });
			EXPECT_EQ(bearrrrCountCallback, bearrrrCountLambda);
			EXPECT_EQ(bearrrrCountCallback, 0);

			size_t beCountCallbackExact = 0;
			size_t beCountLambdaExact = 0;
			database->GetAssetsByName("be", Callback(reportedAssetCounter, &beCountCallbackExact), true);
			database->GetAssetsByName("be", [&](const FileSystemDatabase::AssetInformation&) { beCountLambdaExact++; }, true);
			EXPECT_EQ(beCountCallbackExact, beCountLambdaExact);
			EXPECT_EQ(beCountCallbackExact, 0);

			size_t bearCountCallbackTri = 0;
			size_t bearCountLambdaTri = 0;
			database->GetAssetsByName<TriMesh>("bear", Callback(reportedAssetCounter, &bearCountCallbackTri));
			database->GetAssetsByName<TriMesh>("bear", [&](const FileSystemDatabase::AssetInformation&) { bearCountLambdaTri++; });
			EXPECT_EQ(bearCountCallbackTri, bearCountLambdaTri);
			EXPECT_GT(bearCountCallbackTri, 0);

			size_t bearCountCallbackPoly = 0;
			size_t bearCountLambdaPoly = 0;
			database->GetAssetsByName<PolyMesh>("bear", Callback(reportedAssetCounter, &bearCountCallbackPoly));
			database->GetAssetsByName<PolyMesh>("bear", [&](const FileSystemDatabase::AssetInformation&) { bearCountLambdaPoly++; });
			EXPECT_EQ(bearCountCallbackPoly, bearCountLambdaPoly);
			EXPECT_EQ(bearCountCallbackTri, bearCountLambdaPoly);
			EXPECT_GT(bearCountCallbackPoly, 0);

			size_t bearCountCallbackExact = 0;
			size_t bearCountLambdaExact = 0;
			database->GetAssetsByName<PolyMesh>("bear", Callback(reportedAssetCounter, &bearCountCallbackExact), true);
			database->GetAssetsByName<PolyMesh>("bear", [&](const FileSystemDatabase::AssetInformation&) { bearCountLambdaExact++; }, true);
			EXPECT_EQ(bearCountCallbackExact, bearCountLambdaExact);
			EXPECT_GE(bearCountCallbackTri, bearCountLambdaExact);
			EXPECT_GT(bearCountCallbackExact, 0);

			size_t bearCallbackExactType = 0;
			size_t bearLambdaExactType = 0;
			database->GetAssetsByName<Resource>("bear", Callback(reportedAssetCounter, &bearCallbackExactType), false, true);
			database->GetAssetsByName<Resource>("bear", [&](const FileSystemDatabase::AssetInformation&) { bearLambdaExactType++; }, false, true);
			EXPECT_EQ(bearCallbackExactType, bearLambdaExactType);
			EXPECT_EQ(bearLambdaExactType, 0);

			logger->Info("database->GetAssetsByName(\"bear\", callback/lambda) reported:", 
				"\n    bearCountCallback:      ", bearCountCallback, "; bearCountLambda:      ", bearCountLambda, ";",
				"\n    beCountCallback:        ", beCountCallback, "; beCountLambda:        ", beCountLambda, ";",
				"\n    bearrrrCountCallback:   ", bearrrrCountCallback, "; bearrrrCountLambda:   ", bearrrrCountLambda, ";",
				"\n    beCountCallbackExact:   ", beCountCallbackExact, "; beCountLambdaExact:   ", beCountLambdaExact, ";",
				"\n    bearCountCallbackTri:   ", bearCountCallbackTri, "; bearCountLambdaTri:   ", bearCountLambdaTri, ";"
				"\n    bearCountCallbackPoly:  ", bearCountCallbackPoly, "; bearCountLambdaPoly:  ", bearCountLambdaPoly, ";"
				"\n    bearCountCallbackExact: ", bearCountCallbackExact, "; bearCountLambdaExact: ", bearCountLambdaExact, ";"
				"\n    bearCallbackExactType:  ", bearCallbackExactType, "; bearLambdaExactType:  ", bearLambdaExactType, "!");
		}
	}
}
