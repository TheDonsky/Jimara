#include "ShaderLibrary.h"


namespace Jimara {
	struct FileSystemShaderLibrary::Helpers {

	};

	Reference<FileSystemShaderLibrary> FileSystemShaderLibrary::Create(const OS::Path& shaderDirectory, OS::Logger* logger) {
		// __TODO__: Implement this crap!
		logger->Fatal("FileSystemShaderLibrary::Create - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		return nullptr;
	}

	FileSystemShaderLibrary::FileSystemShaderLibrary() {
		// __TODO__: Implement this crap!
	}

	FileSystemShaderLibrary::~FileSystemShaderLibrary() {
		// __TODO__: Implement this crap!
	}

	const ShaderLibrary::LitShaderSet* FileSystemShaderLibrary::LitShaders()const {
		return m_litShaders;
	}

	Reference<Graphics::SPIRV_Binary> FileSystemShaderLibrary::LoadLitShader(
		const std::string_view& lightingModelPath, const std::string_view& lightingModelStage,
		const LitShader* litShader, Graphics::PipelineStage graphicsStage) {
		// __TODO__: Implement this crap!
		m_log->Fatal("FileSystemShaderLibrary::LoadLitShader - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		return nullptr;
	}

	Reference<Graphics::SPIRV_Binary> FileSystemShaderLibrary::LoadShader(const std::string_view& directCompiledShaderPath) {
		// __TODO__: Implement this crap!
		m_log->Fatal("FileSystemShaderLibrary::LoadShader - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		return nullptr;
	}
}

