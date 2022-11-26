#include "../../GtestHeaders.h"
#include "../TestEnvironmentCreation.h"
#include "Environment/Rendering/Algorithms/SegmentTree/SegmentTreeGenerationKernel.h"



namespace Jimara {
	namespace {
		namespace SegmentTree_GLH {
			using uint = uint32_t;
#include "Environment/Rendering/Algorithms/SegmentTree/SegmentTree.glh"
		}
	}

	// Checks basic functionality defined in SegmentTreeGenerationKernel and SegmentTree.glh
	TEST(SegmentTreeGenerationKernelTest, Basics) {
		const Reference<Graphics::GraphicsDevice> device = Jimara::Test::CreateTestGraphicsDevice();
		ASSERT_NE(device, nullptr);

		const Reference<Graphics::ShaderLoader> shaderLoader = Graphics::ShaderDirectoryLoader::Create("Shaders/", device->Log());
		ASSERT_NE(shaderLoader, nullptr);

		const Reference<SegmentTreeGenerationKernel> kernel = SegmentTreeGenerationKernel::CreateUintSumKernel(device, shaderLoader, 1u);
		ASSERT_NE(kernel, nullptr);

		const Reference<Graphics::PrimaryCommandBuffer> commandBuffer = device->GraphicsQueue()->CreateCommandPool()->CreatePrimaryCommandBuffer();
		ASSERT_NE(commandBuffer, nullptr);

		for (uint32_t bufferSize = 0u; bufferSize < 8192u; bufferSize++) {
			const size_t segmentTreeSize = SegmentTreeGenerationKernel::SegmentTreeBufferSize(bufferSize);
			const Graphics::ArrayBufferReference<uint32_t> buffer = device->CreateArrayBuffer<uint32_t>(segmentTreeSize, Graphics::Buffer::CPUAccess::CPU_READ_WRITE);
			ASSERT_NE(buffer, nullptr);

			// Upload data:
			{
				uint32_t* elems = buffer.Map();
				for (uint32_t i = 0; i < bufferSize; i++)
					elems[i] = i + 1u;
				for (uint32_t i = bufferSize; i < segmentTreeSize; i++)
					elems[i] = 0u;
				buffer->Unmap(true);
			}

			// Generate segment tree:
			{
				commandBuffer->BeginRecording();
				const Graphics::ArrayBufferReference<uint32_t> result = kernel->Execute({ commandBuffer, 0u }, buffer, bufferSize, true);
				commandBuffer->EndRecording();
				device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
				commandBuffer->Wait();
				if (bufferSize > 0u)
					ASSERT_EQ(buffer, result);
			}

			// Download data:
			std::vector<uint32_t> data(buffer->ObjectCount());
			{
				const uint32_t* elems = buffer.Map();
				for (size_t i = 0u; i < data.size(); i++)
					data[i] = elems[i];
				buffer->Unmap(false);
			}

			// Check integrity:
			{
				for (size_t i = 0; i < bufferSize; i++)
					ASSERT_EQ(data[i], i + 1u);
				size_t layerSize = bufferSize;
				size_t layerStart = 0u;
				size_t layerId = 0u;
				while (layerSize > 1u) {
					const size_t nextLayerSize = (layerSize + 1u) >> 1u;
					for (size_t i = 0; i < nextLayerSize; i++) {
						const size_t a = layerStart + (i << 1u);
						const size_t b = a + 1u;
						const size_t c = layerStart + layerSize + i;
						if (a >= layerSize)
							continue;
						else if (b >= layerSize) {
							if (data[a] != data[c]) {
								device->Log()->Error("Mismatch detected: [",
									"bufferSize: ", bufferSize, "; ",
									"Layer: { id:", layerId, "; size: ", layerSize, "; start: ", layerStart, "}]; ",
									"i: ", i, "; a: ", a, "(", data[a], "); c: ", c, "(", data[c], ")]");
								ASSERT_FALSE(true);
							}
						}
						else if (data[a] + data[b] != data[c]) {
							device->Log()->Error("Mismatch detected: [",
								"bufferSize: ", bufferSize, "; ",
								"Layer: { id:", layerId, "; size: ", layerSize, "; start: ", layerStart, "}]; ",
								"i: ", i, "; a: ", a, "(", data[a], "); b: ", b, "(", data[b], "); c: ", c, "(", data[c], ")]");
							ASSERT_FALSE(true);
						}
					}
					layerStart += layerSize;
					layerSize = nextLayerSize;
					layerId++;
				}
			}

			// Check queries:
			for (uint32_t start = 0u; start < bufferSize; start++)
				for (uint32_t end = start; end <= bufferSize; end++) {
					uint32_t queryResult = 0u;
					using uint = uint32_t;
					Jimara_SegmentTree_IncorporateRange(start, end, bufferSize, [&](uint32_t index) { queryResult += data[index]; });
					if (start == end) 
						ASSERT_EQ(queryResult, 0u);
					else {
						uint32_t expectedResult = (data[start] + data[end - 1u]) * (end - start) / 2;
						ASSERT_EQ(queryResult, expectedResult);
					}
				}
		}
	}
}
