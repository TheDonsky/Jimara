#pragma once
#include "../Graphics/GraphicsDevice.h"
#include "Materials/MaterialShader.h"

namespace Jimara {
	/// <summary>
	/// Library of available shaders, enabling loading SPIRV-binaries in a platform/build agnostic way
	/// </summary>
	class JIMARA_API ShaderLibrary : public virtual Object {
	public:
		/// <summary> Lit-shader definition </summary>
		using LitShader = Refactor::Material::LitShader;

		/// <summary> Lit-shader collection </summary>
		using LitShaderSet = Refactor::Material::LitShaderSet;

		/// <summary> Virtual destructor </summary>
		inline virtual ~ShaderLibrary() {}

		/// <summary> Gives access to available lit-shaders within the shader library </summary>
		virtual const LitShaderSet* LitShaders()const = 0;

		/// <summary>
		/// Loads/Retrieves a lit-shader instance for given lightingModel/JM_LightingModelStage/JM_ShaderStage combination
		/// </summary>
		/// <param name="lightingModelPath"> Relative lighting-model path structured as ProjSrcRoot/path/to/code.ext </param>
		/// <param name="lightingModelStage"> JM_LightingModelStage name </param>
		/// <param name="litShader"> Lit-shader definition (HAS TO BE from LitShaders()) </param>
		/// <param name="graphicsStage"> JM_ShaderStage </param>
		/// <returns> SPIR-V binary (shared between multiple requests) </returns>
		virtual Reference<Graphics::SPIRV_Binary> LoadLitShader(
			const std::string_view& lightingModelPath, const std::string_view& lightingModelStage,
			const LitShader* litShader, Graphics::PipelineStage graphicsStage) = 0;

		/// <summary>
		/// Loads a 'direct-compiled'/custom shader from the library
		/// </summary>
		/// <param name="directCompiledShaderPath"> Relative shader path structured as ProjSrcRoot/path/to/code.ext </param>
		/// <returns> SPIR-V binary (shared between multiple requests) </returns>
		virtual Reference<Graphics::SPIRV_Binary> LoadShader(const std::string_view& directCompiledShaderPath) = 0;
	};


	/// <summary>
	/// Shader-Library that loads binaries produced by jimara_build_shaders.py scripts
	/// </summary>
	class JIMARA_API FileSystemShaderLibrary : public virtual ShaderLibrary {
	public:
		/// <summary>
		/// Creates a FileSystemShaderLibrary
		/// </summary>
		/// <param name="shaderDirectory"> Path to the compiled shader directory (shaders within have to be compiled with jimara_build_shaders) </param>
		/// <param name="logger"> Logger for error reporting </param>
		/// <returns> New instance of a FileSystemShaderLibrary </returns>
		Reference<FileSystemShaderLibrary> Create(const OS::Path& shaderDirectory, OS::Logger* logger);

		/// <summary> Virtual destructor </summary>
		virtual ~FileSystemShaderLibrary();

		/// <summary> Gives access to available lit-shaders within the shader library </summary>
		virtual const LitShaderSet* LitShaders()const override;

		/// <summary>
		/// Loads/Retrieves a lit-shader instance for given lightingModel/JM_LightingModelStage/JM_ShaderStage combination
		/// </summary>
		/// <param name="lightingModelPath"> Relative lighting-model path structured as ProjSrcRoot/path/to/code.ext </param>
		/// <param name="lightingModelStage"> JM_LightingModelStage name </param>
		/// <param name="litShader"> Lit-shader definition (HAS TO BE from LitShaders()) </param>
		/// <param name="graphicsStage"> JM_ShaderStage </param>
		/// <returns> SPIR-V binary (shared between multiple requests) </returns>
		virtual Reference<Graphics::SPIRV_Binary> LoadLitShader(
			const std::string_view& lightingModelPath, const std::string_view& lightingModelStage,
			const LitShader* litShader, Graphics::PipelineStage graphicsStage) override;

		/// <summary>
		/// Loads a 'direct-compiled'/custom shader from the library
		/// </summary>
		/// <param name="directCompiledShaderPath"> Relative shader path structured as ProjSrcRoot/path/to/code.ext </param>
		/// <returns> SPIR-V binary (shared between multiple requests) </returns>
		virtual Reference<Graphics::SPIRV_Binary> LoadShader(const std::string_view& directCompiledShaderPath) override;


	private:
		// Lit-shader collection
		const Reference<LitShaderSet> m_litShaders;

		// Logger
		const Reference<OS::Logger> m_log;

		// Constructor is private!
		FileSystemShaderLibrary();

		// Private stuff resides in-here...
		struct Helpers;
	};
}
