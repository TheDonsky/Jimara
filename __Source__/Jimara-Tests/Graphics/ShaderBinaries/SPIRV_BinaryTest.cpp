#include "../../GtestHeaders.h"
#include "Graphics/Data/ShaderBinaries/SPIRV_Binary.h"
#include "OS/Logging/StreamLogger.h"


namespace Jimara {
	namespace Graphics {
		namespace {
			static const std::string TEST_SHADER_DIR = "Shaders/";
		}

		// Makes sure the test shaders are read back correctly
		TEST(SPIRV_BinaryTest, BasicReadFromFile) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			{
				Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "NoBindings.vert.spv", logger);
				ASSERT_NE(binary.operator->(), nullptr);
				EXPECT_EQ(binary->EntryPoint(), "main");
				EXPECT_EQ(binary->ShaderStages(), StageMask(PipelineStage::VERTEX));
				EXPECT_EQ(binary->BindingSetCount(), 0);
			}
			{
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
			{
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
			{
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
			{
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
			{
				Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "ThreeDescriptorSets.frag.spv", logger);
				ASSERT_NE(binary.operator->(), nullptr);
				EXPECT_EQ(binary->EntryPoint(), "main");
				EXPECT_EQ(binary->ShaderStages(), StageMask(PipelineStage::FRAGMENT));
				EXPECT_EQ(binary->BindingSetCount(), 3);
				ASSERT_EQ(binary->BindingSet(0).BindingCount(), 0);
				ASSERT_EQ(binary->BindingSet(1).BindingCount(), 4);
				ASSERT_EQ(binary->BindingSet(2).BindingCount(), 3);
			}
		}
	}
}
