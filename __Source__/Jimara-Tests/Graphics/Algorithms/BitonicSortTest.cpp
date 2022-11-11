#include "../../GtestHeaders.h"
#include "../TestEnvironmentCreation.h"
#include "Core/Stopwatch.h"
#include "Math/Random.h"
#include "Environment/Rendering/Algorithms/BitonicSort/BitonicSortKernel.h"


namespace Jimara {
	namespace {
		static const constexpr uint32_t BLOCK_SIZE = 512u;
		static const constexpr std::string_view BASE_FOLDER = "Jimara/Environment/Rendering/Algorithms/BitonicSort/";
		static const Graphics::ShaderClass BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP(((std::string)BASE_FOLDER) + "BitonicSort_Floats_SingleStep");
		static const Graphics::ShaderClass BITONIC_SORT_FLOATS_GROUPSHARED(((std::string)BASE_FOLDER) + "BitonicSort_Floats_Groupshared");
		static const constexpr size_t MAX_LIST_SIZE = (1 << 22);
		static const constexpr size_t MAX_IN_FLIGHT_BUFFERS = 2;
		static const constexpr size_t ITERATION_PER_CONFIGURATION = 4;


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
			inline bool Initialized()const { return m_log != nullptr && m_graphicsDevice != nullptr && m_shaderSet != nullptr && m_commandPool != nullptr; }

			inline bool Run(const Graphics::ShaderClass* singleStepShaderClass, const Graphics::ShaderClass* groupsharedShaderClass, Callback<float*, size_t> fillList) {
				if (!Initialized()) return false;
				if (!InitializeKernel(singleStepShaderClass, groupsharedShaderClass, MAX_IN_FLIGHT_BUFFERS)) return false;
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
				const Graphics::ShaderClass* singleStepShaderClass, const Graphics::ShaderClass* groupsharedShaderClass,
				const std::vector<size_t>& listSizes, const FillListFn fillList) {
				return Run(singleStepShaderClass, groupsharedShaderClass, listSizes, Callback<float*, size_t>::FromCall(&fillList));
			}
		};
	}

	TEST(BitonicSortTest, AlreadySorted_SingleStep) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			&BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP,
			nullptr, FillSequentialAsc));
	}

	TEST(BitonicSortTest, RandomFloats_SingleStep) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			&BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP,
			nullptr, FillRandom));
	}

	TEST(BitonicSortTest, AlreadySorted_WithGroupsharedStep) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			&BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP,
			&BITONIC_SORT_FLOATS_GROUPSHARED, 
			FillSequentialAsc));
	}

	TEST(BitonicSortTest, RandomFloats_WithGroupsharedStep) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			&BITONIC_SORT_FLOATS_POWER_OF_2_SINGLE_STEP,
			&BITONIC_SORT_FLOATS_GROUPSHARED,
			FillRandom));
	}

	TEST(BitonicSortTest, AlreadySorted_GroupsharedOnly) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			&BITONIC_SORT_FLOATS_GROUPSHARED,
			&BITONIC_SORT_FLOATS_GROUPSHARED,
			FillSequentialAsc));
	}

	TEST(BitonicSortTest, RandomFloats_GroupsharedOnly) {
		EXPECT_TRUE(BitonicSortTestCase().Run(
			&BITONIC_SORT_FLOATS_GROUPSHARED,
			&BITONIC_SORT_FLOATS_GROUPSHARED,
			FillRandom));
	}
}
