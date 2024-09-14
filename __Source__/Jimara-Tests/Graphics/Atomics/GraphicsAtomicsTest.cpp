#include "../../GtestHeaders.h"
#include "Graphics/GraphicsDevice.h"
#include "Data/ShaderLibrary.h"
#include "../../CountingLogger.h"


namespace Jimara {
	namespace Graphics {
		namespace {
			struct GraphicsAtomicsTestContext {
				Reference<Jimara::Test::CountingLogger> logger;
				std::vector<Reference<GraphicsDevice>> devices;
				Reference<ShaderLibrary> shaderLibrary;
				
				inline GraphicsAtomicsTestContext() {
					logger = Object::Instantiate<Jimara::Test::CountingLogger>();
					const Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("GraphicsAtomicsTest");
					const Reference<GraphicsInstance> instance = GraphicsInstance::Create(logger, appInfo);
					for (size_t i = 0u; i < instance->PhysicalDeviceCount(); i++) {
						PhysicalDevice* const physicalDevice = instance->GetPhysicalDevice(i);
						if (!physicalDevice->HasFeatures(PhysicalDevice::DeviceFeatures::COMPUTE)) continue;
						const Reference<GraphicsDevice> device = physicalDevice->CreateLogicalDevice();
						if (device == nullptr) continue;
						devices.push_back(device);
					}
					shaderLibrary = FileSystemShaderLibrary::Create("Shaders/", logger);
				}
			};
		}

		TEST(GraphicsAtomicsTest, CriticalSection_SingleLock_Compute) {
			static const constexpr size_t blockSize = 128u;
			static const constexpr size_t blockCount = 64u;
			
			const GraphicsAtomicsTestContext context;
			context.logger->Info("Block Size: ", blockSize, "; Block count: ", blockCount);

			static const std::string shaderPath = "Jimara-Tests/Graphics/Atomics/CriticalSection_SingleLock_Compute.comp";
			const Reference<SPIRV_Binary> shader = context.shaderLibrary->LoadShader(shaderPath);
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
					for (size_t i = 0; i < expectedValues.size(); i++)
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

			static const std::string shaderPath = "Jimara-Tests/Graphics/Atomics/CriticalSection_MultiLock_Compute.comp";
			const Reference<SPIRV_Binary> shader = context.shaderLibrary->LoadShader(shaderPath);
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
				alignas(4) uint32_t atomicCounter;
				alignas(4) uint32_t locklessCounter;
				alignas(4) uint32_t criticalCounter;
			};

			const constexpr size_t numIterations = 5u;
			const constexpr size_t numQuadRepeats = 7u;
			const constexpr size_t numInstancesPerDraw = 17u;

			static const std::vector<Rect> quads = {
				Rect(Vector2(-0.1f, -0.1f), Vector2(1.1f, 1.1f)),
				Rect(Vector2(0.5f, 0.5f), Vector2(1.1f, 1.1f)),
				Rect(Vector2(-0.1f, -0.1f), Vector2(0.5f, 0.5f)),
				Rect(Vector2(0.75f, -0.1f), Vector2(1.1f, 0.25f)),
				Rect(Vector2(-0.1f, 0.75f), Vector2(0.25f, 1.1f))
			};

			const GraphicsAtomicsTestContext context;

			static const std::string vertexPath = "Jimara-Tests/Graphics/Atomics/CriticalSection_VertexShader.vert";
			const Reference<SPIRV_Binary> vertexShader = context.shaderLibrary->LoadShader(vertexPath);
			ASSERT_NE(vertexShader, nullptr);

			static const std::string singleLockFragmentPath = "Jimara-Tests/Graphics/Atomics/CriticalSection_SingleLock_Fragment.frag";
			const Reference<SPIRV_Binary> singleLockFragment = context.shaderLibrary->LoadShader(singleLockFragmentPath);
			ASSERT_NE(singleLockFragment, nullptr);

			static const std::string multiLockFragmentPath = "Jimara-Tests/Graphics/Atomics/CriticalSection_MultiLock_Fragment.frag";
			const Reference<SPIRV_Binary> multiLockFragment = context.shaderLibrary->LoadShader(multiLockFragmentPath);
			ASSERT_NE(multiLockFragment, nullptr);

			const std::vector<std::pair<Reference<SPIRV_Binary>, std::string_view>> fragmentShaders = { 
				{ singleLockFragment, "Single Lock" }, 
				{ multiLockFragment, "Multi-Lock"} 
			};

			for (size_t deviceId = 0u; deviceId < context.devices.size(); deviceId++) {
				GraphicsDevice* const device = context.devices[deviceId];
				if (device->PhysicalDevice()->HasFeatures(PhysicalDevice::DeviceFeatures::FRAGMENT_SHADER_INTERLOCK)) {
					context.logger->Info("Skipping GPU ", deviceId, " because it supports fragment interlock...");
					continue;
				}
				context.logger->Info("Testing on GPU ", deviceId, "...");
				
				// Create shared resources:
				const Reference<RenderPass> renderPass = device->GetRenderPass(
					Texture::Multisampling::SAMPLE_COUNT_1, 0u, nullptr, Texture::PixelFormat::OTHER, RenderPass::Flags::NONE);
				ASSERT_NE(renderPass, nullptr);
				const Reference<FrameBuffer> frameBuffer = renderPass->CreateFrameBuffer(Size2(23u, 17u));
				ASSERT_NE(frameBuffer, nullptr);
				const size_t totalPixelCount = size_t(frameBuffer->Resolution().x) * frameBuffer->Resolution().y;
				const Reference<BindingPool> bindingPool = device->CreateBindingPool(1u);
				ASSERT_NE(bindingPool, nullptr);
				const Reference<CommandPool> commandPool = device->GraphicsQueue()->CreateCommandPool();
				ASSERT_NE(commandPool, nullptr);

				// Create zero-initialed buffer:
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

				// Define vertex input:
				const ArrayBufferReference<uint32_t> indexBuffer = device->CreateArrayBuffer<uint32_t>(
					quads.size() * 6u * numQuadRepeats, Buffer::CPUAccess::CPU_WRITE_ONLY);
				ASSERT_NE(indexBuffer, nullptr);
				auto createVertexInput = [&](GraphicsPipeline* pipeline) {
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
					return pipeline->CreateVertexInput(&bindingPtr, indexBinding);
				};
				
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

				// Test each pipeline:
				for (size_t shaderId = 0u; shaderId < fragmentShaders.size(); shaderId++) {
					context.logger->Info(fragmentShaders[shaderId].second, "...");
					const Reference<GraphicsPipeline> pipeline = getPipeline(fragmentShaders[shaderId].first);
					ASSERT_NE(pipeline, nullptr);

					const Reference<VertexInput> vertexInput = createVertexInput(pipeline);
					ASSERT_NE(vertexInput, nullptr);

					const ArrayBufferReference<ThreadData> buffer = createZeroInitializedBuffer(sizeof(ThreadData), totalPixelCount);
					ASSERT_NE(buffer, nullptr);
					const ArrayBufferReference<ThreadData> cpuBuffer = device->CreateArrayBuffer<ThreadData>(totalPixelCount, Buffer::CPUAccess::CPU_READ_WRITE);
					ASSERT_NE(cpuBuffer, nullptr);

					const Reference<BindingSet> bindings = [&]() {
						BindingSet::Descriptor desc = {};
						desc.pipeline = pipeline;
						const auto findBuffers = [&](const auto&) {
							return Object::Instantiate<ResourceBinding<ArrayBuffer>>(buffer);
						};
						desc.find.structuredBuffer = &findBuffers;
						return bindingPool->AllocateBindingSet(desc);
					}();
					ASSERT_NE(bindings, nullptr);
					bindings->Update(0u);

					const Reference<PrimaryCommandBuffer> commandBuffer = commandPool->CreatePrimaryCommandBuffer();
					ASSERT_NE(commandBuffer, nullptr);
					commandBuffer->BeginRecording();
					renderPass->BeginPass(commandBuffer, frameBuffer, nullptr);
					vertexInput->Bind(commandBuffer);
					for (size_t it = 0u; it < numIterations; it++) {
						bindings->Bind(InFlightBufferInfo(commandBuffer));
						pipeline->Draw(commandBuffer, indexBuffer->ObjectCount(), numInstancesPerDraw);
					}
					renderPass->EndPass(commandBuffer);
					cpuBuffer->Copy(commandBuffer, buffer);
					commandBuffer->EndRecording();
					device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
					commandBuffer->Wait();

					size_t atomicCounterSum = 0u;
					size_t locklessCounterSum = 0u;
					size_t criticalCounterSum = 0u;
					size_t zeroLockCount = 0u;
					size_t nonzeroCount = 0u;
					size_t matchCount = 0u;

					const ThreadData* ptr = cpuBuffer.Map();
					const ThreadData* const end = ptr + cpuBuffer->ObjectCount();
					while (ptr < end) {
						atomicCounterSum += ptr->atomicCounter;
						locklessCounterSum += ptr->locklessCounter;
						criticalCounterSum += ptr->criticalCounter;
						if (ptr->lock == 0u) zeroLockCount++;
						if (ptr->criticalCounter != 0u) nonzeroCount++;
						if (ptr->atomicCounter == ptr->criticalCounter) matchCount++;
						ptr++;
						EXPECT_EQ(ptr->atomicCounter, ptr->criticalCounter);
					}
					cpuBuffer->Unmap(false);

					context.logger->Info("Stats: \n", 
						"    atomicCounterSum: ", atomicCounterSum, "\n",
						"    locklessCounterSum: ", locklessCounterSum, "\n",
						"    criticalCounterSum: ", criticalCounterSum, "\n",
						"    zeroLockCount: ", zeroLockCount, "\n",
						"    nonzeroCount: ", nonzeroCount, "\n",
						"    matchCount: ", matchCount, "\n");

					EXPECT_EQ(atomicCounterSum, criticalCounterSum);
					EXPECT_EQ(zeroLockCount, cpuBuffer->ObjectCount());
					EXPECT_EQ(matchCount, cpuBuffer->ObjectCount());
				}
			}
		}
	}
}
