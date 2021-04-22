#include "../GtestHeaders.h"
#include "Graphics/Data/ShaderBinaries/SPIRV_Binary.h"
#include "OS/Logging/StreamLogger.h"


namespace Jimara {
	namespace Graphics {

		TEST(SPIRV_BinaryTest, BasicReadFromFile) {
			Reference<OS::Logger> logger = Object::Instantiate<OS::StreamLogger>();
			Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPV("Shaders/Components/Shaders/Test_ForwardLightingModel/Components/Shaders/Test_SampleDiffuseShader.frag.spv", logger);
			ASSERT_EQ(binary.operator->(), nullptr);
		}
	}
}
