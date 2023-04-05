#include "../../GtestHeaders.h"
#include "../TestEnvironmentCreation.h"
#include "Environment/Rendering/Algorithms/Random/GraphicsRNG.h"


namespace Jimara {
	// Tests the basics of GraphicsRNG
	TEST(GraphicsRNGTest, Basics) {
		const Reference<Graphics::GraphicsDevice> device = Jimara::Test::CreateTestGraphicsDevice();
		ASSERT_NE(device, nullptr);

		const Reference<Graphics::ShaderLoader> shaderLoader = Graphics::ShaderDirectoryLoader::Create("Shaders/", device->Log());
		ASSERT_NE(shaderLoader, nullptr);

		const Reference<GraphicsRNG> graphicsRNG = GraphicsRNG::GetShared(device, shaderLoader);
		ASSERT_NE(graphicsRNG, nullptr);

		// Initial buffer has to be 0 sized:
		const Graphics::ArrayBufferReference<GraphicsRNG::State> initialBuffer = (*graphicsRNG);
		{
			ASSERT_NE(initialBuffer, nullptr);
			EXPECT_EQ(initialBuffer->ObjectCount(), 0u);
			EXPECT_EQ(initialBuffer->ObjectSize(), sizeof(GraphicsRNG::State));
		}
		
		// When requested, a buffer of at least desired size should be generated, rounded up to a power of 2:
		const Graphics::ArrayBufferReference<GraphicsRNG::State> smallerBuffer = graphicsRNG->GetBuffer(1020);
		{
			ASSERT_NE(smallerBuffer, nullptr);
			EXPECT_NE(smallerBuffer, initialBuffer);
			EXPECT_EQ(smallerBuffer->ObjectCount(), 1024);
		}

		// When a bigger size is requested a new buffer of desired size should be generated:
		const constexpr size_t BUFFER_SIZE = 1u << 20u;
		const Graphics::ArrayBufferReference<GraphicsRNG::State> rngBuffer = graphicsRNG->GetBuffer(BUFFER_SIZE);
		{
			ASSERT_NE(rngBuffer, nullptr);
			EXPECT_NE(rngBuffer, initialBuffer);
			EXPECT_EQ(rngBuffer->ObjectCount(), BUFFER_SIZE);
			EXPECT_EQ(rngBuffer->ObjectSize(), sizeof(GraphicsRNG::State));
			EXPECT_EQ(rngBuffer->HostAccess(), Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
		}

		// Once a buig buffer is generated, small requests should still return the big buffer:
		{
			Graphics::ArrayBufferReference<GraphicsRNG::State> buffer = graphicsRNG->GetBuffer(120);
			EXPECT_EQ(buffer, rngBuffer);
			EXPECT_EQ(buffer->ObjectCount(), BUFFER_SIZE);
		}

		// After each test, we may need to examine the change, so we allocate a 'mirror buffer' on the CPU side:
		const Graphics::ArrayBufferReference<GraphicsRNG::State> cpuState = device->CreateArrayBuffer<GraphicsRNG::State>(BUFFER_SIZE, Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
		{
			ASSERT_NE(cpuState, nullptr);
			EXPECT_EQ(cpuState->ObjectCount(), BUFFER_SIZE);
		}

		const Reference<Graphics::PrimaryCommandBuffer> commandBuffer = device->GraphicsQueue()->CreateCommandPool()->CreatePrimaryCommandBuffer();
		ASSERT_NE(commandBuffer, nullptr);

		// Check that all states are initialized with different seeds:
		{
			commandBuffer->BeginRecording();
			cpuState->Copy(commandBuffer, rngBuffer);
			commandBuffer->EndRecording();
			device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
			commandBuffer->Wait();

			const GraphicsRNG::State* state = cpuState.Map();
			std::unordered_set<uint32_t> seedvalues;
			const constexpr size_t CHECK_SIZE = (1 << 16);
			static_assert(CHECK_SIZE <= BUFFER_SIZE);
			for (size_t i = 0; i < CHECK_SIZE; i++)
				seedvalues.insert(state[i].a);
			cpuState->Unmap(false);
			EXPECT_EQ(seedvalues.size(), CHECK_SIZE);
		}

		// Smaller and bigger buffers should be able to coexist. Therefor, one should not expect the different buffers to share state:
		{
			std::vector<GraphicsRNG::State> bigBufferStart(smallerBuffer->ObjectCount());
			std::memcpy(bigBufferStart.data(), cpuState->Map(), smallerBuffer->ObjectCount() * smallerBuffer->ObjectSize());
			cpuState->Unmap(false);
			EXPECT_EQ(std::memcmp(bigBufferStart.data(), cpuState->Map(), smallerBuffer->ObjectCount() * smallerBuffer->ObjectSize()), 0u);
			cpuState->Unmap(false);

			commandBuffer->BeginRecording();
			cpuState->Copy(commandBuffer, smallerBuffer);
			commandBuffer->EndRecording();
			device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
			commandBuffer->Wait();

			EXPECT_NE(std::memcmp(bigBufferStart.data(), cpuState->Map(), smallerBuffer->ObjectCount() * smallerBuffer->ObjectSize()), 0u);
			cpuState->Unmap(false);
		}

		// Basic pipeline descriptor for buffer generation:
		struct PipelineDescriptor
			: public virtual Graphics::ComputePipeline::Descriptor
			, public virtual Graphics::PipelineDescriptor::BindingSetDescriptor {
			Reference<Graphics::Shader> shader;
			Graphics::ArrayBufferReference<GraphicsRNG::State> rngBuffer;
			Reference<Graphics::ArrayBuffer> resultBuffer;

			// Graphics::PipelineDescriptor::BindingSetDescriptor:
			inline virtual bool SetByEnvironment()const override { return false; }

			inline virtual size_t ConstantBufferCount()const override { return 0u; }
			inline virtual BindingInfo ConstantBufferInfo(size_t)const override { return {}; }
			inline virtual Reference<Graphics::Buffer> ConstantBuffer(size_t)const override { return nullptr; }
			
			inline virtual size_t StructuredBufferCount()const override { return 2u; }
			inline virtual BindingInfo StructuredBufferInfo(size_t index)const override { return BindingInfo { Graphics::StageMask(Graphics::PipelineStage::COMPUTE), static_cast<uint32_t>(index) }; }
			inline virtual Reference<Graphics::ArrayBuffer> StructuredBuffer(size_t index)const override { return (index <= 0u) ? rngBuffer : resultBuffer; };
			
			inline virtual size_t TextureSamplerCount()const override { return 0u; }
			inline virtual BindingInfo TextureSamplerInfo(size_t)const override { return {}; }
			inline virtual Reference<Graphics::TextureSampler> Sampler(size_t)const override { return {}; }

			// Graphics::PipelineDescriptor:
			inline virtual size_t BindingSetCount()const override { return 1u; }
			inline virtual const Graphics::PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t)const override { return this; }

			// Graphics::ComputePipeline::Descriptor:
			inline virtual Reference<Graphics::Shader> ComputeShader()const override { return shader; }
			inline virtual Size3 NumBlocks()const override { return Size3((resultBuffer->ObjectCount() + 256 - 1u) / 256, 1u, 1u); }
		};
		const Reference<PipelineDescriptor> pipelineDescriptor = Object::Instantiate<PipelineDescriptor>();
		
		// Load shader:
		{
			const Reference<Graphics::ShaderCache> shaderCache = Graphics::ShaderCache::ForDevice(device);
			ASSERT_NE(shaderCache, nullptr);

			const Reference<Graphics::ShaderSet> shaderSet = shaderLoader->LoadShaderSet("");
			ASSERT_NE(shaderSet, nullptr);

			static const Graphics::ShaderClass SHADER_CLASS("Jimara-Tests/Graphics/Algorithms/GraphicsRNG_GenerateFloats");
			const Reference<Graphics::SPIRV_Binary> binary = shaderSet->GetShaderModule(&SHADER_CLASS, Graphics::PipelineStage::COMPUTE);
			ASSERT_NE(binary, nullptr);

			pipelineDescriptor->shader = shaderCache->GetShader(binary);
			ASSERT_NE(pipelineDescriptor->shader, nullptr);
		}

		// Set parameters:
		const Graphics::ArrayBufferReference<float> resultsBuffer = device->CreateArrayBuffer<float>(BUFFER_SIZE, Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
		{
			ASSERT_NE(resultsBuffer, nullptr);
			pipelineDescriptor->rngBuffer = rngBuffer;
			pipelineDescriptor->resultBuffer = resultsBuffer;
		}

		// Create pipeline:
		const Reference<Graphics::ComputePipeline> floatGenerator = device->CreateComputePipeline(pipelineDescriptor, 1u);
		ASSERT_NE(floatGenerator, nullptr);

		// Generate and check if the results are consistent with expectations:
		{
			commandBuffer->BeginRecording();
			cpuState->Copy(commandBuffer, rngBuffer);
			commandBuffer->EndRecording();
			device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
			commandBuffer->Wait();
		}

		// Store previous state:
		std::vector<GraphicsRNG::State> previousState(BUFFER_SIZE);
		std::vector<float> values(BUFFER_SIZE);
		{
			std::memcpy(previousState.data(), cpuState->Map(), BUFFER_SIZE * sizeof(GraphicsRNG::State));
			cpuState->Unmap(false);
			std::memcpy(values.data(), resultsBuffer->Map(), BUFFER_SIZE * sizeof(float));
			resultsBuffer->Unmap(false);
		}

		std::vector<float> averagePerIteration(BUFFER_SIZE);
		for (size_t it = 0; it < 128u; it++) {
			// Generate random floats:
			{
				commandBuffer->BeginRecording();
				floatGenerator->Execute({ commandBuffer, 0u });
				cpuState->Copy(commandBuffer, rngBuffer);
				commandBuffer->EndRecording();
				device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
				commandBuffer->Wait();
			}

			// Make sure the state has changes and update lists:
			{
				const GraphicsRNG::State* rngState = cpuState.Map();
				const float* currentValues = resultsBuffer.Map();
				EXPECT_NE(std::memcmp(previousState.data(), (void*)rngState, BUFFER_SIZE * sizeof(GraphicsRNG::State)), 0);
				EXPECT_NE(std::memcmp(values.data(), (void*)currentValues, BUFFER_SIZE * sizeof(float)), 0);
				std::memcpy(previousState.data(), (void*)rngState, BUFFER_SIZE * sizeof(GraphicsRNG::State));
				std::memcpy(values.data(), (void*)currentValues, BUFFER_SIZE * sizeof(float));
				cpuState->Unmap(false);
				resultsBuffer->Unmap(false);
			}

			// Check average values:
			{
				std::vector<float> percentiles(256u);
				for (size_t i = 0u; i < percentiles.size(); i++)
					percentiles[i] = 0.0f;

				float minimum, maximum, average;
				minimum = maximum = values[0];
				average = 0.0f;

				for (size_t i = 0u; i < BUFFER_SIZE; i++) {
					float value = values[i];
					if (value < minimum) minimum = value;
					if (value > maximum) maximum = value;
					average = Math::Lerp(average, value, 1.0f / ((float)i + 1.0f));
					averagePerIteration[i] = Math::Lerp(averagePerIteration[i], value, 1.0f / ((float)it + 1.0f));
					percentiles[Math::Min(static_cast<size_t>(percentiles.size() * (double)value), percentiles.size() - 1u)]++;
				}

				EXPECT_LT(std::abs(average - 0.5f), 0.1f);
				EXPECT_GE(minimum, 0.0f);
				EXPECT_LT(minimum, 0.1f);
				EXPECT_LE(maximum, 1.0f);
				EXPECT_GT(maximum, 0.9f);

				for (size_t i = 0u; i < percentiles.size(); i++)
					EXPECT_LT(std::abs((percentiles[i] / BUFFER_SIZE) - (1.0f / percentiles.size())), 0.05f);
			}
		}
		for (size_t i = 0u; i < BUFFER_SIZE; i++)
			EXPECT_LT(std::abs(averagePerIteration[i] - 0.5f), 0.15f);
	}
}
