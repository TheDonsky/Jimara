#include "../../GtestHeaders.h"
#include "Graphics/GraphicsDevice.h"
#include "Graphics/Data/ShaderBinaries/ShaderLoader.h"
#include "../../CountingLogger.h"


namespace Jimara {
	namespace Graphics {
		namespace {
			struct GraphicsAtomicsTestContext {
				Reference<Jimara::Test::CountingLogger> logger;
				std::vector<Reference<GraphicsDevice>> devices;
				Reference<ShaderSet> shaderSet;

				inline GraphicsAtomicsTestContext() {
					logger = Object::Instantiate<Jimara::Test::CountingLogger>();
					const Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("GraphicsAtomicsTest");
					const Reference<GraphicsInstance> instance = GraphicsInstance::Create(logger, appInfo);
					for (size_t i = 0u; i < instance->PhysicalDeviceCount(); i++) {
						PhysicalDevice* const physicalDevice = instance->GetPhysicalDevice(i);
						if (!physicalDevice->HasFeature(PhysicalDevice::DeviceFeature::COMPUTE)) continue;
						const Reference<GraphicsDevice> device = physicalDevice->CreateLogicalDevice();
						if (device == nullptr) continue;
						devices.push_back(device);
					}
					const Reference<ShaderLoader> shaderLoader = ShaderDirectoryLoader::Create("Shaders/", logger);
					shaderSet = shaderLoader->LoadShaderSet("");
				}
			};
		}

		TEST(GraphicsAtomicsTest, CriticalSection_SingleLock_Compute) {
			static const constexpr size_t blockSize = 128u;
			static const constexpr size_t blockCount = 64u;
			
			const GraphicsAtomicsTestContext context;
			context.logger->Info("Block Size: ", blockSize, "; Block count: ", blockCount);

			static const Graphics::ShaderClass shaderClass("Jimara-Tests/Graphics/Atomics/CriticalSection_SingleLock_Compute");
			const Reference<SPIRV_Binary> shader = context.shaderSet->GetShaderModule(&shaderClass, PipelineStage::COMPUTE);
			ASSERT_NE(shader, nullptr);
			for (size_t deviceId = 0u; deviceId < context.devices.size(); deviceId++) {
				GraphicsDevice* const device = context.devices[deviceId];
				const Reference<Graphics::ComputePipeline> pipeline = device->GetComputePipeline(shader);
				ASSERT_NE(pipeline, nullptr);
				
				const ArrayBufferReference<uint32_t> elementsBuffer = device->CreateArrayBuffer<uint32_t>(128u, Buffer::CPUAccess::CPU_WRITE_ONLY);
				{
					ASSERT_NE(elementsBuffer, nullptr);
					uint32_t* data = elementsBuffer.Map();
					for (size_t i = 0u; i < elementsBuffer->ObjectCount(); i++)
						data[i] = 0u;
					elementsBuffer->Unmap(true);
				}

				const ArrayBufferReference<uint32_t> lockBuffer = device->CreateArrayBuffer<uint32_t>(1u);
				{
					ASSERT_NE(lockBuffer, nullptr);
					lockBuffer.Map()[0] = 0u;
					lockBuffer->Unmap(true);
				}

				const ArrayBufferReference<uint32_t> cpuBuffer = [&]() -> const ArrayBufferReference<uint32_t> {
					if (elementsBuffer->HostAccess() == Buffer::CPUAccess::CPU_READ_WRITE) return cpuBuffer;
					else return device->CreateArrayBuffer<uint32_t>(elementsBuffer->ObjectCount(), Buffer::CPUAccess::CPU_READ_WRITE);
				}();
				ASSERT_NE(cpuBuffer, nullptr);
				
				const Reference<BindingPool> bindingPool = device->CreateBindingPool(1u);
				ASSERT_NE(bindingPool, nullptr);

				BindingSet::Descriptor desc = {};
				desc.pipeline = pipeline;
				const auto findBuffer = [&](const BindingSet::BindingDescriptor& info) {
					return Object::Instantiate<ResourceBinding<ArrayBuffer>>(info.name == "elements" ? elementsBuffer : lockBuffer);
				};
				desc.find.structuredBuffer = &findBuffer;
				const Reference<BindingSet> bindingSet = bindingPool->AllocateBindingSet(desc);
				ASSERT_NE(bindingSet, nullptr);
				bindingSet->Update(0u);

				const Reference<CommandPool> commandPool = device->GraphicsQueue()->CreateCommandPool();
				ASSERT_NE(commandPool, nullptr);
				const Reference<PrimaryCommandBuffer> commandBuffer = commandPool->CreatePrimaryCommandBuffer();
				ASSERT_NE(commandBuffer, nullptr);

				commandBuffer->BeginRecording();
				bindingSet->Bind(InFlightBufferInfo(commandBuffer, 0u));
				pipeline->Dispatch(commandBuffer, Size3(blockCount, 1u, 1u));
				if (cpuBuffer != elementsBuffer)
					cpuBuffer->Copy(commandBuffer, elementsBuffer);
				commandBuffer->EndRecording();
				device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
				commandBuffer->Wait();

				std::vector<uint32_t> expectedValues;
				for (size_t i = 0u; i < elementsBuffer->ObjectCount(); i++)
					expectedValues.push_back(0u);
				for (size_t c = 0u; c < (blockSize * blockCount); c++)
					for (int i = 0; i < expectedValues.size(); i++)
						expectedValues[i] = expectedValues[(i + expectedValues.size() - 1) % expectedValues.size()] + 1;

				std::stringstream stream;
				stream << "Device " << deviceId << ": ";
				const uint32_t* const cpuData = cpuBuffer.Map();
				for (size_t i = 0u; i < expectedValues.size(); i++)
					stream << "[" << expectedValues[i] << " - " << cpuData[i] << "] ";
				stream << std::endl;
				context.logger->Info(stream.str());
				EXPECT_EQ(std::memcmp((const void*)cpuData, (const void*)expectedValues.data(), sizeof(uint32_t) * expectedValues.size()), 0);
				cpuBuffer->Unmap(false);
			}
		}

		TEST(GraphicsAtomicsTest, CriticalSection_MultiLock_Compute) {
			static const constexpr size_t blockSize = 512u;
			static const constexpr size_t blockCount = 1024u;

			struct ThreadData {
				alignas(4) uint32_t lock;
				alignas(4) uint32_t value;
			};

			const GraphicsAtomicsTestContext context;
			context.logger->Info("Block Size: ", blockSize, "; Block count: ", blockCount);

			static const Graphics::ShaderClass shaderClass("Jimara-Tests/Graphics/Atomics/CriticalSection_MultiLock_Compute");
			const Reference<SPIRV_Binary> shader = context.shaderSet->GetShaderModule(&shaderClass, PipelineStage::COMPUTE);
			ASSERT_NE(shader, nullptr);
			for (size_t deviceId = 0u; deviceId < context.devices.size(); deviceId++) {
				GraphicsDevice* const device = context.devices[deviceId];
				const Reference<Graphics::ComputePipeline> pipeline = device->GetComputePipeline(shader);
				ASSERT_NE(pipeline, nullptr);

				const ArrayBufferReference<ThreadData> elementsBuffer = device->CreateArrayBuffer<ThreadData>(127u, Buffer::CPUAccess::CPU_WRITE_ONLY);
				{
					ASSERT_NE(elementsBuffer, nullptr);
					ThreadData* data = elementsBuffer.Map();
					for (size_t i = 0u; i < elementsBuffer->ObjectCount(); i++) {
						data[i].lock = 0u;
						data[i].value = static_cast<uint32_t>(i);
					}
					elementsBuffer->Unmap(true);
				}

				const ArrayBufferReference<ThreadData> cpuBuffer = [&]() -> const ArrayBufferReference<ThreadData> {
					if (elementsBuffer->HostAccess() == Buffer::CPUAccess::CPU_READ_WRITE) return cpuBuffer;
					else return device->CreateArrayBuffer<ThreadData>(elementsBuffer->ObjectCount(), Buffer::CPUAccess::CPU_READ_WRITE);
				}();
				ASSERT_NE(cpuBuffer, nullptr);

				const Reference<BindingPool> bindingPool = device->CreateBindingPool(1u);
				ASSERT_NE(bindingPool, nullptr);

				BindingSet::Descriptor desc = {};
				desc.pipeline = pipeline;
				const auto findBuffer = [&](const BindingSet::BindingDescriptor& info) {
					return Object::Instantiate<ResourceBinding<ArrayBuffer>>(elementsBuffer);
				};
				desc.find.structuredBuffer = &findBuffer;
				const Reference<BindingSet> bindingSet = bindingPool->AllocateBindingSet(desc);
				ASSERT_NE(bindingSet, nullptr);
				bindingSet->Update(0u);

				const Reference<CommandPool> commandPool = device->GraphicsQueue()->CreateCommandPool();
				ASSERT_NE(commandPool, nullptr);
				const Reference<PrimaryCommandBuffer> commandBuffer = commandPool->CreatePrimaryCommandBuffer();
				ASSERT_NE(commandBuffer, nullptr);

				commandBuffer->BeginRecording();
				bindingSet->Bind(InFlightBufferInfo(commandBuffer, 0u));
				pipeline->Dispatch(commandBuffer, Size3(blockCount, 1u, 1u));
				if (cpuBuffer != elementsBuffer)
					cpuBuffer->Copy(commandBuffer, elementsBuffer);
				commandBuffer->EndRecording();
				device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
				commandBuffer->Wait();

				std::vector<uint32_t> expectedValues;
				for (size_t i = 0u; i < elementsBuffer->ObjectCount(); i++)
					expectedValues.push_back(static_cast<uint32_t>(i));
				for (size_t blockId = 0u; blockId < blockCount; blockId++)
					for (size_t threadId = 0u; threadId < blockSize; threadId++)
					{
						size_t index = (blockId * blockSize + threadId) % expectedValues.size();
						expectedValues[index] = (expectedValues[index] + 1u) | (expectedValues[index] * 15u);
					}

				std::stringstream stream;
				stream << "Device " << deviceId << ": ";
				const ThreadData* const cpuData = cpuBuffer.Map();
				bool mismatchFound = false;
				for (size_t i = 0u; i < expectedValues.size(); i++) {
					stream << "[" << expectedValues[i] << " - " << cpuData[i].value << "] ";
					mismatchFound |= (expectedValues[i] != cpuData[i].value);
				}
				stream << std::endl;
				context.logger->Info(stream.str());
				EXPECT_FALSE(mismatchFound);
				cpuBuffer->Unmap(false);
			}
		}

		TEST(GraphicsAtomicsTest, CriticalSection_Fragment) {
			struct ThreadData {
				alignas(4) uint32_t lock;
				alignas(4) uint32_t count;
			};

			const constexpr size_t numIterations = 2u;
			const constexpr size_t numQuadRepeats = 2u;
			const constexpr size_t numInstancesPerDraw = 3u;

			static const std::vector<Rect> quads = {
				Rect(Vector2(-0.1f, -0.1f), Vector2(1.1f, 1.1f)),
				Rect(Vector2(0.5f, 0.5f), Vector2(1.1f, 1.1f)),
				Rect(Vector2(-0.1f, -0.1f), Vector2(0.5f, 0.5f))
			};

			const GraphicsAtomicsTestContext context;

			static const Graphics::ShaderClass vertexClass("Jimara-Tests/Graphics/Atomics/CriticalSection_VertexShader");
			const Reference<SPIRV_Binary> vertexShader = context.shaderSet->GetShaderModule(&vertexClass, PipelineStage::VERTEX);
			ASSERT_NE(vertexShader, nullptr);

			static const Graphics::ShaderClass singleLockFragmentClass("Jimara-Tests/Graphics/Atomics/CriticalSection_SingleLock_Fragment");
			const Reference<SPIRV_Binary> singleLockFragment = context.shaderSet->GetShaderModule(&singleLockFragmentClass, PipelineStage::FRAGMENT);
			ASSERT_NE(singleLockFragment, nullptr);

			static const Graphics::ShaderClass multiLockFragmentClass("Jimara-Tests/Graphics/Atomics/CriticalSection_MultiLock_Fragment");
			const Reference<SPIRV_Binary> multiLockFragment = context.shaderSet->GetShaderModule(&multiLockFragmentClass, PipelineStage::FRAGMENT);
			ASSERT_NE(multiLockFragment, nullptr);

			for (size_t deviceId = 0u; deviceId < context.devices.size(); deviceId++) {
				GraphicsDevice* const device = context.devices[deviceId];
				context.logger->Info("Testing on GPU ", deviceId, "...");
				
				// Create render pass:
				const Reference<RenderPass> renderPass = device->GetRenderPass(
					Texture::Multisampling::SAMPLE_COUNT_1, 0u, nullptr, Texture::PixelFormat::OTHER, RenderPass::Flags::NONE);
				ASSERT_NE(renderPass, nullptr);
				const Reference<FrameBuffer> frameBuffer = renderPass->CreateFrameBuffer(Size2(90u, 50u));
				ASSERT_NE(frameBuffer, nullptr);

				// Create pipelines:
				auto getPipeline = [&](SPIRV_Binary* shader) {
					GraphicsPipeline::Descriptor desc = {};
					desc.vertexShader = vertexShader;
					desc.fragmentShader = shader;
					GraphicsPipeline::VertexInputInfo vertexInfo = {};
					vertexInfo.bufferElementSize = sizeof(Vector2);
					vertexInfo.locations.Push(GraphicsPipeline::VertexInputInfo::LocationInfo(0, 0u));
					desc.vertexInput.Push(vertexInfo);
					return renderPass->GetGraphicsPipeline(desc);
				};
				const Reference<GraphicsPipeline> singleLockPipeline = getPipeline(singleLockFragment);
				ASSERT_NE(singleLockPipeline, nullptr);
				const Reference<GraphicsPipeline> multiLockPipeline = getPipeline(multiLockFragment);
				ASSERT_NE(multiLockPipeline, nullptr);

				// Define vertex input:
				const ArrayBufferReference<uint32_t> indexBuffer = device->CreateArrayBuffer<uint32_t>(
					quads.size() * 6u * numQuadRepeats, Buffer::CPUAccess::CPU_WRITE_ONLY);
				ASSERT_NE(indexBuffer, nullptr);
				const Reference<VertexInput> vertexInput = [&]() {
					const ArrayBufferReference<Vector2> vertexBuffer = device->CreateArrayBuffer<Vector2>(
						quads.size() * 4u * numQuadRepeats, Buffer::CPUAccess::CPU_WRITE_ONLY);
					assert(vertexBuffer != nullptr);

					Vector2* vertexPtr = vertexBuffer.Map();
					uint32_t vertexIndex = 0u;
					auto pushVertex = [&](float x, float y) {
						(*vertexPtr) = Vector2(x, y);
						vertexPtr++;
						vertexIndex++;
					};

					uint32_t* indexPtr = indexBuffer.Map();
					auto pushIndex = [&](uint32_t index) {
						(*indexPtr) = index;
						indexPtr++;
					};
					
					for (size_t i = 0u; i < numQuadRepeats; i++)
						for (size_t j = 0u; j < quads.size(); j++) {
							const Rect quad = quads[j];

							pushIndex(vertexIndex);
							pushIndex(vertexIndex + 1u);
							pushIndex(vertexIndex + 2u);

							pushIndex(vertexIndex);
							pushIndex(vertexIndex + 2u);
							pushIndex(vertexIndex + 3u);

							pushVertex(quad.start.x, quad.start.y);
							pushVertex(quad.end.x, quad.start.y);
							pushVertex(quad.end.x, quad.end.y);
							pushVertex(quad.start.x, quad.end.y);
						}

					indexBuffer->Unmap(true);
					vertexBuffer->Unmap(true);

					const Reference<ResourceBinding<ArrayBuffer>> binding = Object::Instantiate<ResourceBinding<ArrayBuffer>>(vertexBuffer);
					const Reference<ResourceBinding<ArrayBuffer>> indexBinding = Object::Instantiate<ResourceBinding<ArrayBuffer>>(indexBuffer);
					const ResourceBinding<ArrayBuffer>* bindingPtr = binding;
					return singleLockPipeline->CreateVertexInput(&bindingPtr, indexBinding);
				}();
				assert(vertexInput != nullptr);

				// Create input buffers:
				auto createZeroInitializedBuffer = [&](size_t elemSize, size_t elemCount) {
					assert((elemSize % sizeof(uint32_t)) == 0u);
					const Reference<ArrayBuffer> buffer = device->CreateArrayBuffer(elemSize, elemCount, Buffer::CPUAccess::CPU_WRITE_ONLY);
					assert(buffer != nullptr);
					uint32_t* ptr = reinterpret_cast<uint32_t*>(buffer->Map());
					uint32_t* const end = ptr + (elemCount * elemSize / sizeof(uint32_t));
					while (ptr < end) {
						(*ptr) = 0u;
						ptr++;
					}
					buffer->Unmap(true);
					return buffer;
				};
				const size_t totalPixelCount = frameBuffer->Resolution().x * frameBuffer->Resolution().y;
				const ArrayBufferReference<uint32_t> singleLockLock = createZeroInitializedBuffer(sizeof(uint32_t), 1u);
				const ArrayBufferReference<uint32_t> singleLockData = createZeroInitializedBuffer(sizeof(uint32_t), totalPixelCount);
				const ArrayBufferReference<ThreadData> multiLockBuffer = createZeroInitializedBuffer(sizeof(ThreadData), totalPixelCount);
				const ArrayBufferReference<uint32_t> singleLockCPUData = device->CreateArrayBuffer<uint32_t>(totalPixelCount, Buffer::CPUAccess::CPU_READ_WRITE);
				ASSERT_NE(singleLockCPUData, nullptr);
				const ArrayBufferReference<ThreadData> multiLockCPUBuffer = device->CreateArrayBuffer<ThreadData>(totalPixelCount, Buffer::CPUAccess::CPU_READ_WRITE);
				ASSERT_NE(multiLockCPUBuffer, nullptr);

				// Create bindings:
				const Reference<BindingPool> bindingPool = device->CreateBindingPool(1u);
				ASSERT_NE(bindingPool, nullptr);
				const Reference<BindingSet> singleLockBindings = [&]() {
					BindingSet::Descriptor desc = {};
					desc.pipeline = singleLockPipeline;
					const auto findBuffers = [&](const auto& info) {
						return Object::Instantiate<ResourceBinding<ArrayBuffer>>((info.name == "lock") ? singleLockLock : singleLockData);
					};
					desc.find.structuredBuffer = &findBuffers;
					return bindingPool->AllocateBindingSet(desc);
				}();
				ASSERT_NE(singleLockBindings, nullptr);
				singleLockBindings->Update(0u);
				const Reference<BindingSet> multiLockBindings = [&]() {
					BindingSet::Descriptor desc = {};
					desc.pipeline = multiLockPipeline;
					const auto findBuffers = [&](const auto&) {
						return Object::Instantiate<ResourceBinding<ArrayBuffer>>(multiLockBuffer);
					};
					desc.find.structuredBuffer = &findBuffers;
					return bindingPool->AllocateBindingSet(desc);
				}();
				ASSERT_NE(multiLockBindings, nullptr);
				multiLockBindings->Update(0u);

				// Create and execute command buffer:
				const Reference<CommandPool> commandPool = device->GraphicsQueue()->CreateCommandPool();
				ASSERT_NE(commandPool, nullptr);
				const Reference<PrimaryCommandBuffer> commandBuffer = commandPool->CreatePrimaryCommandBuffer();
				ASSERT_NE(commandBuffer, nullptr);
				commandBuffer->BeginRecording();
				renderPass->BeginPass(commandBuffer, frameBuffer, nullptr);
				vertexInput->Bind(commandBuffer);
				for (size_t it = 0u; it < numIterations; it++) {
					if (device->PhysicalDevice()->Type() == PhysicalDevice::DeviceType::DESCRETE) {
						// Integrated gpu seemed to time out unless we made frame buffer really small, which is not desirable for test quality...
						singleLockBindings->Bind(InFlightBufferInfo(commandBuffer));
						singleLockPipeline->Draw(commandBuffer, indexBuffer->ObjectCount(), numInstancesPerDraw);
					}
					multiLockBindings->Bind(InFlightBufferInfo(commandBuffer));
					multiLockPipeline->Draw(commandBuffer, indexBuffer->ObjectCount(), numInstancesPerDraw);
				}
				renderPass->EndPass(commandBuffer);
				singleLockCPUData->Copy(commandBuffer, singleLockData);
				multiLockCPUBuffer->Copy(commandBuffer, multiLockBuffer);
				commandBuffer->EndRecording();
				device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
				commandBuffer->Wait();

				// Make sure buffers are filled in correctly:
				std::vector<uint32_t> expectedData;
				expectedData.resize(totalPixelCount);
				const uint32_t delta = numQuadRepeats * numInstancesPerDraw * numIterations;
				for (size_t i = 0u; i < quads.size(); i++) {
					const Rect quad = quads[i];
					const Size2 pixelCount = frameBuffer->Resolution();
					auto toPixelIndex = [&](Vector2 pos) {
						return Size2(
							Math::Min(uint32_t(Math::Max(0, int(pos.x * pixelCount.x))), pixelCount.x - 1u),
							Math::Min(uint32_t(Math::Max(0, int(pos.y * pixelCount.y))), pixelCount.y - 1u));
					};
					const Size2 startPixel = toPixelIndex(quad.start);
					const Size2 endPixel = toPixelIndex(quad.end);
					for (uint32_t y = startPixel.y; y <= endPixel.y; y++)
						for (uint32_t x = startPixel.x; x <= endPixel.x; x++) {
							const uint32_t index = y * pixelCount.x + x;
							expectedData[index] += delta;
						}
				}
				if (device->PhysicalDevice()->Type() == PhysicalDevice::DeviceType::DESCRETE) {
					const uint32_t* singleDataPTR = singleLockCPUData.Map();
					size_t matchedPixelCount = 0u;
					size_t sum = 0u;
					size_t expectedSum = 0u;
					for (size_t i = 0u; i < totalPixelCount; i++) {
						if (singleDataPTR[i] == expectedData[i])
							matchedPixelCount++;
						sum += singleDataPTR[i];
						expectedSum += expectedData[i];
					}
					EXPECT_GT((float(matchedPixelCount) / totalPixelCount), 0.9f);
					const float singleLockSumFraction = (float(sum) / expectedSum);
					EXPECT_GT(singleLockSumFraction, 0.95f);
					EXPECT_LT(singleLockSumFraction, 1.05f);
					singleLockCPUData->Unmap(false);
				}
				{
					const ThreadData* multiDataPTR = multiLockCPUBuffer.Map();
					size_t matchedPixelCount = 0u;
					size_t sum = 0u;
					size_t expectedSum = 0u;
					for (size_t i = 0u; i < totalPixelCount; i++) {
						if (multiDataPTR[i].count == expectedData[i])
							matchedPixelCount++;
						sum += multiDataPTR[i].count;
						expectedSum += expectedData[i];
					}
					EXPECT_GT((float(matchedPixelCount) / totalPixelCount), 0.9f);
					const float multiLockSumFraction = (float(sum) / expectedSum);
					EXPECT_GT(multiLockSumFraction, 0.95f);
					EXPECT_LT(multiLockSumFraction, 1.05f);
					multiLockCPUBuffer->Unmap(false);
				}
			}
		}
	}
}
