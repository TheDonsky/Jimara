#include "ShaderSet.h"
#include <sstream>


namespace Jimara {
	namespace Graphics {
		ShaderDirectory::ShaderDirectory(const std::string_view& directory, OS::Logger* logger) : m_directory(directory), m_logger(logger) {}

		Reference<SPIRV_Binary> ShaderDirectory::GetShaderModule(const std::string& shaderPath, PipelineStage stage) {
			std::stringstream nameStream;
			nameStream << m_directory;
			if (m_directory.length() > 0
				&& m_directory[m_directory.length() - 1] != '/'
				&& m_directory[m_directory.length() - 1] != '\\') nameStream << '/';
			if (shaderPath.length() > 0) {
				const size_t lastSymbolId = (shaderPath.length() - 1);
				size_t lastIndex;
				for (lastIndex = lastSymbolId; lastIndex > 0; lastIndex--) {
					char symbol = shaderPath[lastIndex];
					if (symbol == '.') {
						lastIndex--;
						break;
					}
					else if (symbol == '/' || symbol == '\\') {
						lastIndex = lastSymbolId;
						break;
					}
				}
				if (lastIndex == 0 && shaderPath[lastIndex] != '.') lastIndex = lastSymbolId;
				for (size_t i = 0; i <= lastIndex; i++) nameStream << shaderPath[i];
			}
			else {
				if (m_logger != nullptr) m_logger->Error("ShaderDirectory::GetShaderModule - Shader Path empty!");
				return nullptr;
			}
			if (stage == PipelineStage::COMPUTE) nameStream << ".comp";
			else if (stage == PipelineStage::VERTEX) nameStream << ".vert";
			else if (stage == PipelineStage::FRAGMENT) nameStream << ".frag";
			else {
				if (m_logger != nullptr) m_logger->Error("ShaderDirectory::GetShaderModule - Invalid pipeline stage!");
				return nullptr;
			}
			nameStream << ".spv";
			return SPIRV_Binary::FromSPVCached(nameStream.str(), m_logger);
		}

		Reference<SPIRV_Binary> ShaderDirectory::GetShaderModule(ShaderClass* shaderClass, PipelineStage stage) {
			if (shaderClass == nullptr) return nullptr;
			else return GetShaderModule(shaderClass->ShaderPath(), stage);
		}
	}
}
