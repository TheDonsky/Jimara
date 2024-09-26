#pragma once
#include "../Graphics/GraphicsDevice.h"
#include "Materials/Material.h"

namespace Jimara {
	/// <summary>
	/// Library of available shaders, enabling loading SPIRV-binaries in a platform/build agnostic way
	/// </summary>
	class JIMARA_API ShaderLibrary : public virtual Object {
	public:
		/// <summary> Virtual destructor </summary>
		inline virtual ~ShaderLibrary() {}

		/// <summary> Gives access to available lit-shaders within the shader library </summary>
		virtual const Material::LitShaderSet* LitShaders()const = 0;

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
			const Material::LitShader* litShader, Graphics::PipelineStage graphicsStage) = 0;

		/// <summary>
		/// Loads a 'direct-compiled'/custom shader from the library
		/// </summary>
		/// <param name="directCompiledShaderPath"> Relative shader path structured as ProjSrcRoot/path/to/code.ext </param>
		/// <returns> SPIR-V binary (shared between multiple requests) </returns>
		virtual Reference<Graphics::SPIRV_Binary> LoadShader(const std::string_view& directCompiledShaderPath) = 0;

		/// <summary>
		/// Translates light type name to unique type identifier that can be used within the shaders
		/// </summary>
		/// <param name="lightTypeName"> Light type name </param>
		/// <param name="lightTypeId"> Reference to store light type identifier at </param>
		/// <returns> True, if light type was found </returns>
		virtual bool GetLightTypeId(const std::string& lightTypeName, uint32_t& lightTypeId)const = 0;

		/// <summary> Maximal size of a single light data buffer </summary>
		virtual size_t PerLightDataSize()const = 0;
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
		static Reference<FileSystemShaderLibrary> Create(const OS::Path& shaderDirectory, OS::Logger* logger);

		/// <summary> Virtual destructor </summary>
		virtual ~FileSystemShaderLibrary();

		/// <summary> Gives access to available lit-shaders within the shader library </summary>
		virtual const Material::LitShaderSet* LitShaders()const override;

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
			const Material::LitShader* litShader, Graphics::PipelineStage graphicsStage)override;

		/// <summary>
		/// Loads a 'direct-compiled'/custom shader from the library
		/// </summary>
		/// <param name="directCompiledShaderPath"> Relative shader path structured as ProjSrcRoot/path/to/code.ext </param>
		/// <returns> SPIR-V binary (shared between multiple requests) </returns>
		virtual Reference<Graphics::SPIRV_Binary> LoadShader(const std::string_view& directCompiledShaderPath)override;

		/// <summary>
		/// Translates light type name to unique type identifier that can be used within the shaders
		/// </summary>
		/// <param name="lightTypeName"> Light type name </param>
		/// <param name="lightTypeId"> Reference to store light type identifier at </param>
		/// <returns> True, if light type was found </returns>
		virtual bool GetLightTypeId(const std::string& lightTypeName, uint32_t& lightTypeId)const override;

		/// <summary> Maximal size of a single light data buffer </summary>
		virtual size_t PerLightDataSize()const override;


	private:
		// Logger for error reporting
		const Reference<OS::Logger> m_logger;

		// Root shader directory
		const OS::Path m_baseDirectory;

		// Lit-shader collection
		const Reference<const Material::LitShaderSet> m_litShaders;

		// Light type name to Id:
		const std::unordered_map<std::string, uint32_t> m_lightTypeIds;

		// Maximal size of a single light data buffer
		const size_t m_perLightDataSize;

		// Directories per each lighting model
		const std::unordered_map<std::string, std::string> m_lightingModelDirectories;

		// Constructor is private!
		FileSystemShaderLibrary(OS::Logger* logger, 
			const OS::Path& directory, const Material::LitShaderSet* litShaders,
			std::unordered_map<std::string, uint32_t>&& lightTypeIds, size_t perLightDataSize,
			std::unordered_map<std::string, std::string>&& lightingModelDirectories);

		// Some helper stuff resides in here..
		struct Helpers;
	};
}
