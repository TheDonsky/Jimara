#include "../../GtestHeaders.h"
#include "../TestEnvironmentCreation.h"
#include "Environment/Rendering/Algorithms/Random/GraphicsRNG.h"


namespace Jimara {
	namespace {

	}

	// Tests the basics of GraphicsRNG
	TEST(GraphicsRNGTest, Basics) {
		const Reference<Graphics::GraphicsDevice> device = Jimara::Test::CreateTestGraphicsDevice();
		ASSERT_NE(device, nullptr);

		const Reference<Graphics::ShaderLoader> shaderLoader = Graphics::ShaderDirectoryLoader::Create("Shaders/", device->Log());
		ASSERT_NE(shaderLoader, nullptr);

		const Reference<Graphics::PrimaryCommandBuffer> commandBuffer = device->GraphicsQueue()->CreateCommandPool()->CreatePrimaryCommandBuffer();
		ASSERT_NE(commandBuffer, nullptr);

		const Reference<GraphicsRNG> graphicsRNG = GraphicsRNG::GetShared(device, shaderLoader);
		ASSERT_NE(graphicsRNG, nullptr);

		// Initial buffer has to be 0 sized:
		const Graphics::ArrayBufferReference<GraphicsRNG::State> initialBuffer = (*graphicsRNG);
		{
			ASSERT_NE(initialBuffer, nullptr);
			EXPECT_EQ(initialBuffer->ObjectCount(), 0u);
			EXPECT_EQ(initialBuffer->ObjectSize(), sizeof(GraphicsRNG::State));
		}

		// When requested, a buffer of desired size should be generated:
		const constexpr size_t BUFFER_SIZE = 1u << 20u;
		const Graphics::ArrayBufferReference<GraphicsRNG::State> rngBuffer = graphicsRNG->GetBuffer(BUFFER_SIZE);
		{
			ASSERT_NE(rngBuffer, nullptr);
			EXPECT_NE(rngBuffer, initialBuffer);
			EXPECT_EQ(rngBuffer->ObjectCount(), BUFFER_SIZE);
			EXPECT_EQ(rngBuffer->ObjectSize(), sizeof(GraphicsRNG::State));
			EXPECT_EQ(rngBuffer->HostAccess(), Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
		}

		// After each test, we may need to examine the change, so we allocate a 'mirror buffer' on the CPU side:
		const Graphics::ArrayBufferReference<GraphicsRNG::State> cpuState = device->CreateArrayBuffer<GraphicsRNG::State>(BUFFER_SIZE, Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
		{
			ASSERT_NE(cpuState, nullptr);
			EXPECT_EQ(cpuState->ObjectCount(), BUFFER_SIZE);
		}

		// Check that all states are initialized with different seeds:
		{
			commandBuffer->BeginRecording();
			cpuState->Copy(commandBuffer, rngBuffer);
			commandBuffer->EndRecording();
			device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
			commandBuffer->Wait();

			const GraphicsRNG::State* state = cpuState.Map();
			std::unordered_set<uint32_t> seedvalues;
			for (size_t i = 0; i < BUFFER_SIZE; i++)
				seedvalues.insert(state[i].a);
			cpuState->Unmap(false);
			EXPECT_EQ(seedvalues.size(), BUFFER_SIZE);
		}

		// __TODO__: Implement this crap!
		ASSERT_FALSE(true);
	}
}
