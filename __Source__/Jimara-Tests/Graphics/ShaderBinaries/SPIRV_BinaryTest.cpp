#include "../../GtestHeaders.h"
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
			EXPECT_EQ(binary->BindingSet(0).Binding(0).binding, 0);
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
				
				bool Complete() { 
					return
						(noBindings != nullptr) &&
						(constantBinding != nullptr) &&
						(structuredBinding != nullptr) &&
						(samplerBinding != nullptr) &&
						(twoDescriptorSets != nullptr) &&
						(threeDescriptorSets != nullptr);
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
				
				setAddr(binaries.noBindings);
				EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
				EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
				EXPECT_EQ(bindings.size(), 0);

				setAddr(binaries.constantBinding);
				EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
				EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
				EXPECT_EQ(bindings.size(), 0);

				setAddr(binaries.structuredBinding);
				EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
				EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
				EXPECT_EQ(bindings.size(), 0);

				setAddr(binaries.samplerBinding);
				EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
				EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
				EXPECT_EQ(bindings.size(), 0);

				setAddr(binaries.twoDescriptorSets);
				EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(&addr, 1, desc, addBinding, logger));
				EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(shaderBindingsSets, addr->BindingSetCount(), desc, addBinding, logger));
				EXPECT_EQ(bindings.size(), 0);

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
			binaryList.push_back(binaries.constantBinding);
			binaryList.push_back(binaries.structuredBinding);
			binaryList.push_back(binaries.samplerBinding);
			binaryList.push_back(binaries.twoDescriptorSets);
			binaryList.push_back(binaries.threeDescriptorSets);

			ShaderResourceBindings::ShaderBindingDescription desc;
			for (size_t i = 0; i <= binaryList.size(); i++) {
				EXPECT_TRUE(ShaderResourceBindings::GenerateShaderBindings(binaryList.data(), i, desc, addBinding, logger));
				EXPECT_EQ(bindings.size(), 0);
			}
		}

		namespace {
			struct IndividualResourceBindings {
				Reference<GraphicsDevice> device;

				Reference<ShaderResourceBindings::ConstantBufferBinding> constantBuffer;			// (set=0, binding=0, stage=vert)		(constantBinding)
				Reference<ShaderResourceBindings::StructuredBufferBinding> structuredBuffer;		// (set=0, binding=1, stage=frag)		(structuredBinding)
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

				inline IndividualResourceBindings(OS::Logger* logger = nullptr) {
					if (logger == nullptr) logger = Object::Instantiate<OS::StreamLogger>();
					device = [&]() -> Reference<GraphicsDevice> {
						Reference<Application::AppInformation> appInfo = Object::Instantiate<Application::AppInformation>("SPIRV_BinaryTest", Application::AppVersion(1, 0, 0));
						Reference<GraphicsInstance> graphicsInstance = GraphicsInstance::Create(logger, appInfo);
						if (graphicsInstance->PhysicalDeviceCount() <= 0) return nullptr;
						else return graphicsInstance->GetPhysicalDevice(0)->CreateLogicalDevice();
					}();
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

					constantBuffer = makeConstantBuffer("constantBuffer");
					structuredBuffer = makeStructuredBuffer("structuredBuffer");
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

				bool Complete() {
					return
						(constantBuffer != nullptr) &&
						(structuredBuffer != nullptr) &&
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

		TEST(SPIRV_BinaryTest, GenerateShaderBindings_ConstantBuffer) {
			TestBinaries binaries;
			IndividualResourceBindings bindings;
			ASSERT_TRUE(binaries.Complete() && bindings.Complete());

			ASSERT_EQ("IMPLEMENTED", "NOT YET");
		}

		TEST(SPIRV_BinaryTest, GenerateShaderBindings_StructuredBuffer) {
			TestBinaries binaries;
			IndividualResourceBindings bindings;
			ASSERT_TRUE(binaries.Complete() && bindings.Complete());

			ASSERT_EQ("IMPLEMENTED", "NOT YET");
		}

		TEST(SPIRV_BinaryTest, GenerateShaderBindings_TextureSampler) {
			TestBinaries binaries;
			IndividualResourceBindings bindings;
			ASSERT_TRUE(binaries.Complete() && bindings.Complete());

			ASSERT_EQ("IMPLEMENTED", "NOT YET");
		}

		TEST(SPIRV_BinaryTest, GenerateShaderBindings_TwoDescriptorSets) {
			TestBinaries binaries;
			IndividualResourceBindings bindings;
			ASSERT_TRUE(binaries.Complete() && bindings.Complete());

			ASSERT_EQ("IMPLEMENTED", "NOT YET");
		}

		TEST(SPIRV_BinaryTest, GenerateShaderBindings_TwoDescriptorSets_Incomplete) {
			TestBinaries binaries;
			IndividualResourceBindings bindings;
			ASSERT_TRUE(binaries.Complete() && bindings.Complete());

			ASSERT_EQ("IMPLEMENTED", "NOT YET");
		}

		TEST(SPIRV_BinaryTest, GenerateShaderBindings_ThreeDescriptorSets) {
			TestBinaries binaries;
			IndividualResourceBindings bindings;
			ASSERT_TRUE(binaries.Complete() && bindings.Complete());

			ASSERT_EQ("IMPLEMENTED", "NOT YET");
		}

		TEST(SPIRV_BinaryTest, GenerateShaderBindings_TwoAndThreeDescriptorSets) {
			TestBinaries binaries;
			IndividualResourceBindings bindings;
			ASSERT_TRUE(binaries.Complete() && bindings.Complete());

			ASSERT_EQ("IMPLEMENTED", "NOT YET");
		}
	}
}
