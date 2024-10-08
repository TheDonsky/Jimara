#include "../../GtestHeaders.h"
#include "../TestEnvironmentCreation.h"
#include "Core/Stopwatch.h"
#include "Math/Random.h"
#include "Environment/Rendering/Algorithms/BitonicSort/BitonicSortKernel.h"


namespace Jimara {
	namespace {
		static const constexpr uint32_t BLOCK_SIZE = 512u;
		static const constexpr std::string_view BASE_FOLDER = "Jimara/Environment/Rendering/Algorithms/BitonicSort/";
		static const std::string BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP = (((std::string)BASE_FOLDER) + "BitonicSort_Floats_SingleStep.comp");
		static const std::string BITONIC_SORT_FLOATS_GROUPSHARED(((std::string)BASE_FOLDER) + "BitonicSort_Floats_Groupshared.comp");
		static const constexpr size_t MAX_LIST_SIZE = (1 << 22);
		static const constexpr size_t MAX_IN_FLIGHT_BUFFERS = 2;
		static const constexpr size_t ITERATION_PER_CONFIGURATION = 4;


		inline static Reference<ShaderLibrary> GetShaderSet(OS::Logger* logger) {
			Reference<ShaderLibrary> shaderLibrary = FileSystemShaderLibrary::Create("Shaders/", logger);
			if (shaderLibrary == nullptr) {
				logger->Error("BitonicSortTest::GetShaderSet - Failed to create shader loader!");
				return nullptr;
			}
			return shaderLibrary;
		}

		inline static Reference<Graphics::SPIRV_Binary> GetShader(Graphics::GraphicsDevice* device, ShaderLibrary* shaderLibrary, const std::string_view& shaderPath) {
			if (shaderPath.empty())
				return nullptr;
			const Reference<Graphics::SPIRV_Binary> binary = shaderLibrary->LoadShader(shaderPath);
			if (binary == nullptr) {
				device->Log()->Error("BitonicSortTest::GetShader - Failed to load shader for \"", shaderPath, "\"!");
				return nullptr;
			}
			return binary;
		}

		inline static void FillSequentialAsc(float* values, size_t count) {
			for (size_t i = 0; i < count; i++) values[i] = static_cast<float>(i);
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
			const Reference<ShaderLibrary> m_shaderLibrary;
			const Reference<Graphics::CommandPool> m_commandPool;
			const Reference<Graphics::ResourceBinding<Graphics::ArrayBuffer>> m_binding;

			Reference<BitonicSortKernel> m_kernel;
			std::vector<float> m_bufferInput;
			Graphics::ArrayBufferReference<float> m_inputBuffer;
			Graphics::ArrayBufferReference<float> m_outputBuffer;

			inline BitonicSortTestCase(const Reference<Graphics::GraphicsDevice>& device)
				: m_log((device != nullptr) ? device->Log() : nullptr)
				, m_graphicsDevice(device)
				, m_shaderLibrary((device != nullptr) ? GetShaderSet(device->Log()) : nullptr)
				, m_commandPool((device != nullptr) ? device->GraphicsQueue()->CreateCommandPool() : nullptr)
				, m_binding(Object::Instantiate<Graphics::ResourceBinding<Graphics::ArrayBuffer>>()) {
			}

			inline bool InitializeKernel(const std::string_view& singleStepShaderPath, const std::string_view& groupsharedShaderPath, size_t inFlightBufferCount) {
				const Reference<Graphics::SPIRV_Binary> singleStepShader = GetShader(m_graphicsDevice, m_shaderLibrary, singleStepShaderPath);
				const Reference<Graphics::SPIRV_Binary> groupsharedShader = GetShader(m_graphicsDevice, m_shaderLibrary, groupsharedShaderPath);
				auto findStructuredBuffer = [&](const Graphics::BindingSet::BindingDescriptor& desc) 
					-> const Graphics::ResourceBinding<Graphics::ArrayBuffer>* {
					if (desc.name == "elements") return m_binding;
					else return nullptr;
				};
				Graphics::BindingSet::BindingSearchFunctions search = {};
				search.structuredBuffer = &findStructuredBuffer;
				m_kernel = nullptr;
				m_kernel = BitonicSortKernel::Create(m_graphicsDevice, search, inFlightBufferCount, BLOCK_SIZE, singleStepShader, groupsharedShader);
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
				m_kernel->Execute(Graphics::InFlightBufferInfo(commandBuffer, commandBufferId), m_bufferInput.size());
				commandBuffer->EndRecording();
				m_graphicsDevice->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
				commandBuffer->Wait();
				return stopwatch.Reset();
			}

			inline float DownloadResults() {
				const Reference<Graphics::PrimaryCommandBuffer> commandBuffer = m_commandPool->CreatePrimaryCommandBuffer();
				Stopwatch stopwatch;
				commandBuffer->BeginRecording();
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
			inline BitonicSortTestCase() : BitonicSortTestCase(Jimara::Test::CreateTestGraphicsDevice()) {}
			inline bool Initialized()const { return m_log != nullptr && m_graphicsDevice != nullptr && m_shaderLibrary != nullptr && m_commandPool != nullptr; }

			inline bool Run(const std::string_view& singleStepShaderPath, const std::string_view& groupsharedShaderPath, Callback<float*, size_t> fillList) {
				if (!Initialized()) return false;
				if (!InitializeKernel(singleStepShaderPath, groupsharedShaderPath, MAX_IN_FLIGHT_BUFFERS)) return false;
				for (size_t listSize = 1u; listSize <= MAX_LIST_SIZE; listSize <<= 1) {
					if (!SetBufferInputSize(listSize)) return false;
					float totalGenerationTime = 0.0f;
					float totalGPUTime = 0.0f;
					float totalCPUTime = 0.0f;
					float totalDownloadTime = 0.0f;
					for (size_t iterationId = 0u; iterationId < ITERATION_PER_CONFIGURATION; iterationId++)
						for (size_t commandBufferId = 0; commandBufferId < MAX_IN_FLIGHT_BUFFERS; commandBufferId++) {
							totalGenerationTime += FillBuffers(fillList);
							totalGPUTime += ExecutePipeline(commandBufferId);
							totalCPUTime += SortCPUBuffer();
							totalDownloadTime += DownloadResults();
							const int result = CompareResults();
							if (result != 0) return false;
						}
					auto avgMsc = [&](float t) { return (t * 1000.0f / ITERATION_PER_CONFIGURATION / MAX_IN_FLIGHT_BUFFERS); };
					m_log->Info(
						std::fixed, std::setprecision(3),
						"Count: ", m_bufferInput.size(),
						"; Upload: ", avgMsc(totalGenerationTime), "ms (total: ", totalGenerationTime, "s)",
						"; GPU: ", avgMsc(totalGPUTime), "ms (total: ", totalGPUTime, "s)",
						"; CPU: ", avgMsc(totalCPUTime), "ms (total: ", totalCPUTime, "s)",
						"; Download: ", avgMsc(totalDownloadTime), "ms (total: ", totalDownloadTime, "s)");
				}
				return true;
			}

			template<typename FillListFn>
			inline bool Run(
				const std::string_view& singleStepShaderPath, const std::string_view& groupsharedShaderPath,
				const std::vector<size_t>& listSizes, const FillListFn fillList) {
				return Run(singleStepShaderPath, groupsharedShaderPath, listSizes, Callback<float*, size_t>::FromCall(&fillList));
			}
		};
	}

	TEST(BitonicSortTest, AlreadySorted_SingleStep) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP,
			"", FillSequentialAsc));
	}

	TEST(BitonicSortTest, RandomFloats_SingleStep) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP,
			"", FillRandom));
	}

	TEST(BitonicSortTest, AlreadySorted_WithGroupsharedStep) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP,
			BITONIC_SORT_FLOATS_GROUPSHARED, 
			FillSequentialAsc));
	}

	TEST(BitonicSortTest, RandomFloats_WithGroupsharedStep) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP,
			BITONIC_SORT_FLOATS_GROUPSHARED,
			FillRandom));
	}

	TEST(BitonicSortTest, AlreadySorted_GroupsharedOnly) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			BITONIC_SORT_FLOATS_GROUPSHARED,
			BITONIC_SORT_FLOATS_GROUPSHARED,
			FillSequentialAsc));
	}

	TEST(BitonicSortTest, RandomFloats_GroupsharedOnly) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			BITONIC_SORT_FLOATS_GROUPSHARED,
			BITONIC_SORT_FLOATS_GROUPSHARED,
			FillRandom));
	}
}
