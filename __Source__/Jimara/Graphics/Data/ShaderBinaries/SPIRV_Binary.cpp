#include "SPIRV_Binary.h"
#pragma warning(disable: 26812)
#include <spirv_reflect.h>
#pragma warning(disable: 26819)
#pragma warning(disable: 6385)
#pragma warning(disable: 6386)
#pragma warning(disable: 6387)
#pragma warning(disable: 6011)
#include <spirv_reflect.c>
#pragma warning(default: 26819)
#pragma warning(default: 6385)
#pragma warning(default: 6386)
#pragma warning(default: 6387)
#pragma warning(default: 6011)
#include <fstream>


namespace Jimara {
	namespace Graphics {
		Reference<SPIRV_Binary> SPIRV_Binary::FromSPV(const std::string& filename, OS::Logger* logger) {
			std::ifstream file(filename, std::ios::ate | std::ios::binary);
			if (!file.is_open()) {
				if (logger != nullptr)
					logger->Error("SPIRV_Binary::FromSPV - Failed to open file \"" + filename + "\"!");
				return nullptr;
			}
			size_t fileSize = (size_t)file.tellg();
			std::vector<char> content(fileSize);
			file.seekg(0);
			file.read(content.data(), fileSize);
			file.close();
			return FromData(content, logger);
		}

		Reference<SPIRV_Binary> SPIRV_Binary::FromData(const void* data, size_t size, OS::Logger* logger) {
			SpvReflectShaderModule spvModule;
			SpvReflectResult spvResult = spvReflectCreateShaderModule(size, data, &spvModule);
			if (spvResult != SPV_REFLECT_RESULT_SUCCESS) {
				if (logger != nullptr)
					logger->Error("SPIRV_Binary::FromData - spvReflectCreateShaderModule failed with code ", static_cast<int>(spvResult), "!");
				return nullptr;
			}
			if (logger != nullptr) {
				logger->Info("SPIRV_Binary::FromData - Shader stage: ", spvModule.shader_stage, "; entry point name: \"", spvModule.entry_point_name, "\"");
				for (size_t i = 0; i < spvModule.entry_point_count; i++) {
					const SpvReflectEntryPoint& entryPoint = spvModule.entry_points[i];
					logger->Info("SPIRV_Binary::FromData - entryPoint:\"", entryPoint.name, "\"");
				}
				for (size_t i = 0; i < spvModule.input_variable_count; i++) {
					const SpvReflectInterfaceVariable* input = spvModule.input_variables[i];
					logger->Info("SPIRV_Binary::FromData - input:\"", input->name, "\" location:", input->location);
				}
				for (size_t i = 0; i < spvModule.descriptor_binding_count; i++) {
					const SpvReflectDescriptorBinding& binding = spvModule.descriptor_bindings[i];
					logger->Info("SPIRV_Binary::FromData - descriptor binding:\"", binding.name, "\"");
				}
			}
			spvReflectDestroyShaderModule(&spvModule);
			Reference<SPIRV_Binary> reference = new SPIRV_Binary(data, size, logger);
			reference->ReleaseRef();
			return reference;
		}

		Reference<SPIRV_Binary> SPIRV_Binary::FromData(const std::vector<uint8_t>& data, OS::Logger* logger) {
			return FromData((void*)data.data(), data.size(), logger);
		}

		Reference<SPIRV_Binary> SPIRV_Binary::FromData(const std::vector<char>& data, OS::Logger* logger) {
			return FromData((void*)data.data(), data.size(), logger);
		}
		
		SPIRV_Binary::SPIRV_Binary(const void* data, size_t size, OS::Logger* logger) : m_logger(logger) {
			// __TODO__: Implement this crap!
		}

		SPIRV_Binary::~SPIRV_Binary() {
			// __TODO__: Implement this crap!
		}
	}
}
#pragma warning(default: 26812)
