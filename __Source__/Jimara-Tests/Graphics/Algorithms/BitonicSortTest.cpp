#include "../../GtestHeaders.h"
#include "OS/Logging/StreamLogger.h"
#include "Core/Stopwatch.h"
#include "Math/Random.h"
#include "Graphics/Data/ShaderBinaries/ShaderLoader.h"
#include "Environment/Rendering/Algorithms/BitonicSort/BitonicSortKernel.h"


namespace Jimara {
	namespace {
		static const constexpr uint32_t BLOCK_SIZE = 512u;
		static const constexpr std::string_view BASE_FOLDER = "Jimara/Environment/Rendering/Algorithms/BitonicSort/";
		static const Graphics::ShaderClass BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP(((std::string)BASE_FOLDER) + "BitonicSort_Floats_PowerOf2");
		static const Graphics::ShaderClass BITONIC_SORT_FLOATS_ANY_SIZE_SINGLE_STEP(((std::string)BASE_FOLDER) + "BitonicSort_Floats_AnySize");
		static const constexpr size_t MAX_LIST_SIZE = (1 << 22);
		static const constexpr size_t MAX_IN_FLIGHT_BUFFERS = 2;
		static const constexpr size_t ITERATION_PER_CONFIGURATION = 4;

		inline static Reference<Graphics::GraphicsDevice> CreateGraphicsDevice() {
			const Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
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

		inline static Reference<Graphics::Shader> GetShader(Graphics::GraphicsDevice* device, Graphics::ShaderSet* shaderSet, const Graphics::ShaderClass* shaderClass) {
			if (shaderClass == nullptr) return nullptr;
			const Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(shaderClass, Graphics::PipelineStage::COMPUTE);
			if (binary == nullptr) {
				device->Log()->Error("BitonicSortTest::GetShader - Failed to load shader for \"", shaderClass->ShaderPath(), "\"!");
				return nullptr;
			}
			const Reference<Graphics::ShaderCache> shaderCache = Graphics::ShaderCache::ForDevice(device);
			if (shaderCache == nullptr) {
				device->Log()->Error("BitonicSortTest::GetShader - Failed to get shader cache!");
				return nullptr;
			}
			const Reference<Graphics::Shader> shader = shaderCache->GetShader(binary);
			if (shader == nullptr) 
				device->Log()->Error("BitonicSortTest::GetShader - Failed to create shader for \"", shaderClass->ShaderPath(), "\"!");
			return shader;
		}

		inline static const std::vector<size_t>& PowersOf2() {
			static const std::vector<size_t> sizes = []() {
				std::vector<size_t> elems;
				for (size_t listSize = 1u; listSize <= MAX_LIST_SIZE; listSize <<= 1u)
					elems.push_back(listSize);
				return elems;
			}();
			return sizes;
		}

		inline static std::vector<size_t> RandomListSizes() {
			std::vector<size_t> elems;
			for (size_t listSize = 1u; listSize <= MAX_LIST_SIZE; listSize <<= 1u)
				elems.push_back(Random::ThreadRNG()() % listSize);
			return elems;
		}

		inline static void FillSequentialAsc(float* values, size_t count) {
			for (size_t i = 0; i < count; i++) values[i] = i;
		}

		inline static void FillRandom(float* values, size_t count) {
			std::mt19937& rng = Random::ThreadRNG();
			std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
			for (size_t i = 0; i < count; i++) values[i] = dis(rng);
		}

		class BitonicSortTestCase {
		private:
			const Reference<OS::Logger> m_log;
			const Reference<Graphics::GraphicsDevice> m_graphicsDevice;
			const Reference<Graphics::ShaderSet> m_shaderSet;
			const Reference<Graphics::CommandPool> m_commandPool;
			const Reference<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding> m_binding;

			Reference<BitonicSortKernel> m_kernel;
			std::vector<float> m_bufferInput;
			Graphics::ArrayBufferReference<float> m_inputBuffer;
			Graphics::ArrayBufferReference<float> m_outputBuffer;

			inline BitonicSortTestCase(const Reference<Graphics::GraphicsDevice>& device)
				: m_log((device != nullptr) ? device->Log() : nullptr)
				, m_graphicsDevice(device)
				, m_shaderSet((device != nullptr) ? GetShaderSet(device->Log()) : nullptr)
				, m_commandPool((device != nullptr) ? device->GraphicsQueue()->CreateCommandPool() : nullptr)
				, m_binding(Object::Instantiate<Graphics::ShaderResourceBindings::NamedStructuredBufferBinding>("elements")) {
			}

			inline bool InitializeKernel(const Graphics::ShaderClass* singleStepShaderClass, const Graphics::ShaderClass* groupsharedShaderClass, size_t inFlightBufferCount) {
				const Reference<Graphics::Shader> singleStepShader = GetShader(m_graphicsDevice, m_shaderSet, singleStepShaderClass);
				const Reference<Graphics::Shader> groupsharedShader = GetShader(m_graphicsDevice, m_shaderSet, groupsharedShaderClass);
				Graphics::ShaderResourceBindings::ShaderBindingDescription bindings;
				const Graphics::ShaderResourceBindings::NamedStructuredBufferBinding* bindingAddr = m_binding;
				bindings.structuredBufferBindings = &bindingAddr;
				bindings.structuredBufferBindingCount = 1u;
				m_kernel = nullptr;
				m_kernel = BitonicSortKernel::Create(m_graphicsDevice, bindings, inFlightBufferCount, BLOCK_SIZE, singleStepShader, groupsharedShader);
				if (m_kernel == nullptr)
					m_log->Error("BitonicSortTestCase::InitializeKernel - Failed to create BitonicSortKernel!");
				return m_kernel != nullptr;
			}

			inline bool SetBufferInputSize(size_t size) {
				m_bufferInput.resize(size);
				m_inputBuffer = nullptr;
				m_outputBuffer = nullptr;
				m_inputBuffer = m_graphicsDevice->CreateArrayBuffer<float>(m_bufferInput.size());
				m_outputBuffer = m_graphicsDevice->CreateArrayBuffer<float>(m_bufferInput.size(), Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
				m_binding->BoundObject() = m_inputBuffer;
				if (m_inputBuffer == nullptr || m_outputBuffer == nullptr)
					m_log->Error("BitonicSortTestCase::SetBufferInputSize - Failed to create buffers!");
				return (m_inputBuffer != nullptr && m_outputBuffer != nullptr);
			}

			inline float FillBuffers(const Callback<float*, size_t> fillList) {
				Stopwatch stopwatch;
				
				fillList(m_bufferInput.data(), m_bufferInput.size());
				std::memcpy(m_outputBuffer->Map(), m_bufferInput.data(), sizeof(float) * m_bufferInput.size());
				m_outputBuffer->Unmap(true);
				
				const Reference<Graphics::PrimaryCommandBuffer> commandBuffer = m_commandPool->CreatePrimaryCommandBuffer();
				commandBuffer->BeginRecording();
				m_inputBuffer->Copy(commandBuffer, m_outputBuffer);
				commandBuffer->EndRecording();
				m_graphicsDevice->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
				commandBuffer->Wait();
				
				return stopwatch.Elapsed();
			}

			inline float ExecutePipeline(size_t commandBufferId) {
				const Reference<Graphics::PrimaryCommandBuffer> commandBuffer = m_commandPool->CreatePrimaryCommandBuffer();
				Stopwatch stopwatch;
				commandBuffer->BeginRecording();
				m_kernel->Execute(Graphics::Pipeline::CommandBufferInfo(commandBuffer, commandBufferId), m_bufferInput.size());
				m_outputBuffer->Copy(commandBuffer, m_inputBuffer);
				commandBuffer->EndRecording();
				m_graphicsDevice->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
				commandBuffer->Wait();
				return stopwatch.Reset();
			}

			inline float SortCPUBuffer() {
				Stopwatch stopwatch;
				std::sort(m_bufferInput.begin(), m_bufferInput.end());
				return stopwatch.Elapsed();
			}

			inline int CompareResults() {
				const int result = std::memcmp(m_outputBuffer->Map(), m_bufferInput.data(), sizeof(float) * m_bufferInput.size());
				m_outputBuffer->Unmap(false);
				if (result != 0)
					m_log->Error("BitonicSortTestCase::CompareResults - Mismatch detected!");
				return result;
			}

		public:
			inline BitonicSortTestCase() : BitonicSortTestCase(CreateGraphicsDevice()) {}
			inline bool Initialized()const { return m_log != nullptr && m_graphicsDevice != nullptr && m_shaderSet != nullptr && m_commandPool != nullptr; }

			inline bool Run(
				const Graphics::ShaderClass* singleStepShaderClass, const Graphics::ShaderClass* groupsharedShaderClass,
				const std::vector<size_t>& listSizes, Callback<float*, size_t> fillList) {
				if (!Initialized()) return false;
				if (!InitializeKernel(singleStepShaderClass, groupsharedShaderClass, MAX_IN_FLIGHT_BUFFERS)) return false;
				for (size_t listSizeId = 0u; listSizeId < listSizes.size(); listSizeId++) {
					if (!SetBufferInputSize(listSizes[listSizeId])) return false;
					float totalGenerationTime = 0.0f;
					float totalGPUTime = 0.0f;
					float totalCPUTime = 0.0f;
					for (size_t iterationId = 0u; iterationId < ITERATION_PER_CONFIGURATION; iterationId++)
						for (size_t commandBufferId = 0; commandBufferId < MAX_IN_FLIGHT_BUFFERS; commandBufferId++) {
							totalGenerationTime += FillBuffers(fillList);
							totalGPUTime += ExecutePipeline(commandBufferId);
							totalCPUTime += SortCPUBuffer();
							const int result = CompareResults();
							if (result != 0) return false;
						}
					m_log->Info(
						std::fixed, std::setprecision(3),
						"Count: ", m_bufferInput.size(),
						"; Gen: ", (totalGenerationTime * 1000.0f / ITERATION_PER_CONFIGURATION / MAX_IN_FLIGHT_BUFFERS), "ms (total: ", totalGenerationTime, "s)",
						"; GPU: ", (totalGPUTime * 1000.0f / ITERATION_PER_CONFIGURATION / MAX_IN_FLIGHT_BUFFERS), "ms (total: ", totalGPUTime, "s)",
						"; CPU: ", (totalCPUTime * 1000.0f / ITERATION_PER_CONFIGURATION / MAX_IN_FLIGHT_BUFFERS), "ms (total: ", totalCPUTime, "s)");
				}
				return true;
			}

			template<typename FillListFn>
			inline bool Run(
				const Graphics::ShaderClass* singleStepShaderClass, const Graphics::ShaderClass* groupsharedShaderClass,
				const std::vector<size_t>& listSizes, const FillListFn& fillList) {
				return Run(singleStepShaderClass, groupsharedShaderClass, listSizes, Callback<float*, size_t>::FromCall(&fillList));
			}
		};
	}

	TEST(BitonicSortTest, AlreadySorted_SingleStep_PowerOf2) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			&BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP,
			nullptr, PowersOf2(), FillSequentialAsc));
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, AlreadySorted_SingleStep_RandomSize) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			&BITONIC_SORT_FLOATS_ANY_SIZE_SINGLE_STEP,
			nullptr, RandomListSizes(), FillSequentialAsc));
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, RandomFloats_SingleStep_PowerOf2) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			&BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP,
			nullptr, PowersOf2(), FillRandom));
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, RandomFloats_SingleStep_RandomSize) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			&BITONIC_SORT_FLOATS_ANY_SIZE_SINGLE_STEP,
			nullptr, RandomListSizes(), FillRandom));
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, AlreadySorted_WithGroupsharedStep_PowerOf2) {
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, AlreadySorted_WithGroupsharedStep_RandomSize) {
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, RandomFloats_WithGroupsharedStep_PowerOf2) {
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, RandomFloats_WithGroupsharedStep_RandomSize) {
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, AlreadySorted_GroupsharedOnly_PowerOf2) {
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, AlreadySorted_GroupsharedOnly_RandomSize) {
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, RandomFloats_GroupsharedOnly_PowerOf2) {
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}

	TEST(BitonicSortTest, RandomFloats_GroupsharedOnly_RandomSize) {
		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}
}
