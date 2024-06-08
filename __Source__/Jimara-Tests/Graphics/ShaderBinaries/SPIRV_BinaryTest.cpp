#include "../../GtestHeaders.h"
#include "../../Memory.h"
#include "Graphics/Data/ShaderBinaries/ShaderSet.h"
#include "OS/Logging/StreamLogger.h"


namespace Jimara {
	namespace Graphics {
		namespace {
			static const std::string TEST_SHADER_DIR = "Shaders/47DEQpj8HBSa-_TImW-5JCeuQeRkm5NMpJWZG3hSuFU/Jimara-Tests/Graphics/ShaderBinaries/Shaders/";
		}

		// Reads NoBindings.vert bytecode and makes sure, everything is OK
		TEST(SPIRV_BinaryTest, BasicReadFromFile_NoBindings) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "NoBindings.vert.spv", logger);
			ASSERT_NE(binary.operator->(), nullptr);
			EXPECT_EQ(binary->EntryPoint(), "main");
			EXPECT_EQ(binary->ShaderStages(), PipelineStage::VERTEX);
			EXPECT_EQ(binary->BindingSetCount(), 0);
		}

		// Reads ConstantBinding.vert bytecode and makes sure, everything is OK
		TEST(SPIRV_BinaryTest, BasicReadFromFile_ConstantBinding) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "ConstantBinding.vert.spv", logger);
			ASSERT_NE(binary.operator->(), nullptr);
			EXPECT_EQ(binary->EntryPoint(), "main");
			EXPECT_EQ(binary->ShaderStages(), PipelineStage::VERTEX);
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
			EXPECT_EQ(binary->ShaderStages(), PipelineStage::FRAGMENT);
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
			EXPECT_EQ(binary->ShaderStages(), PipelineStage::FRAGMENT);
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
			EXPECT_EQ(binary->ShaderStages(), PipelineStage::VERTEX);
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
			EXPECT_EQ(binary->ShaderStages(), PipelineStage::FRAGMENT);
			EXPECT_EQ(binary->BindingSetCount(), 3);
			ASSERT_EQ(binary->BindingSet(0).BindingCount(), 0);
			ASSERT_EQ(binary->BindingSet(1).BindingCount(), 4);
			ASSERT_EQ(binary->BindingSet(2).BindingCount(), 3);
		}

		// Reads BindlessSets.vert bytecode and makes sure, everything is OK
		TEST(SPIRV_BinaryTest, BasicReadFromFile_BindlessSets) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "BindlessSets.vert.spv", logger);
			ASSERT_NE(binary.operator->(), nullptr);
			EXPECT_EQ(binary->EntryPoint(), "main");
			EXPECT_EQ(binary->ShaderStages(), PipelineStage::VERTEX);
			EXPECT_EQ(binary->BindingSetCount(), 3);
			{
				ASSERT_EQ(binary->BindingSet(0).BindingCount(), 1);
				EXPECT_EQ(binary->BindingSet(0).Binding(0).type, SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER_ARRAY);
				EXPECT_EQ(binary->BindingSet(0).Binding(0).binding, 0);
			}
			{
				ASSERT_EQ(binary->BindingSet(1).BindingCount(), 2);
				EXPECT_EQ(binary->BindingSet(1).Binding(0).type, SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER_ARRAY);
				EXPECT_EQ(binary->BindingSet(1).Binding(0).binding, 0);
				EXPECT_EQ(binary->BindingSet(1).Binding(1).type, SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER_ARRAY);
				EXPECT_EQ(binary->BindingSet(1).Binding(1).binding, 0);
			}
			{
				ASSERT_EQ(binary->BindingSet(2).BindingCount(), 1);
				EXPECT_EQ(binary->BindingSet(2).Binding(0).type, SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER_ARRAY);
				EXPECT_EQ(binary->BindingSet(2).Binding(0).binding, 1);
			}
		}

		// Reads BindlessSets.vert bytecode and makes sure, everything is OK
		TEST(SPIRV_BinaryTest, BasicReadFromFile_VertexInputs) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPV(TEST_SHADER_DIR + "VertexInput.vert.spv", logger);
			ASSERT_NE(binary.operator->(), nullptr);
			EXPECT_EQ(binary->EntryPoint(), "main");
			EXPECT_EQ(binary->ShaderStages(), PipelineStage::VERTEX);
			EXPECT_EQ(binary->BindingSetCount(), 0u);
			EXPECT_EQ(binary->ShaderInputCount(), 17u);
			const SPIRV_Binary::ShaderInputInfo* info = nullptr;
#define BasicReadFromFile_VertexInputs_CHECK(inputName, inputFormat, locationSlot) { \
				ASSERT_TRUE(binary->FindShaderInput(inputName, info)); \
				ASSERT_NE(info, nullptr); \
				ASSERT_EQ(info->name, inputName); \
				EXPECT_EQ(info->format, inputFormat); \
				EXPECT_EQ(info->location, locationSlot); \
				ASSERT_LT(info->index, binary->ShaderInputCount()); \
				ASSERT_EQ(info, &binary->ShaderInput(info->index)); \
			}

			BasicReadFromFile_VertexInputs_CHECK("floatInput0", SPIRV_Binary::ShaderInputInfo::Type::FLOAT, 0);
			BasicReadFromFile_VertexInputs_CHECK("floatInput1", SPIRV_Binary::ShaderInputInfo::Type::FLOAT, 1);
			BasicReadFromFile_VertexInputs_CHECK("vec2Input", SPIRV_Binary::ShaderInputInfo::Type::FLOAT2, 4);
			BasicReadFromFile_VertexInputs_CHECK("vec3Input", SPIRV_Binary::ShaderInputInfo::Type::FLOAT3, 5);
			BasicReadFromFile_VertexInputs_CHECK("vec4Input0", SPIRV_Binary::ShaderInputInfo::Type::FLOAT4, 6);
			BasicReadFromFile_VertexInputs_CHECK("vec4Input1", SPIRV_Binary::ShaderInputInfo::Type::FLOAT4, 7);

			BasicReadFromFile_VertexInputs_CHECK("intInput", SPIRV_Binary::ShaderInputInfo::Type::INT, 8);
			BasicReadFromFile_VertexInputs_CHECK("ivec2Input", SPIRV_Binary::ShaderInputInfo::Type::INT2, 9);
			BasicReadFromFile_VertexInputs_CHECK("ivec3Input", SPIRV_Binary::ShaderInputInfo::Type::INT3, 10);
			BasicReadFromFile_VertexInputs_CHECK("ivec4Input", SPIRV_Binary::ShaderInputInfo::Type::INT4, 11);

			BasicReadFromFile_VertexInputs_CHECK("uintInput", SPIRV_Binary::ShaderInputInfo::Type::UINT, 16);
			BasicReadFromFile_VertexInputs_CHECK("uvec2Input", SPIRV_Binary::ShaderInputInfo::Type::UINT2, 17);
			BasicReadFromFile_VertexInputs_CHECK("uvec3Input", SPIRV_Binary::ShaderInputInfo::Type::UINT3, 18);
			BasicReadFromFile_VertexInputs_CHECK("uvec4Input", SPIRV_Binary::ShaderInputInfo::Type::UINT4, 19);

			// Bool was not allowed....

			BasicReadFromFile_VertexInputs_CHECK("mat2Input", SPIRV_Binary::ShaderInputInfo::Type::MAT_2X2, 24);
			BasicReadFromFile_VertexInputs_CHECK("mat3Input", SPIRV_Binary::ShaderInputInfo::Type::MAT_3X3, 28);
			BasicReadFromFile_VertexInputs_CHECK("mat4Input", SPIRV_Binary::ShaderInputInfo::Type::MAT_4X4, 32);
#undef BasicReadFromFile_VertexInputs_CHECK
		}
	}
}
