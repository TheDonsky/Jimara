#include "../../GtestHeaders.h"
#include "OS/Logging/StreamLogger.h"
#include "Core/Stopwatch.h"
#include "Graphics/Data/ShaderBinaries/ShaderLoader.h"
#include "Environment/Rendering/Algorithms/BitonicSort/BitonicSortKernel.h"


namespace Jimara {
	namespace {
		static const constexpr uint32_t BLOCK_SIZE = 512u;
		static const constexpr std::string_view BASE_FOLDER = "Jimara/Environment/Rendering/Algorithms/BitonicSort/";
		static const Graphics::ShaderClass BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP(((std::string)BASE_FOLDER) + "BitonicSort_Floats_PowerOf2");
		static const Graphics::ShaderClass BITONIC_SORT_FLOATS_ANY_SIZE_SINGLE_STEP(((std::string)BASE_FOLDER) + "BitonicSort_Floats_AnySize");
		static const constexpr size_t MAX_LIST_SIZE = (1 << 24);

		inline static Reference<Graphics::GraphicsDevice> CreateGraphicsDevice(OS::Logger* logger) {
			const Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("Jimara_BitonicSortTest");
			const Reference<Graphics::GraphicsInstance> instance = Graphics::GraphicsInstance::Create(logger, appInfo);
			if (instance == nullptr) {
				logger->Error("BitonicSortTest::CreateGraphicsDevice - Failed to create graphics instance!");
				return nullptr;
			}
			Graphics::PhysicalDevice* physicalDevice = nullptr;
			for (size_t i = 0; i < instance->PhysicalDeviceCount(); i++) {
				Graphics::PhysicalDevice* device = instance->GetPhysicalDevice(i);
				if (device == nullptr || (!device->HasFeature(Graphics::PhysicalDevice::DeviceFeature::COMPUTE)))
					continue;
				if (physicalDevice == nullptr) 
					physicalDevice = device;
				else if (
					(device->Type() != Graphics::PhysicalDevice::DeviceType::VIRTUAL) && 
					(device->Type() > physicalDevice->Type() || physicalDevice->Type() == Graphics::PhysicalDevice::DeviceType::VIRTUAL))
					physicalDevice = device;
			}
			if (physicalDevice == nullptr) {
				logger->Error("BitonicSortTest::CreateGraphicsDevice - No compatible device found on the system!");
				return nullptr;
			}
			const Reference<Graphics::GraphicsDevice> device = physicalDevice->CreateLogicalDevice();
			if (device == nullptr)
				logger->Error("BitonicSortTest::CreateGraphicsDevice - Failed to create graphics device!");
			return device;
		}

		inline static Reference<Graphics::ShaderSet> GetShaderSet(OS::Logger* logger) {
			Reference<Graphics::ShaderLoader> shaderLoader = Graphics::ShaderDirectoryLoader::Create("Shaders/", logger);
			if (shaderLoader == nullptr) {
				logger->Error("BitonicSortTest::GetShaderSet - Failed to create shader loader!");
				return nullptr;
			}
			Reference<Graphics::ShaderSet> shaderSet = shaderLoader->LoadShaderSet("");
			if (shaderSet == nullptr) 
				logger->Error("BitonicSortTest::GetShaderSet - Failed to retrieve shader set!");
			return shaderSet;
		}

		inline static Reference<Graphics::Shader> GetShader(Graphics::GraphicsDevice* device, Graphics::ShaderSet* shaderSet, const Graphics::ShaderClass& shaderClass) {
			const Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(&shaderClass, Graphics::PipelineStage::COMPUTE);
			if (binary == nullptr) {
				device->Log()->Error("BitonicSortTest::GetShader - Failed to load shader for \"", shaderClass.ShaderPath(), "\"!");
				return nullptr;
			}
			const Reference<Graphics::ShaderCache> shaderCache = Graphics::ShaderCache::ForDevice(device);
			if (shaderCache == nullptr) {
				device->Log()->Error("BitonicSortTest::GetShader - Failed to get shader cache!");
				return nullptr;
			}
			const Reference<Graphics::Shader> shader = shaderCache->GetShader(binary);
			if (shader == nullptr) 
				device->Log()->Error("BitonicSortTest::GetShader - Failed to create shader for \"", shaderClass.ShaderPath(), "\"!");
			return shader;
		}

#define JIMARA_BitonicSortTest_InitializeTestCase \
		const Reference<OS::Logger> log = Object::Instantiate<OS::StreamLogger>(); \
		const Reference<Graphics::GraphicsDevice> device = CreateGraphicsDevice(log); \
		ASSERT_NE(device, nullptr); \
		const Reference<Graphics::ShaderSet> shaderSet = GetShaderSet(log); \
		ASSERT_NE(shaderSet, nullptr)

#define JIMARA_BitonicSortTest_LoadShader(shaderName, shaderClass) \
		const Reference<Graphics::Shader> shaderName = GetShader(device, shaderSet, shaderClass); \
		ASSERT_NE(shaderName, nullptr)
	}

	TEST(BitonicSortTest, AlreadySorted_SingleStep_PowerOf2) {
		JIMARA_BitonicSortTest_InitializeTestCase;
		JIMARA_BitonicSortTest_LoadShader(shader, BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP);
		
		// __TODO__: Implement this crap!

		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, AlreadySorted_SingleStep_RandomSize) {
		JIMARA_BitonicSortTest_InitializeTestCase;
		JIMARA_BitonicSortTest_LoadShader(shader, BITONIC_SORT_FLOATS_ANY_SIZE_SINGLE_STEP);
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, RandomFloats_SingleStep_PowerOf2) {
		JIMARA_BitonicSortTest_InitializeTestCase;
		JIMARA_BitonicSortTest_LoadShader(shader, BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP);
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, RandomFloats_SingleStep_RandomSize) {
		JIMARA_BitonicSortTest_InitializeTestCase;
		JIMARA_BitonicSortTest_LoadShader(shader, BITONIC_SORT_FLOATS_ANY_SIZE_SINGLE_STEP);
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, AlreadySorted_WithGroupsharedStep_PowerOf2) {
		JIMARA_BitonicSortTest_InitializeTestCase;
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, AlreadySorted_WithGroupsharedStep_RandomSize) {
		JIMARA_BitonicSortTest_InitializeTestCase;
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, RandomFloats_WithGroupsharedStep_PowerOf2) {
		JIMARA_BitonicSortTest_InitializeTestCase;
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, RandomFloats_WithGroupsharedStep_RandomSize) {
		JIMARA_BitonicSortTest_InitializeTestCase;
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, AlreadySorted_GroupsharedOnly_PowerOf2) {
		JIMARA_BitonicSortTest_InitializeTestCase;
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, AlreadySorted_GroupsharedOnly_RandomSize) {
		JIMARA_BitonicSortTest_InitializeTestCase;
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, RandomFloats_GroupsharedOnly_PowerOf2) {
		JIMARA_BitonicSortTest_InitializeTestCase;
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, RandomFloats_GroupsharedOnly_RandomSize) {
		JIMARA_BitonicSortTest_InitializeTestCase;
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}
}
