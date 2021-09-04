#include "../../GtestHeaders.h"
#include "../../Memory.h"
#include "../../CountingLogger.h"
#include "Core/Stopwatch.h"
#include "Graphics/GraphicsDevice.h"
#include <random>


namespace Jimara {
	namespace Graphics {
		namespace {
			static const std::string TEST_SHADER_DIR = "Shaders/";

			inline static Reference<SPIRV_Binary> LoadBinary(OS::Logger* logger, const std::string_view& name) {
				return SPIRV_Binary::FromSPVCached(TEST_SHADER_DIR + name.data(), logger);
			}

			inline static Reference<SPIRV_Binary> LoadSumKernel(OS::Logger* logger) {
				return LoadBinary(logger, "SumKernel.comp.spv");
			}

			inline static std::vector<float> GenerateRandomNumbers(size_t count = 77773987) {
				std::vector<float> values(count);
				std::mt19937 rng(0);
				std::uniform_real_distribution<float> dis(-2.0f, 2.0f);
				for (size_t i = 0; i < values.size(); i++) values[i] = dis(rng);
				return values;
			}

			const size_t BLOCK_SIZE = 256;

			struct SumKernelDescriptor : public virtual ComputePipeline::Descriptor, public virtual PipelineDescriptor::BindingSetDescriptor {
				Reference<Shader> shader;
				Reference<Buffer> settings;
				Reference<ArrayBuffer> input;
				Reference<ArrayBuffer> output;
				size_t outputSize = 0;


				// PipelineDescriptor::BindingSetDescriptor:
				inline virtual bool SetByEnvironment()const override { return false; }

				inline virtual size_t ConstantBufferCount()const override { return 1; }
				inline virtual BindingInfo ConstantBufferInfo(size_t)const override { return { StageMask(PipelineStage::COMPUTE), 0 }; }
				inline virtual Reference<Buffer> ConstantBuffer(size_t)const override { return settings; }


				inline virtual size_t StructuredBufferCount()const override { return 2; }
				inline virtual BindingInfo StructuredBufferInfo(size_t index)const override { return { StageMask(PipelineStage::COMPUTE), uint32_t(index) + 1 }; }
				inline virtual Reference<ArrayBuffer> StructuredBuffer(size_t index)const override { return index > 0 ? output : input; }

				inline virtual size_t TextureSamplerCount()const override { return 0; }
				inline virtual BindingInfo TextureSamplerInfo(size_t)const override { return BindingInfo(); }
				inline virtual Reference<TextureSampler> Sampler(size_t)const override { return nullptr; }


				// PipelineDescriptor:
				inline virtual size_t BindingSetCount()const override { return 1; }
				inline virtual const PipelineDescriptor::BindingSetDescriptor* BindingSet(size_t)const override { return this; }

				// ComputePipeline::Descriptor:
				inline virtual Reference<Shader> ComputeShader()const override { return shader; }
				inline virtual Size3 NumBlocks() override { return Size3(outputSize, 1, 1); }
			};
		}

		TEST(ComputePipelineTest, BasicSumKernel) {
			const Reference<Jimara::Test::CountingLogger> logger = Object::Instantiate<Jimara::Test::CountingLogger>();
			const Reference<SPIRV_Binary> sumKernelBinary = LoadSumKernel(logger);
			ASSERT_NE(sumKernelBinary, nullptr);
			EXPECT_EQ(logger->NumUnsafe(), 0);
			
#ifdef _WIN32
			Jimara::Test::Memory::MemorySnapshot snapshot;
			auto updateSnapshot = [&]() { snapshot = Jimara::Test::Memory::MemorySnapshot(); };
			auto compareSnapshot = [&]() -> bool { return snapshot.Compare(); };
#else
#ifndef NDEBUG
			size_t snapshot;
			auto updateSnapshot = [&]() { snapshot = Object::DEBUG_ActiveInstanceCount(); };
			auto compareSnapshot = [&]() -> bool { return snapshot == Object::DEBUG_ActiveInstanceCount(); };
#else
			auto updateSnapshot = [&]() {};
			auto compareSnapshot = [&]() -> bool { return true; };
#endif 
#endif

			for (size_t testIt = 0; testIt < 2; testIt++) {
				updateSnapshot();
				const std::vector<float> numbers = GenerateRandomNumbers();
				float sum = 0.0f;
				{
					std::vector<float> buff = numbers;
					Stopwatch stopwatch;
					size_t s = 1;
					while (s < numbers.size()) {
						size_t ss = s << 1;
						for (size_t i = 0; (i + s) < numbers.size(); i += ss)
							buff[i] += buff[i + s];
						s = ss;
					}
					float time = stopwatch.Elapsed();
					sum = buff[0];
					logger->Info("CPU sum time [1 thread] - ", time);
				}
				Application::AppInformation info("JimaraTest", Application::AppVersion(0, 0, 1));
				for (
					GraphicsInstance::Backend backend = static_cast<GraphicsInstance::Backend>(0);
					backend < GraphicsInstance::Backend::BACKEND_OPTION_COUNT;
					backend = static_cast<GraphicsInstance::Backend>(static_cast<size_t>(backend) + 1)) {
					const Reference<GraphicsInstance> graphicsInstance = GraphicsInstance::Create(logger, &info, backend);
					ASSERT_NE(graphicsInstance, nullptr);
					for (size_t deviceId = 0; deviceId < graphicsInstance->PhysicalDeviceCount(); deviceId++) {
						PhysicalDevice* const physDevice = graphicsInstance->GetPhysicalDevice(deviceId);
						if (physDevice == nullptr) {
							logger->Warning("Backend - ", (size_t)backend, ": Physical device ", deviceId, " missing...");
							continue;
						}
						else if (physDevice->Type() != PhysicalDevice::DeviceType::DESCRETE && physDevice->Type() != PhysicalDevice::DeviceType::INTEGRATED) {
							logger->Info("Backend - ", (size_t)backend, ": Physical device ", deviceId, " <", physDevice->Name(), "> is neither discrete nor integrated, so we're gonna ignore it...");
							continue;
						}
						else if (!physDevice->HasFeature(PhysicalDevice::DeviceFeature::COMPUTE)) {
							logger->Info("Backend - ", (size_t)backend, ": Physical device ", deviceId, " <", physDevice->Name(), "> does not support compute shaders...");
							continue;
						}
						const Reference<GraphicsDevice> device = physDevice->CreateLogicalDevice();
						ASSERT_NE(device, nullptr);

						const Stopwatch stopwatch;
						const ArrayBufferReference<float> inputBuffer = device->CreateArrayBuffer<float>(numbers.size(), ArrayBuffer::CPUAccess::CPU_WRITE_ONLY);
						{
							ASSERT_NE(inputBuffer, nullptr);
							float* data = inputBuffer.Map();
							for (size_t i = 0; i < numbers.size(); i++) data[i] = numbers[i];
							inputBuffer->Unmap(true);
						}
						const float UPLOAD_TIME = stopwatch.Elapsed();

						const ArrayBufferReference<float> intermediateBuffer =
							device->CreateArrayBuffer<float>((numbers.size() + BLOCK_SIZE - 1) / BLOCK_SIZE, ArrayBuffer::CPUAccess::CPU_WRITE_ONLY);
						ASSERT_NE(intermediateBuffer, nullptr);

						const ArrayBufferReference<float> resultBuffer = device->CreateArrayBuffer<float>(1, ArrayBuffer::CPUAccess::CPU_READ_WRITE);
						ASSERT_NE(resultBuffer, nullptr);
						const float ALLOCATION_TIME = stopwatch.Elapsed() - UPLOAD_TIME;

						const Reference<SumKernelDescriptor> descriptor = Object::Instantiate<SumKernelDescriptor>();
						descriptor->shader = device->CreateShader(sumKernelBinary);
						ASSERT_NE(descriptor->shader, nullptr);
						const float SHADER_CREATION_TIME = stopwatch.Elapsed() - ALLOCATION_TIME;

						descriptor->input = inputBuffer;
						descriptor->output = intermediateBuffer;

						size_t iterationsLeft = [&]() {
							size_t inputSize = numbers.size();
							size_t count = 0;
							while (inputSize > 1) {
								inputSize = (inputSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
								count++;
							}
							return count;
						}();
						const Reference<ComputePipeline> pipeline = device->CreateComputePipeline(descriptor, iterationsLeft);
						ASSERT_NE(pipeline, nullptr);
						const float PIPELINE_CREATION_TIME = stopwatch.Elapsed() - SHADER_CREATION_TIME;

						const Reference<PrimaryCommandBuffer> commandBuffer = device->GraphicsQueue()->CreateCommandPool()->CreatePrimaryCommandBuffer();
						ASSERT_NE(commandBuffer, nullptr);

						commandBuffer->BeginRecording();
						size_t inputSize = numbers.size();
						while (inputSize > 1) {
							descriptor->outputSize = (inputSize + BLOCK_SIZE - 1) / BLOCK_SIZE;
							if (descriptor->outputSize <= 1) descriptor->output = resultBuffer;
							const BufferReference<uint32_t> settings = device->CreateConstantBuffer<uint32_t>();
							{
								ASSERT_NE(settings, nullptr);
								settings.Map() = static_cast<uint32_t>(inputSize);
								settings->Unmap(true);
								descriptor->settings = settings;
							}
							iterationsLeft--;
							pipeline->Execute(Pipeline::CommandBufferInfo(commandBuffer, iterationsLeft));
							inputSize = descriptor->outputSize;
							std::swap(descriptor->input, descriptor->output);
						}
						commandBuffer->EndRecording();
						device->GraphicsQueue()->ExecuteCommandBuffer(commandBuffer);
						commandBuffer->Wait();
						float calculatedSum = resultBuffer.Map()[0];
						resultBuffer->Unmap(false);
						EXPECT_EQ(calculatedSum, sum);

						const float EXECUTION_TIME = stopwatch.Elapsed() - PIPELINE_CREATION_TIME;
						const float TOTAL_TIME = stopwatch.Elapsed();

						logger->Info("BasicSumKernel: {\n",
							"    Input size:                 ", numbers.size(), ";\n",
							"    Upload time:                ", UPLOAD_TIME, ";\n",
							"    Additional allocation time: ", ALLOCATION_TIME, ";\n",
							"    Shader creation time:       ", SHADER_CREATION_TIME, ";\n",
							"    Pipeline creation time:     ", PIPELINE_CREATION_TIME, ";\n",
							"    Execution time:             ", EXECUTION_TIME, ";\n",
							"    Total compute time:         ", TOTAL_TIME, ";\n",
							"    Expected sum:               ", sum, ";\n",
							"    Calculated sum:             ", calculatedSum, ";\n",
							"}");
					}
				}
			}
			EXPECT_TRUE(compareSnapshot());
		}
	}
}
