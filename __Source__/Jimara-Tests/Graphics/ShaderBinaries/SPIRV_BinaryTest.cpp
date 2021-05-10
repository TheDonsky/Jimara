#include "../../GtestHeaders.h"
#include "../../Memory.h"
#include "Graphics/Data/ShaderBinaries/ShaderResourceBindings.h"
#include "OS/Logging/StreamLogger.h"


namespace Jimara {
	namespace Graphics {
		namespace {
			static const std::string TEST_SHADER_DIR = "Shaders/";
		}

		// Reads NoBindings.vert bytecode and makes sure, everything is OK
		TEST(SPIRV_BinaryTest, BasicReadFromFile_NoBindings) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "NoBindings.vert.spv", logger);
			ASSERT_NE(binary.operator->(), nullptr);
			EXPECT_EQ(binary->EntryPoint(), "main");
			EXPECT_EQ(binary->ShaderStages(), StageMask(PipelineStage::VERTEX));
			EXPECT_EQ(binary->BindingSetCount(), 0);
		}

		// Reads ConstantBinding.vert bytecode and makes sure, everything is OK
		TEST(SPIRV_BinaryTest, BasicReadFromFile_ConstantBinding) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "ConstantBinding.vert.spv", logger);
			ASSERT_NE(binary.operator->(), nullptr);
			EXPECT_EQ(binary->EntryPoint(), "main");
			EXPECT_EQ(binary->ShaderStages(), StageMask(PipelineStage::VERTEX));
			ASSERT_EQ(binary->BindingSetCount(), 1);
			ASSERT_EQ(binary->BindingSet(0).BindingCount(), 1);
			EXPECT_EQ(binary->BindingSet(0).Binding(0).binding, 2);
			EXPECT_EQ(binary->BindingSet(0).Binding(0).index, 0);
			EXPECT_EQ(binary->BindingSet(0).Binding(0).name, "constantBuffer");
			EXPECT_EQ(binary->BindingSet(0).Binding(0).set, 0);
			EXPECT_EQ(binary->BindingSet(0).Binding(0).type, SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER);
			const SPIRV_Binary::BindingInfo* bindingInfo = nullptr;
			EXPECT_TRUE(binary->BindingSet(0).FindBinding("constantBuffer", bindingInfo));
			EXPECT_EQ(bindingInfo, &binary->BindingSet(0).Binding(0));
			EXPECT_TRUE(binary->FindBinding("constantBuffer", bindingInfo));
			EXPECT_EQ(bindingInfo, &binary->BindingSet(0).Binding(0));
		}

		// Reads StructuredBinding.frag bytecode and makes sure, everything is OK
		TEST(SPIRV_BinaryTest, BasicReadFromFile_StructuredBinding) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "StructuredBinding.frag.spv", logger);
			ASSERT_NE(binary.operator->(), nullptr);
			EXPECT_EQ(binary->EntryPoint(), "main");
			EXPECT_EQ(binary->ShaderStages(), StageMask(PipelineStage::FRAGMENT));
			ASSERT_EQ(binary->BindingSetCount(), 1);
			ASSERT_EQ(binary->BindingSet(0).BindingCount(), 1);
			EXPECT_EQ(binary->BindingSet(0).Binding(0).binding, 1);
			EXPECT_EQ(binary->BindingSet(0).Binding(0).index, 0);
			EXPECT_EQ(binary->BindingSet(0).Binding(0).name, "structuredBuffer");
			EXPECT_EQ(binary->BindingSet(0).Binding(0).set, 0);
			EXPECT_EQ(binary->BindingSet(0).Binding(0).type, SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER);
			const SPIRV_Binary::BindingInfo* bindingInfo = nullptr;
			EXPECT_TRUE(binary->BindingSet(0).FindBinding("structuredBuffer", bindingInfo));
			EXPECT_EQ(bindingInfo, &binary->BindingSet(0).Binding(0));
			EXPECT_TRUE(binary->FindBinding("structuredBuffer", bindingInfo));
			EXPECT_EQ(bindingInfo, &binary->BindingSet(0).Binding(0));
		}

		// Reads SamplerBinding.frag bytecode and makes sure, everything is OK
		TEST(SPIRV_BinaryTest, BasicReadFromFile_SamplerBinding) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "SamplerBinding.frag.spv", logger);
			ASSERT_NE(binary.operator->(), nullptr);
			EXPECT_EQ(binary->EntryPoint(), "main");
			EXPECT_EQ(binary->ShaderStages(), StageMask(PipelineStage::FRAGMENT));
			ASSERT_EQ(binary->BindingSetCount(), 1);
			ASSERT_EQ(binary->BindingSet(0).BindingCount(), 1);
			EXPECT_EQ(binary->BindingSet(0).Binding(0).binding, 2);
			EXPECT_EQ(binary->BindingSet(0).Binding(0).index, 0);
			EXPECT_EQ(binary->BindingSet(0).Binding(0).name, "textureSampler");
			EXPECT_EQ(binary->BindingSet(0).Binding(0).set, 0);
			EXPECT_EQ(binary->BindingSet(0).Binding(0).type, SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER);
			const SPIRV_Binary::BindingInfo* bindingInfo = nullptr;
			EXPECT_TRUE(binary->BindingSet(0).FindBinding("textureSampler", bindingInfo));
			EXPECT_EQ(bindingInfo, &binary->BindingSet(0).Binding(0));
			EXPECT_TRUE(binary->FindBinding("textureSampler", bindingInfo));
			EXPECT_EQ(bindingInfo, &binary->BindingSet(0).Binding(0));
		}

		// Reads TwoDescriptorSets.vert bytecode and makes sure, everything is OK
		TEST(SPIRV_BinaryTest, BasicReadFromFile_TwoDescriptorSets) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "TwoDescriptorSets.vert.spv", logger);
			ASSERT_NE(binary.operator->(), nullptr);
			EXPECT_EQ(binary->EntryPoint(), "main");
			EXPECT_EQ(binary->ShaderStages(), StageMask(PipelineStage::VERTEX));
			ASSERT_EQ(binary->BindingSetCount(), 2);
			ASSERT_EQ(binary->BindingSet(0).BindingCount(), 4);
			ASSERT_EQ(binary->BindingSet(1).BindingCount(), 5);

			const SPIRV_Binary::BindingInfo* fromSet = nullptr;
			const SPIRV_Binary::BindingInfo* fromWholeBinary = nullptr;

			ASSERT_TRUE(binary->BindingSet(0).FindBinding("constantBuffer_0_3", fromSet));
			ASSERT_TRUE(binary->FindBinding("constantBuffer_0_3", fromWholeBinary));
			EXPECT_EQ(fromSet, fromWholeBinary);
			EXPECT_EQ(fromSet->binding, 3);
			EXPECT_EQ(fromSet->set, 0);
			EXPECT_EQ(fromSet->type, SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER);

			ASSERT_TRUE(binary->BindingSet(0).FindBinding("structuredBuffer_0_7", fromSet));
			ASSERT_TRUE(binary->FindBinding("structuredBuffer_0_7", fromWholeBinary));
			EXPECT_EQ(fromSet, fromWholeBinary);
			EXPECT_EQ(fromSet->binding, 7);
			EXPECT_EQ(fromSet->set, 0);
			EXPECT_EQ(fromSet->type, SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER);

			ASSERT_TRUE(binary->BindingSet(0).FindBinding("textureSampler_0_2", fromSet));
			ASSERT_TRUE(binary->FindBinding("textureSampler_0_2", fromWholeBinary));
			EXPECT_EQ(fromSet, fromWholeBinary);
			EXPECT_EQ(fromSet->binding, 2);
			EXPECT_EQ(fromSet->set, 0);
			EXPECT_EQ(fromSet->type, SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER);

			ASSERT_TRUE(binary->BindingSet(0).FindBinding("structuredBuffer_0_5", fromSet));
			ASSERT_TRUE(binary->FindBinding("structuredBuffer_0_5", fromWholeBinary));
			EXPECT_EQ(fromSet, fromWholeBinary);
			EXPECT_EQ(fromSet->binding, 5);
			EXPECT_EQ(fromSet->set, 0);
			EXPECT_EQ(fromSet->type, SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER);

			ASSERT_TRUE(binary->BindingSet(1).FindBinding("constantBuffer_1_5", fromSet));
			ASSERT_TRUE(binary->FindBinding("constantBuffer_1_5", fromWholeBinary));
			EXPECT_EQ(fromSet, fromWholeBinary);
			EXPECT_EQ(fromSet->binding, 5);
			EXPECT_EQ(fromSet->set, 1);
			EXPECT_EQ(fromSet->type, SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER);
			EXPECT_FALSE(binary->BindingSet(0).FindBinding("constantBuffer_1_5", fromSet));
		}

		// Reads ThreeDescriptorSets.frag bytecode and makes sure, everything is OK
		TEST(SPIRV_BinaryTest, BasicReadFromFile_ThreeDescriptorSets) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "ThreeDescriptorSets.frag.spv", logger);
			ASSERT_NE(binary.operator->(), nullptr);
			EXPECT_EQ(binary->EntryPoint(), "main");
			EXPECT_EQ(binary->ShaderStages(), StageMask(PipelineStage::FRAGMENT));
			EXPECT_EQ(binary->BindingSetCount(), 3);
			ASSERT_EQ(binary->BindingSet(0).BindingCount(), 0);
			ASSERT_EQ(binary->BindingSet(1).BindingCount(), 4);
			ASSERT_EQ(binary->BindingSet(2).BindingCount(), 3);
		}


		namespace {
			struct TestBinaries {
				const Reference<SPIRV_Binary> noBindings = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "NoBindings.vert.spv", nullptr);
				const Reference<SPIRV_Binary> constantBinding = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "ConstantBinding.vert.spv", nullptr);
				const Reference<SPIRV_Binary> structuredBinding = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "StructuredBinding.frag.spv", nullptr);
				const Reference<SPIRV_Binary> samplerBinding = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "SamplerBinding.frag.spv", nullptr);
				const Reference<SPIRV_Binary> twoDescriptorSets = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "TwoDescriptorSets.vert.spv", nullptr);
				const Reference<SPIRV_Binary> threeDescriptorSets = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "ThreeDescriptorSets.frag.spv", nullptr);
				
				bool Complete()const { 
					return
						(noBindings != nullptr) &&
						(constantBinding != nullptr) &&
						(structuredBinding != nullptr) &&
						(samplerBinding != nullptr) &&
						(twoDescriptorSets != nullptr) &&
						(threeDescriptorSets != nullptr);
				}
			};

			struct MemorySnapshot {
#ifndef NDEBUG
				const size_t initialInstanceCount;
#endif
				const size_t initialAllocation;

				inline MemorySnapshot() :
#ifndef NDEBUG
					initialInstanceCount(Object::DEBUG_ActiveInstanceCount()),
#endif
					initialAllocation(Test::Memory::HeapAllocation()) {}

				inline bool Compare() {
#ifndef NDEBUG
					if (initialInstanceCount != Object::DEBUG_ActiveInstanceCount()) return false;
#endif
					return initialAllocation == Test::Memory::HeapAllocation();
				}
			};
		}

		// Invokes GenerateShaderBindings for individual modules without any resource bindings
		TEST(SPIRV_BinaryTest, GenerateShaderBindings_NoResources) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			TestBinaries binaries;
			ASSERT_TRUE(binaries.Complete());
			
			for (size_t i = 0; i < 10; i++) {
				ShaderResourceBindings::ShaderBindingDescription desc;
				const SPIRV_Binary* addr = nullptr;
				ShaderResourceBindings::ShaderModuleBindingSet shaderBindingsSets[3];
				auto setAddr = [&](const SPIRV_Binary* value) {
					addr = value;
					for (size_t i = 0; i < addr->BindingSetCount(); i++)
						shaderBindingsSets[i] = ShaderResourceBindings::ShaderModuleBindingSet(&addr->BindingSet(i), addr->ShaderStages());
				};
				std::vector<ShaderResourceBindings::BindingSetInfo> bindings;
				auto addBinding = [&](const ShaderResourceBindings::BindingSetInfo& info) { bindings.push_back(info); };
				
				{
					setAddr(binaries.noBindings);
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
					EXPECT_EQ(bindings.size(), 0);
					MemorySnapshot snapshot;
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
					EXPECT_TRUE(snapshot.Compare());
				}

				{
					setAddr(binaries.constantBinding);
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
					EXPECT_EQ(bindings.size(), 0);
					MemorySnapshot snapshot;
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
					EXPECT_TRUE(snapshot.Compare());
				}

				{
					setAddr(binaries.structuredBinding);
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
					EXPECT_EQ(bindings.size(), 0);
					MemorySnapshot snapshot;
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
					EXPECT_TRUE(snapshot.Compare());
				}

				{
					setAddr(binaries.samplerBinding);
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
					EXPECT_EQ(bindings.size(), 0);
					MemorySnapshot snapshot;
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
					EXPECT_TRUE(snapshot.Compare());
				}

				{
					setAddr(binaries.twoDescriptorSets);
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
					EXPECT_EQ(bindings.size(), 0);
					MemorySnapshot snapshot;
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
					EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
					EXPECT_TRUE(snapshot.Compare());
				}

				// Set 0 is empty, so it, technically, should be considered complete:
				setAddr(binaries.threeDescriptorSets);
				EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
				EXPECT_EQ(bindings.size(), 1);
				EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
				EXPECT_EQ(bindings.size(), 2);
			}
		}


		// Invokes GenerateShaderBindings for combinarions of modules without any resource bindings
		TEST(SPIRV_BinaryTest, GenerateShaderBindings_NoResources_Mix) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			TestBinaries binaries;
			ASSERT_TRUE(binaries.Complete());

			std::vector<ShaderResourceBindings::BindingSetInfo> bindings;
			auto addBinding = [&](const ShaderResourceBindings::BindingSetInfo& info) { bindings.push_back(info); };

			std::vector<SPIRV_Binary*> binaryList;
			binaryList.push_back(binaries.noBindings);
			binaryList.push_back(binaries.structuredBinding);
			binaryList.push_back(binaries.samplerBinding);
			binaryList.push_back(binaries.twoDescriptorSets);
			binaryList.push_back(binaries.threeDescriptorSets);

			ShaderResourceBindings::ShaderBindingDescription desc;
			for (size_t i = 0; i <= binaryList.size(); i++) {
				EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(binaryList.data(), i, desc, addBinding, logger));
				EXPECT_EQ(bindings.size(), 0);
			}
			MemorySnapshot snapshot;
			EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(binaryList.data(), binaryList.size(), desc, addBinding, logger));
			EXPECT_TRUE(snapshot.Compare());
		}

		namespace {
			struct IndividualResourceBindings {
				Reference<GraphicsDevice> device;

				Reference<ShaderResourceBindings::StructuredBufferBinding> structuredBuffer;		// (set=0, binding=1, stage=frag)		(structuredBinding)
				Reference<ShaderResourceBindings::ConstantBufferBinding> constantBuffer;			// (set=0, binding=2, stage=vert)		(constantBinding) <conflict>
				Reference<ShaderResourceBindings::TextureSamplerBinding> textureSampler;			// (set=0, binding=2, stage=frag)		(samplerBinding) <duplicate>
				
				Reference<ShaderResourceBindings::TextureSamplerBinding> textureSampler_0_2;		// (set=0, binding=2, stage=vert)		(twoDescriptorSets) <duplicate>
				Reference<ShaderResourceBindings::ConstantBufferBinding> constantBuffer_0_3;		// (set=0, binding=3, stage=vert)		(twoDescriptorSets)
				Reference<ShaderResourceBindings::StructuredBufferBinding> structuredBuffer_0_5;	// (set=0, binding=5, stage=vert)		(twoDescriptorSets)
				Reference<ShaderResourceBindings::StructuredBufferBinding> structuredBuffer_0_7;	// (set=0, binding=7, stage=vert)		(twoDescriptorSets)

				Reference<ShaderResourceBindings::StructuredBufferBinding> structuredBuffer_1_0;	// (set=1, binding=0, stage=vert;frag)	(twoDescriptorSets;threeDescriptorSets)
				Reference<ShaderResourceBindings::TextureSamplerBinding> textureSampler_1_1;		// (set=1, binding=1, stage=vert;frag)	(twoDescriptorSets;threeDescriptorSets)
				Reference<ShaderResourceBindings::TextureSamplerBinding> textureSampler_1_2;		// (set=1, binding=2, stage=vert)		(twoDescriptorSets)
				Reference<ShaderResourceBindings::ConstantBufferBinding> constantBuffer_1_5;		// (set=1, binding=5, stage=vert)		(twoDescriptorSets)
				Reference<ShaderResourceBindings::ConstantBufferBinding> constantBuffer_1_8;		// (set=1, binding=8, stage=vert;frag)	(twoDescriptorSets;threeDescriptorSets)
				Reference<ShaderResourceBindings::TextureSamplerBinding> textureSampler_1_10;		// (set=1, binding=10, stage=frag)		(threeDescriptorSets)

				Reference<ShaderResourceBindings::TextureSamplerBinding> textureSampler_2_1;		// (set=2, binding=1, stage=frag)		(threeDescriptorSets)
				Reference<ShaderResourceBindings::ConstantBufferBinding> constantBuffer_2_3;		// (set=2, binding=3, stage=frag)		(threeDescriptorSets)
				Reference<ShaderResourceBindings::StructuredBufferBinding> structuredBuffer_2_4;	// (set=2, binding=4, stage=frag)		(threeDescriptorSets)

				inline IndividualResourceBindings(Reference<OS::Logger> logger = nullptr) {
					if (logger == nullptr) logger = Object::Instantiate<OS::StreamLogger>();
					Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("SPIRV_BinaryTest", Application::AppVersion(1, 0, 0));
					if (appInfo == nullptr) return;
					Reference<GraphicsInstance> graphicsInstance = GraphicsInstance::Create(logger, appInfo);
					if (graphicsInstance->PhysicalDeviceCount() <= 0) return;
					device = graphicsInstance->GetPhysicalDevice(0)->CreateLogicalDevice();

					if (device == nullptr) return;
					auto makeConstantBuffer = [&](const std::string& name) -> Reference<ShaderResourceBindings::ConstantBufferBinding> {
						return Object::Instantiate<ShaderResourceBindings::ConstantBufferBinding>(name, device->CreateConstantBuffer(sizeof(float)));
					};
					auto makeStructuredBuffer = [&](const std::string& name) -> Reference<ShaderResourceBindings::StructuredBufferBinding> {
						return Object::Instantiate<ShaderResourceBindings::StructuredBufferBinding>(name, device->CreateArrayBuffer(sizeof(float), 4));
					};
					auto makeTextureSampler = [&](const std::string& name) -> Reference<ShaderResourceBindings::TextureSamplerBinding> {
						return Object::Instantiate<ShaderResourceBindings::TextureSamplerBinding>(name,
							device->CreateTexture(Texture::TextureType::TEXTURE_2D, Texture::PixelFormat::B8G8R8A8_SRGB, Size3(1, 1, 1), 1, true)
							->CreateView(TextureView::ViewType::VIEW_2D)->CreateSampler());
					};

					structuredBuffer = makeStructuredBuffer("structuredBuffer");
					constantBuffer = makeConstantBuffer("constantBuffer");
					textureSampler = makeTextureSampler("textureSampler");

					textureSampler_0_2 = makeTextureSampler("textureSampler_0_2");
					constantBuffer_0_3 = makeConstantBuffer("constantBuffer_0_3");
					structuredBuffer_0_5 = makeStructuredBuffer("structuredBuffer_0_5");
					structuredBuffer_0_7 = makeStructuredBuffer("structuredBuffer_0_7");

					structuredBuffer_1_0 = makeStructuredBuffer("structuredBuffer_1_0");
					textureSampler_1_1 = makeTextureSampler("textureSampler_1_1");
					textureSampler_1_2 = makeTextureSampler("textureSampler_1_2");
					constantBuffer_1_5 = makeConstantBuffer("constantBuffer_1_5");
					constantBuffer_1_8 = makeConstantBuffer("constantBuffer_1_8");
					textureSampler_1_10 = makeTextureSampler("textureSampler_1_10");

					textureSampler_2_1 = makeTextureSampler("textureSampler_2_1");
					constantBuffer_2_3 = makeConstantBuffer("constantBuffer_2_3");
					structuredBuffer_2_4 = makeStructuredBuffer("structuredBuffer_2_4");
				}

				bool Complete()const {
					return
						(structuredBuffer != nullptr) &&
						(constantBuffer != nullptr) &&
						(textureSampler != nullptr) &&
						
						(textureSampler_0_2 != nullptr) &&
						(constantBuffer_0_3 != nullptr) &&
						(structuredBuffer_0_5 != nullptr) &&
						(structuredBuffer_0_7 != nullptr) &&
						
						(structuredBuffer_1_0 != nullptr) &&
						(textureSampler_1_1 != nullptr) &&
						(textureSampler_1_2 != nullptr) &&
						(constantBuffer_1_5 != nullptr) &&
						(constantBuffer_1_8 != nullptr) &&
						(textureSampler_1_10 != nullptr) &&

						(textureSampler_2_1 != nullptr) &&
						(constantBuffer_2_3 != nullptr) &&
						(structuredBuffer_2_4 != nullptr);
				}
			};

			struct AllBindings {
				const std::vector<const ShaderResourceBindings::ConstantBufferBinding*> constantBuffers;
				const std::vector<const ShaderResourceBindings::StructuredBufferBinding*> structuredBuffers;
				const std::vector<const ShaderResourceBindings::TextureSamplerBinding*> textureSamplers;

				inline AllBindings(const IndividualResourceBindings& bindings) 
					: constantBuffers { 
					bindings.constantBuffer, 
					bindings.constantBuffer_0_3, 
					bindings.constantBuffer_1_5, 
					bindings.constantBuffer_1_8,
					bindings.constantBuffer_2_3
				}, structuredBuffers { 
						bindings.structuredBuffer, 
						bindings.structuredBuffer_0_5, 
						bindings.structuredBuffer_0_7, 
						bindings.structuredBuffer_1_0,
						bindings.structuredBuffer_2_4
				}, textureSamplers { 
					bindings.textureSampler, 
					bindings.textureSampler_0_2, 
					bindings.textureSampler_1_1, 
					bindings.textureSampler_1_2, 
					bindings.textureSampler_1_10,
					bindings.textureSampler_2_1
				} {}

				inline operator ShaderResourceBindings::ShaderBindingDescription()const {
					ShaderResourceBindings::ShaderBindingDescription desc;

					desc.constantBufferBindings = constantBuffers.data();
					desc.constantBufferBindingCount = constantBuffers.size();
					
					desc.structuredBufferBindings = structuredBuffers.data();
					desc.structuredBufferBindingCount = structuredBuffers.size();

					desc.textureSamplerBindings = textureSamplers.data();
					desc.textureSamplerBindingCount = textureSamplers.size();

					return desc;
				}
			};
		}


		// Builds binding set descriptors from ConstantBuffer.vert and makes sure it works
		TEST(SPIRV_BinaryTest, GenerateShaderBindings_ConstantBuffer) {
			const TestBinaries binaries;
			const IndividualResourceBindings resourceBindings;
			ASSERT_TRUE(binaries.Complete() && resourceBindings.Complete());

			std::vector<ShaderResourceBindings::BindingSetInfo> bindings;
			auto addBinding = [&](const ShaderResourceBindings::BindingSetInfo& info) { bindings.push_back(info); };

			const ShaderResourceBindings::ConstantBufferBinding* const constantBuffer = resourceBindings.constantBuffer;
			ShaderResourceBindings::ShaderBindingDescription desc;
			desc.constantBufferBindings = &constantBuffer;
			desc.constantBufferBindingCount = 1;
			const SPIRV_Binary* binary = binaries.constantBinding;

			EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, desc, addBinding, nullptr));
			ASSERT_EQ(bindings.size(), 1);
			const ShaderResourceBindings::BindingSetInfo& foundBindingSet = bindings[0];
			EXPECT_EQ(foundBindingSet.setIndex, 0);
			EXPECT_EQ(foundBindingSet.set->SetByEnvironment(), false);
			ASSERT_EQ(foundBindingSet.set->ConstantBufferCount(), 1);
			EXPECT_EQ(foundBindingSet.set->StructuredBufferCount(), 0);
			EXPECT_EQ(foundBindingSet.set->TextureSamplerCount(), 0);
			const PipelineDescriptor::BindingSetDescriptor::BindingInfo bindingInfo = foundBindingSet.set->ConstantBufferInfo(0);
			EXPECT_EQ(bindingInfo.binding, 2);
			EXPECT_EQ(bindingInfo.stages, StageMask(PipelineStage::VERTEX));
			EXPECT_EQ(foundBindingSet.set->ConstantBuffer(0), resourceBindings.constantBuffer->BoundObject());
			
			bindings.clear();
			MemorySnapshot memSnapshot;
			EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, desc, addBinding, nullptr));
			bindings.clear();
			EXPECT_TRUE(memSnapshot.Compare());
		}

		// Builds binding set descriptors from StructuredBuffer.frag and makes sure it works
		TEST(SPIRV_BinaryTest, GenerateShaderBindings_StructuredBuffer) {
			const TestBinaries binaries;
			const IndividualResourceBindings resourceBindings;
			ASSERT_TRUE(binaries.Complete() && resourceBindings.Complete());

			std::vector<ShaderResourceBindings::BindingSetInfo> bindings;
			auto addBinding = [&](const ShaderResourceBindings::BindingSetInfo& info) { bindings.push_back(info); };

			const SPIRV_Binary* binary = binaries.structuredBinding;
			AllBindings allBindings(resourceBindings);
			EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, allBindings, addBinding, nullptr));
			ASSERT_EQ(bindings.size(), 1);
			const ShaderResourceBindings::BindingSetInfo& foundBindingSet = bindings[0];
			EXPECT_EQ(foundBindingSet.setIndex, 0);
			EXPECT_EQ(foundBindingSet.set->SetByEnvironment(), false);
			EXPECT_EQ(foundBindingSet.set->ConstantBufferCount(), 0);
			ASSERT_EQ(foundBindingSet.set->StructuredBufferCount(), 1);
			EXPECT_EQ(foundBindingSet.set->TextureSamplerCount(), 0);
			const PipelineDescriptor::BindingSetDescriptor::BindingInfo bindingInfo = foundBindingSet.set->StructuredBufferInfo(0);
			EXPECT_EQ(bindingInfo.binding, 1);
			EXPECT_EQ(bindingInfo.stages, StageMask(PipelineStage::FRAGMENT));
			EXPECT_EQ(foundBindingSet.set->StructuredBuffer(0), resourceBindings.structuredBuffer->BoundObject());

			bindings.clear();
			MemorySnapshot memSnapshot;
			EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, allBindings, addBinding, nullptr));
			bindings.clear();
			EXPECT_TRUE(memSnapshot.Compare());
		}

		// Builds binding set descriptors from TextureSampler.frag and makes sure it works
		TEST(SPIRV_BinaryTest, GenerateShaderBindings_TextureSampler) {
			const TestBinaries binaries;
			const IndividualResourceBindings resourceBindings;
			ASSERT_TRUE(binaries.Complete() && resourceBindings.Complete());

			std::vector<ShaderResourceBindings::BindingSetInfo> bindings;
			auto addBinding = [&](const ShaderResourceBindings::BindingSetInfo& info) { bindings.push_back(info); };

			const SPIRV_Binary* binary = binaries.samplerBinding;
			AllBindings allBindings(resourceBindings);
			EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, allBindings, addBinding, nullptr));
			ASSERT_EQ(bindings.size(), 1);
			const ShaderResourceBindings::BindingSetInfo& foundBindingSet = bindings[0];
			EXPECT_EQ(foundBindingSet.setIndex, 0);
			EXPECT_EQ(foundBindingSet.set->SetByEnvironment(), false);
			EXPECT_EQ(foundBindingSet.set->ConstantBufferCount(), 0);
			EXPECT_EQ(foundBindingSet.set->StructuredBufferCount(), 0);
			ASSERT_EQ(foundBindingSet.set->TextureSamplerCount(), 1);
			const PipelineDescriptor::BindingSetDescriptor::BindingInfo bindingInfo = foundBindingSet.set->TextureSamplerInfo(0);
			EXPECT_EQ(bindingInfo.binding, 2);
			EXPECT_EQ(bindingInfo.stages, StageMask(PipelineStage::FRAGMENT));
			EXPECT_EQ(foundBindingSet.set->Sampler(0), resourceBindings.textureSampler->BoundObject());

			bindings.clear();
			MemorySnapshot memSnapshot;
			EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, allBindings, addBinding, nullptr));
			bindings.clear();
			EXPECT_TRUE(memSnapshot.Compare());
		}

		// Builds binding set descriptors from TwoDescriptorSets.vert and makes sure it works
		TEST(SPIRV_BinaryTest, GenerateShaderBindings_TwoDescriptorSets) {
			TestBinaries binaries;
			IndividualResourceBindings resourceBindings;
			ASSERT_TRUE(binaries.Complete() && resourceBindings.Complete());

			std::vector<ShaderResourceBindings::BindingSetInfo> bindings;
			auto addBinding = [&](const ShaderResourceBindings::BindingSetInfo& info) { bindings.push_back(info); };

			const SPIRV_Binary* binary = binaries.twoDescriptorSets;
			AllBindings allBindings(resourceBindings);
			EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, allBindings, addBinding, nullptr));
			ASSERT_EQ(bindings.size(), 2);
			std::vector<Reference<PipelineDescriptor::BindingSetDescriptor>> descriptors;
			for (size_t i = 0; i < bindings.size(); i++) {
				const ShaderResourceBindings::BindingSetInfo& setInfo = bindings[i];
				if (descriptors.size() <= setInfo.setIndex) descriptors.resize(setInfo.setIndex + 1);
				descriptors[setInfo.setIndex] = setInfo.set;
			}
			{
				const PipelineDescriptor::BindingSetDescriptor* firstSet = descriptors[0];
				ASSERT_NE(firstSet, nullptr);
				EXPECT_EQ(firstSet->SetByEnvironment(), false);
				
				ASSERT_EQ(firstSet->ConstantBufferCount(), 1);
				EXPECT_EQ(firstSet->ConstantBufferInfo(0).binding, 3);
				EXPECT_EQ(firstSet->ConstantBufferInfo(0).stages, StageMask(PipelineStage::VERTEX));
				EXPECT_EQ(firstSet->ConstantBuffer(0), resourceBindings.constantBuffer_0_3->BoundObject());
				
				ASSERT_EQ(firstSet->StructuredBufferCount(), 2);
				bool structuredBuffersAnalized[2] = { false, false };
				for (size_t i = 0; i < firstSet->StructuredBufferCount(); i++) {
					const PipelineDescriptor::BindingSetDescriptor::BindingInfo bindingInfo = firstSet->StructuredBufferInfo(i);
					EXPECT_EQ(bindingInfo.stages, StageMask(PipelineStage::VERTEX));
					if (bindingInfo.binding == 5) {
						EXPECT_EQ(firstSet->StructuredBuffer(i), resourceBindings.structuredBuffer_0_5->BoundObject());
						structuredBuffersAnalized[0] = true;
					}
					else if (bindingInfo.binding == 7) {
						EXPECT_EQ(firstSet->StructuredBuffer(i), resourceBindings.structuredBuffer_0_7->BoundObject());
						structuredBuffersAnalized[1] = true;
					}
					else EXPECT_FALSE(true);
				}
				for (size_t i = 0; i < 2; i++) EXPECT_TRUE(structuredBuffersAnalized[i]);
				
				ASSERT_EQ(firstSet->TextureSamplerCount(), 1);
				EXPECT_EQ(firstSet->TextureSamplerInfo(0).binding, 2);
				EXPECT_EQ(firstSet->TextureSamplerInfo(0).stages, StageMask(PipelineStage::VERTEX));
				EXPECT_EQ(firstSet->Sampler(0), resourceBindings.textureSampler_0_2->BoundObject());
			}
			{
				const PipelineDescriptor::BindingSetDescriptor* secondSet = descriptors[1];
				ASSERT_NE(secondSet, nullptr);
				EXPECT_EQ(secondSet->SetByEnvironment(), false);

				ASSERT_EQ(secondSet->ConstantBufferCount(), 2);
				bool constantBuffersFound[2] = { false, false };
				for (size_t i = 0; i < secondSet->ConstantBufferCount(); i++) {
					const PipelineDescriptor::BindingSetDescriptor::BindingInfo bindingInfo = secondSet->ConstantBufferInfo(i);
					EXPECT_EQ(bindingInfo.stages, StageMask(PipelineStage::VERTEX));
					if (bindingInfo.binding == 5) {
						EXPECT_EQ(secondSet->ConstantBuffer(i), resourceBindings.constantBuffer_1_5->BoundObject());
						constantBuffersFound[0] = true;
					}
					else if (bindingInfo.binding == 8) {
						EXPECT_EQ(secondSet->ConstantBuffer(i), resourceBindings.constantBuffer_1_8->BoundObject());
						constantBuffersFound[1] = true;
					}
					else EXPECT_FALSE(true);
				}
				for (size_t i = 0; i < 2; i++) EXPECT_TRUE(constantBuffersFound[i]);

				ASSERT_EQ(secondSet->StructuredBufferCount(), 1);
				EXPECT_EQ(secondSet->StructuredBufferInfo(0).binding, 0);
				EXPECT_EQ(secondSet->StructuredBufferInfo(0).stages, StageMask(PipelineStage::VERTEX));
				EXPECT_EQ(secondSet->StructuredBuffer(0), resourceBindings.structuredBuffer_1_0->BoundObject());

				ASSERT_EQ(secondSet->TextureSamplerCount(), 2);
				bool textureSamplersFound[2] = { false, false };
				for (size_t i = 0; i < secondSet->TextureSamplerCount(); i++) {
					const PipelineDescriptor::BindingSetDescriptor::BindingInfo bindingInfo = secondSet->TextureSamplerInfo(i);
					EXPECT_EQ(bindingInfo.stages, StageMask(PipelineStage::VERTEX));
					if (bindingInfo.binding == 1) {
						EXPECT_EQ(secondSet->Sampler(i), resourceBindings.textureSampler_1_1->BoundObject());
						textureSamplersFound[0] = true;
					}
					else if (bindingInfo.binding == 2) {
						EXPECT_EQ(secondSet->Sampler(i), resourceBindings.textureSampler_1_2->BoundObject());
						textureSamplersFound[1] = true;
					}
					else EXPECT_FALSE(true);
				}
				for (size_t i = 0; i < 2; i++) EXPECT_TRUE(textureSamplersFound[i]);
			}

			bindings.clear();
			descriptors.clear();
			MemorySnapshot memSnapshot;
			EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, allBindings, addBinding, nullptr));
			bindings.clear();
			EXPECT_TRUE(memSnapshot.Compare());
		}

		// Builds binding set descriptors from TwoDescriptorSets.vert with partial resource bindings as well as all bindings for a single set and none for another
		TEST(SPIRV_BinaryTest, GenerateShaderBindings_TwoDescriptorSets_Incomplete) {
			TestBinaries binaries;
			IndividualResourceBindings resourceBindings;
			ASSERT_TRUE(binaries.Complete() && resourceBindings.Complete());

			const SPIRV_Binary* binary = binaries.twoDescriptorSets;
			std::vector<ShaderResourceBindings::BindingSetInfo> bindings;
			auto addBinding = [&](const ShaderResourceBindings::BindingSetInfo& info) { bindings.push_back(info); };
			
			// Collect resources only from the first binding set and make sure everything works:
			{
				bindings.clear();
				{
					ShaderResourceBindings::ShaderBindingDescription desc;
					
					const ShaderResourceBindings::ConstantBufferBinding* constantBuffer = resourceBindings.constantBuffer_0_3;
					desc.constantBufferBindings = &constantBuffer;
					desc.constantBufferBindingCount = 1;
					ASSERT_FALSE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, desc, addBinding, resourceBindings.device->Log()));

					const ShaderResourceBindings::StructuredBufferBinding* structuredBuffers[2] = { resourceBindings.structuredBuffer_0_5, resourceBindings.structuredBuffer_0_7 };
					desc.structuredBufferBindings = structuredBuffers;
					desc.structuredBufferBindingCount = 2;
					ASSERT_FALSE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, desc, addBinding, resourceBindings.device->Log()));

					const ShaderResourceBindings::TextureSamplerBinding* textureSampler = resourceBindings.textureSampler_0_2;
					desc.textureSamplerBindings = &textureSampler;
					desc.textureSamplerBindingCount = 1;

					desc.structuredBufferBindingCount = 1;
					ASSERT_FALSE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, desc, addBinding, resourceBindings.device->Log()));
					desc.structuredBufferBindingCount = 2;

					desc.constantBufferBindingCount = 0;
					ASSERT_FALSE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, desc, addBinding, resourceBindings.device->Log()));
					desc.constantBufferBindingCount = 1;

					ASSERT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, desc, addBinding, resourceBindings.device->Log()));
				}
				ASSERT_EQ(bindings.size(), 1);
				EXPECT_EQ(bindings[0].setIndex, 0);
				const PipelineDescriptor::BindingSetDescriptor* firstSet = bindings[0].set;
				ASSERT_NE(firstSet, nullptr);
				EXPECT_EQ(firstSet->SetByEnvironment(), false);

				ASSERT_EQ(firstSet->ConstantBufferCount(), 1);
				EXPECT_EQ(firstSet->ConstantBufferInfo(0).binding, 3);
				EXPECT_EQ(firstSet->ConstantBufferInfo(0).stages, StageMask(PipelineStage::VERTEX));
				EXPECT_EQ(firstSet->ConstantBuffer(0), resourceBindings.constantBuffer_0_3->BoundObject());

				ASSERT_EQ(firstSet->StructuredBufferCount(), 2);

				ASSERT_EQ(firstSet->TextureSamplerCount(), 1);
				EXPECT_EQ(firstSet->TextureSamplerInfo(0).binding, 2);
				EXPECT_EQ(firstSet->TextureSamplerInfo(0).stages, StageMask(PipelineStage::VERTEX));
				EXPECT_EQ(firstSet->Sampler(0), resourceBindings.textureSampler_0_2->BoundObject());
			}
			
			// Collect resources only from the second binding set and make sure everything works:
			{
				bindings.clear();
				{
					ShaderResourceBindings::ShaderBindingDescription desc;
					
					const ShaderResourceBindings::ConstantBufferBinding* constantBuffers[2] = { resourceBindings.constantBuffer_1_5, resourceBindings.constantBuffer_1_8 };
					desc.constantBufferBindings = constantBuffers;
					desc.constantBufferBindingCount = 2;
					ASSERT_FALSE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, desc, addBinding, resourceBindings.device->Log()));
					
					const ShaderResourceBindings::StructuredBufferBinding* structuredBuffer = resourceBindings.structuredBuffer_1_0;
					desc.structuredBufferBindings = &structuredBuffer;
					desc.structuredBufferBindingCount = 1;
					ASSERT_FALSE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, desc, addBinding, resourceBindings.device->Log()));

					const ShaderResourceBindings::TextureSamplerBinding* textureSamplers[2] = { resourceBindings.textureSampler_1_1, resourceBindings.textureSampler_1_2 };
					desc.textureSamplerBindings = textureSamplers;
					desc.textureSamplerBindingCount = 1;
					ASSERT_FALSE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, desc, addBinding, resourceBindings.device->Log()));

					desc.textureSamplerBindingCount = 2;
					ASSERT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, desc, addBinding, resourceBindings.device->Log()));
				}
				ASSERT_EQ(bindings.size(), 1);
				EXPECT_EQ(bindings[0].setIndex, 1);
				const PipelineDescriptor::BindingSetDescriptor* secondSet = bindings[0].set;
				ASSERT_NE(secondSet, nullptr);
				EXPECT_EQ(secondSet->SetByEnvironment(), false);
				ASSERT_EQ(secondSet->ConstantBufferCount(), 2);
				ASSERT_EQ(secondSet->StructuredBufferCount(), 1);
				EXPECT_EQ(secondSet->StructuredBufferInfo(0).binding, 0);
				EXPECT_EQ(secondSet->StructuredBufferInfo(0).stages, StageMask(PipelineStage::VERTEX));
				EXPECT_EQ(secondSet->StructuredBuffer(0), resourceBindings.structuredBuffer_1_0->BoundObject());
				ASSERT_EQ(secondSet->TextureSamplerCount(), 2);
			}
		}

		// Builds binding set descriptors from ThreeDescriptorSets.frag and makes sure it works
		TEST(SPIRV_BinaryTest, GenerateShaderBindings_ThreeDescriptorSets) {
			TestBinaries binaries;
			IndividualResourceBindings resourceBindings;
			ASSERT_TRUE(binaries.Complete() && resourceBindings.Complete());

			std::vector<ShaderResourceBindings::BindingSetInfo> bindings;
			auto addBinding = [&](const ShaderResourceBindings::BindingSetInfo& info) { bindings.push_back(info); };

			const SPIRV_Binary* binary = binaries.threeDescriptorSets;
			AllBindings allBindings(resourceBindings);
			EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, allBindings, addBinding, nullptr));
			ASSERT_EQ(bindings.size(), 3);
			std::vector<Reference<PipelineDescriptor::BindingSetDescriptor>> descriptors;
			for (size_t i = 0; i < bindings.size(); i++) {
				const ShaderResourceBindings::BindingSetInfo& setInfo = bindings[i];
				if (descriptors.size() <= setInfo.setIndex) descriptors.resize(setInfo.setIndex + 1);
				descriptors[setInfo.setIndex] = setInfo.set;
			}
			{
				const PipelineDescriptor::BindingSetDescriptor* firstSet = descriptors[0];
				ASSERT_NE(firstSet, nullptr);
				EXPECT_EQ(firstSet->SetByEnvironment(), false);
				EXPECT_EQ(firstSet->ConstantBufferCount(), 0);
				EXPECT_EQ(firstSet->StructuredBufferCount(), 0);
				EXPECT_EQ(firstSet->TextureSamplerCount(), 0);
			}
			{
				const PipelineDescriptor::BindingSetDescriptor* secondSet = descriptors[1];
				ASSERT_NE(secondSet, nullptr);
				EXPECT_EQ(secondSet->SetByEnvironment(), false);

				ASSERT_EQ(secondSet->ConstantBufferCount(), 1);
				EXPECT_EQ(secondSet->ConstantBufferInfo(0).binding, 8);
				EXPECT_EQ(secondSet->ConstantBufferInfo(0).stages, StageMask(PipelineStage::FRAGMENT));
				EXPECT_EQ(secondSet->ConstantBuffer(0), resourceBindings.constantBuffer_1_8->BoundObject());

				ASSERT_EQ(secondSet->StructuredBufferCount(), 1);
				EXPECT_EQ(secondSet->StructuredBufferInfo(0).binding, 0);
				EXPECT_EQ(secondSet->StructuredBufferInfo(0).stages, StageMask(PipelineStage::FRAGMENT));
				EXPECT_EQ(secondSet->StructuredBuffer(0), resourceBindings.structuredBuffer_1_0->BoundObject());

				ASSERT_EQ(secondSet->TextureSamplerCount(), 2);
			}
			{
				const PipelineDescriptor::BindingSetDescriptor* thirdSet = descriptors[2];
				ASSERT_NE(thirdSet, nullptr);
				EXPECT_EQ(thirdSet->SetByEnvironment(), false);

				ASSERT_EQ(thirdSet->ConstantBufferCount(), 1);
				EXPECT_EQ(thirdSet->ConstantBufferInfo(0).binding, 3);
				EXPECT_EQ(thirdSet->ConstantBufferInfo(0).stages, StageMask(PipelineStage::FRAGMENT));
				EXPECT_EQ(thirdSet->ConstantBuffer(0), resourceBindings.constantBuffer_2_3->BoundObject());

				ASSERT_EQ(thirdSet->StructuredBufferCount(), 1);
				EXPECT_EQ(thirdSet->StructuredBufferInfo(0).binding, 4);
				EXPECT_EQ(thirdSet->StructuredBufferInfo(0).stages, StageMask(PipelineStage::FRAGMENT));
				EXPECT_EQ(thirdSet->StructuredBuffer(0), resourceBindings.structuredBuffer_2_4->BoundObject());

				ASSERT_EQ(thirdSet->TextureSamplerCount(), 1);
				EXPECT_EQ(thirdSet->TextureSamplerInfo(0).binding, 1);
				EXPECT_EQ(thirdSet->TextureSamplerInfo(0).stages, StageMask(PipelineStage::FRAGMENT));
				EXPECT_EQ(thirdSet->Sampler(0), resourceBindings.textureSampler_2_1->BoundObject());
			}

			bindings.clear();
			descriptors.clear();
			MemorySnapshot memSnapshot;
			EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&binary, 1, allBindings, addBinding, nullptr));
			bindings.clear();
			EXPECT_TRUE(memSnapshot.Compare());
		}

		// Builds binding set descriptors from TwoDescriptorSets.vert and ThreeDescriptorSets.frag and makes sure the merger works as intended
		TEST(SPIRV_BinaryTest, GenerateShaderBindings_TwoAndThreeDescriptorSets) {
			TestBinaries binaries;
			IndividualResourceBindings resourceBindings;
			ASSERT_TRUE(binaries.Complete() && resourceBindings.Complete());

			std::vector<ShaderResourceBindings::BindingSetInfo> bindings;
			auto addBinding = [&](const ShaderResourceBindings::BindingSetInfo& info) { bindings.push_back(info); };

			const SPIRV_Binary* bytecodes[2] = { binaries.twoDescriptorSets, binaries.threeDescriptorSets };
			AllBindings allBindings(resourceBindings);
			EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(bytecodes, 2, allBindings, addBinding, nullptr));
			ASSERT_EQ(bindings.size(), 3);
			std::vector<Reference<PipelineDescriptor::BindingSetDescriptor>> descriptors;
			for (size_t i = 0; i < bindings.size(); i++) {
				const ShaderResourceBindings::BindingSetInfo& setInfo = bindings[i];
				if (descriptors.size() <= setInfo.setIndex) descriptors.resize(setInfo.setIndex + 1);
				descriptors[setInfo.setIndex] = setInfo.set;
			}
			{
				const PipelineDescriptor::BindingSetDescriptor* firstSet = descriptors[0];
				ASSERT_NE(firstSet, nullptr);
				EXPECT_EQ(firstSet->SetByEnvironment(), false);

				ASSERT_EQ(firstSet->ConstantBufferCount(), 1);
				EXPECT_EQ(firstSet->ConstantBufferInfo(0).binding, 3);
				EXPECT_EQ(firstSet->ConstantBufferInfo(0).stages, StageMask(PipelineStage::VERTEX));
				EXPECT_EQ(firstSet->ConstantBuffer(0), resourceBindings.constantBuffer_0_3->BoundObject());

				ASSERT_EQ(firstSet->StructuredBufferCount(), 2);

				ASSERT_EQ(firstSet->TextureSamplerCount(), 1);
				EXPECT_EQ(firstSet->TextureSamplerInfo(0).binding, 2);
				EXPECT_EQ(firstSet->TextureSamplerInfo(0).stages, StageMask(PipelineStage::VERTEX));
				EXPECT_EQ(firstSet->Sampler(0), resourceBindings.textureSampler_0_2->BoundObject());
			}
			{
				const PipelineDescriptor::BindingSetDescriptor* secondSet = descriptors[1];
				ASSERT_NE(secondSet, nullptr);
				EXPECT_EQ(secondSet->SetByEnvironment(), false);

				ASSERT_EQ(secondSet->ConstantBufferCount(), 2);
				bool constantBuffersFound[2] = { false, false };
				for (size_t i = 0; i < secondSet->ConstantBufferCount(); i++) {
					const PipelineDescriptor::BindingSetDescriptor::BindingInfo bindingInfo = secondSet->ConstantBufferInfo(i);
					if (bindingInfo.binding == 5) {
						EXPECT_EQ(bindingInfo.stages, StageMask(PipelineStage::VERTEX));
						EXPECT_EQ(secondSet->ConstantBuffer(i), resourceBindings.constantBuffer_1_5->BoundObject());
						constantBuffersFound[0] = true;
					}
					else if (bindingInfo.binding == 8) {
						EXPECT_EQ(bindingInfo.stages, StageMask(PipelineStage::VERTEX, PipelineStage::FRAGMENT));
						EXPECT_EQ(secondSet->ConstantBuffer(i), resourceBindings.constantBuffer_1_8->BoundObject());
						constantBuffersFound[1] = true;
					}
					else EXPECT_FALSE(true);
				}
				for (size_t i = 0; i < 2; i++) EXPECT_TRUE(constantBuffersFound[i]);

				ASSERT_EQ(secondSet->StructuredBufferCount(), 1);
				EXPECT_EQ(secondSet->StructuredBufferInfo(0).binding, 0);
				EXPECT_EQ(secondSet->StructuredBufferInfo(0).stages, StageMask(PipelineStage::VERTEX, PipelineStage::FRAGMENT));
				EXPECT_EQ(secondSet->StructuredBuffer(0), resourceBindings.structuredBuffer_1_0->BoundObject());

				ASSERT_EQ(secondSet->TextureSamplerCount(), 3);
				bool textureSamplersFound[3] = { false, false, false };
				for (size_t i = 0; i < secondSet->TextureSamplerCount(); i++) {
					const PipelineDescriptor::BindingSetDescriptor::BindingInfo bindingInfo = secondSet->TextureSamplerInfo(i);
					if (bindingInfo.binding == 1) {
						EXPECT_EQ(bindingInfo.stages, StageMask(PipelineStage::VERTEX, PipelineStage::FRAGMENT));
						EXPECT_EQ(secondSet->Sampler(i), resourceBindings.textureSampler_1_1->BoundObject());
						textureSamplersFound[0] = true;
					}
					else if (bindingInfo.binding == 2) {
						EXPECT_EQ(bindingInfo.stages, StageMask(PipelineStage::VERTEX));
						EXPECT_EQ(secondSet->Sampler(i), resourceBindings.textureSampler_1_2->BoundObject());
						textureSamplersFound[1] = true;
					}
					else if (bindingInfo.binding == 10) {
						EXPECT_EQ(bindingInfo.stages, StageMask(PipelineStage::FRAGMENT));
						EXPECT_EQ(secondSet->Sampler(i), resourceBindings.textureSampler_1_10->BoundObject());
						textureSamplersFound[2] = true;
					}
					else EXPECT_FALSE(true);
				}
				for (size_t i = 0; i < 3; i++) EXPECT_TRUE(textureSamplersFound[i]);
			}
			{
				const PipelineDescriptor::BindingSetDescriptor* thirdSet = descriptors[2];
				ASSERT_NE(thirdSet, nullptr);
				EXPECT_EQ(thirdSet->SetByEnvironment(), false);

				ASSERT_EQ(thirdSet->ConstantBufferCount(), 1);
				EXPECT_EQ(thirdSet->ConstantBufferInfo(0).binding, 3);
				EXPECT_EQ(thirdSet->ConstantBufferInfo(0).stages, StageMask(PipelineStage::FRAGMENT));
				EXPECT_EQ(thirdSet->ConstantBuffer(0), resourceBindings.constantBuffer_2_3->BoundObject());

				ASSERT_EQ(thirdSet->StructuredBufferCount(), 1);
				EXPECT_EQ(thirdSet->StructuredBufferInfo(0).binding, 4);
				EXPECT_EQ(thirdSet->StructuredBufferInfo(0).stages, StageMask(PipelineStage::FRAGMENT));
				EXPECT_EQ(thirdSet->StructuredBuffer(0), resourceBindings.structuredBuffer_2_4->BoundObject());

				ASSERT_EQ(thirdSet->TextureSamplerCount(), 1);
				EXPECT_EQ(thirdSet->TextureSamplerInfo(0).binding, 1);
				EXPECT_EQ(thirdSet->TextureSamplerInfo(0).stages, StageMask(PipelineStage::FRAGMENT));
				EXPECT_EQ(thirdSet->Sampler(0), resourceBindings.textureSampler_2_1->BoundObject());
			}
		}

		// Tests an example, when one or more resource bindings are competing for the same {set;binding} pair (should pick either one)
		TEST(SPIRV_BinaryTest, GenerateShaderBindings_Conflict_DoublyMappedBindings) {
			TestBinaries binaries;
			IndividualResourceBindings resourceBindings;
			ASSERT_TRUE(binaries.Complete() && resourceBindings.Complete());

			std::vector<ShaderResourceBindings::BindingSetInfo> bindings;
			auto addBinding = [&](const ShaderResourceBindings::BindingSetInfo& info) { bindings.push_back(info); };
			const SPIRV_Binary* bytecodes[2] = { binaries.samplerBinding, binaries.twoDescriptorSets };

			auto findDescriptor = [&]() {
				PipelineDescriptor::BindingSetDescriptor* desc = nullptr;
				for (size_t i = 0; i < bindings.size(); i++)
					if (bindings[i].setIndex == 0) {
						desc = bindings[i].set;
						break;
					}
				return desc;
			};

			// Provide both:
			{
				ASSERT_TRUE(ShaderResourceBindings::GenerateShaderBindings(bytecodes, 2, AllBindings(resourceBindings), addBinding, resourceBindings.device->Log()));
				PipelineDescriptor::BindingSetDescriptor* desc = findDescriptor();
				ASSERT_NE(desc, nullptr);
				ASSERT_EQ(desc->TextureSamplerCount(), 1);
				EXPECT_EQ(desc->TextureSamplerInfo(0).binding, 2);
				EXPECT_EQ(desc->TextureSamplerInfo(0).stages, StageMask(PipelineStage::VERTEX, PipelineStage::FRAGMENT));
				EXPECT_TRUE((desc->Sampler(0) == resourceBindings.textureSampler->BoundObject()) || (desc->Sampler(0) == resourceBindings.textureSampler_0_2->BoundObject()));
			}

			// Provide only one or the other:
			{
				ShaderResourceBindings::ShaderBindingDescription description;

				const ShaderResourceBindings::ConstantBufferBinding* constantBuffer = resourceBindings.constantBuffer_0_3;
				description.constantBufferBindings = &constantBuffer;
				description.constantBufferBindingCount = 1;
				const ShaderResourceBindings::StructuredBufferBinding* const structuredBuffers[2] = { resourceBindings.structuredBuffer_0_5, resourceBindings.structuredBuffer_0_7 };
				description.structuredBufferBindings = structuredBuffers;
				description.structuredBufferBindingCount = 2;
				const ShaderResourceBindings::TextureSamplerBinding* textureSampler = resourceBindings.textureSampler_0_2;
				description.textureSamplerBindings = &textureSampler;
				description.textureSamplerBindingCount = 1;

				{
					bindings.clear();
					ASSERT_TRUE(ShaderResourceBindings::GenerateShaderBindings(bytecodes, 2, description, addBinding, resourceBindings.device->Log()));
					PipelineDescriptor::BindingSetDescriptor* desc = findDescriptor();
					ASSERT_NE(desc, nullptr);
					ASSERT_EQ(desc->TextureSamplerCount(), 1);
					EXPECT_EQ(desc->TextureSamplerInfo(0).binding, 2);
					EXPECT_EQ(desc->TextureSamplerInfo(0).stages, StageMask(PipelineStage::VERTEX, PipelineStage::FRAGMENT));
					EXPECT_EQ(desc->Sampler(0), resourceBindings.textureSampler_0_2->BoundObject());
				}
				{
					textureSampler = resourceBindings.textureSampler;
					bindings.clear();
					ASSERT_TRUE(ShaderResourceBindings::GenerateShaderBindings(bytecodes, 2, description, addBinding, resourceBindings.device->Log()));
					PipelineDescriptor::BindingSetDescriptor* desc = findDescriptor();
					ASSERT_NE(desc, nullptr);
					ASSERT_EQ(desc->TextureSamplerCount(), 1);
					EXPECT_EQ(desc->TextureSamplerInfo(0).binding, 2);
					EXPECT_EQ(desc->TextureSamplerInfo(0).stages, StageMask(PipelineStage::VERTEX, PipelineStage::FRAGMENT));
					EXPECT_EQ(desc->Sampler(0), resourceBindings.textureSampler->BoundObject());
				}
			}
		}

		// Tests an example, when there are more than one overlapping bindings, that have a type mismatch (that renders shader modules incompatible, so the system should fail)
		TEST(SPIRV_BinaryTest, GenerateShaderBindings_Conflict_TypeMismatch) {
			TestBinaries binaries;
			IndividualResourceBindings resourceBindings;
			ASSERT_TRUE(binaries.Complete() && resourceBindings.Complete());

			std::vector<ShaderResourceBindings::BindingSetInfo> bindings;
			auto addBinding = [&](const ShaderResourceBindings::BindingSetInfo& info) { bindings.push_back(info); };

			const SPIRV_Binary* bytecodes[2] = { binaries.constantBinding, binaries.twoDescriptorSets };
			ASSERT_FALSE(ShaderResourceBindings::GenerateShaderBindings(bytecodes, 2, AllBindings(resourceBindings), addBinding, resourceBindings.device->Log()));
			
			MemorySnapshot memSnapshot;
			EXPECT_FALSE(ShaderResourceBindings::GenerateShaderBindings(bytecodes, 2, AllBindings(resourceBindings), addBinding, nullptr));
			EXPECT_TRUE(memSnapshot.Compare());
		}
	}
}
