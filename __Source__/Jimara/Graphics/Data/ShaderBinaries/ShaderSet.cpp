#include "ShaderSet.h"
#include <sstream>


namespace Jimara {
	namespace Graphics {
		ShaderDirectory::ShaderDirectory(const OS::Path& directory, OS::Logger* logger) : m_directory(directory), m_logger(logger) {}

		Reference<SPIRV_Binary> ShaderDirectory::GetShaderModule(const OS::Path& shaderPath, PipelineStage stage) {
			std::wstringstream nameStream;
			
			const std::wstring directory = m_directory;
			nameStream << directory;
			if (directory.length() > 0
				&& directory[directory.length() - 1] != L'/'
				&& directory[directory.length() - 1] != L'\\') nameStream << L'/';

			const std::wstring shaderSubPath = shaderPath;
			if (shaderSubPath.length() > 0) {
				const size_t lastSymbolId = (shaderSubPath.length() - 1);
				size_t lastIndex;
				for (lastIndex = lastSymbolId; lastIndex > 0; lastIndex--) {
					wchar_t symbol = shaderSubPath[lastIndex];
					if (symbol == L'.') {
						lastIndex--;
						break;
					}
					else if (symbol == L'/' || symbol == L'\\') {
						lastIndex = lastSymbolId;
						break;
					}
				}
				if (lastIndex == 0 && shaderSubPath[lastIndex] != L'.') lastIndex = lastSymbolId;
				for (size_t i = 0; i <= lastIndex; i++) nameStream << shaderSubPath[i];
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
			nameStream << L".spv";
			return SPIRV_Binary::FromSPVCached(nameStream.str(), m_logger);
		}

		Reference<SPIRV_Binary> ShaderDirectory::GetShaderModule(const ShaderClass* shaderClass, PipelineStage stage) {
			if (shaderClass == nullptr) return nullptr;
			else return GetShaderModule(shaderClass->ShaderPath(), stage);
		}
	}
}
