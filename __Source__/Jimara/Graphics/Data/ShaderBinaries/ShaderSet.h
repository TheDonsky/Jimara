#pragma once
#include "ShaderClass.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Shader set for loading shader modules
		/// </summary>
		class JIMARA_API ShaderSet : public virtual Object {
		public:
			/// <summary>
			/// Loads or gets cached SPIRV_Binary for given shader class and stage
			/// </summary>
			/// <param name="shaderClass"> Shader class </param>
			/// <param name="stage"> Pipeline stage we're interested in </param>
			/// <returns> Shader module or nullptr if load fails </returns>
			virtual Reference<SPIRV_Binary> GetShaderModule(const ShaderClass* shaderClass, PipelineStage stage) = 0;
		};

		/// <summary>
		/// ShaderSet, that loads shaders from a simple directory tree, 
		/// interpreting ShaderClass::ShaderPath() as a direct relative path to the shader binary (.vert/frag/etc.spv)
		/// </summary>
		class JIMARA_API ShaderDirectory : public virtual ShaderSet {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="directory"> Base directory </param>
			/// <param name="logger"> Logger for error reporting </param>
			ShaderDirectory(const OS::Path& directory, OS::Logger* logger);

			/// <summary>
			/// Loads or gets cached SPIRV_Binary for given shader path and stage
			/// </summary>
			/// <param name="shaderPath"> Relative path to the shader </param>
			/// <param name="stage"> Pipeline stage we're interested in </param>
			/// <returns> Shader module or nullptr if load fails </returns>
			Reference<SPIRV_Binary> GetShaderModule(const OS::Path& shaderPath, PipelineStage stage);

			/// <summary>
			/// Loads or gets cached SPIRV_Binary for given shader class and stage
			/// </summary>
			/// <param name="shaderClass"> Shader class </param>
			/// <param name="stage"> Pipeline stage we're interested in </param>
			/// <returns> Shader module or nullptr if load fails </returns>
			virtual Reference<SPIRV_Binary> GetShaderModule(const ShaderClass* shaderClass, PipelineStage stage) override;


		private:
			// Logger for error reporting
			const Reference<OS::Logger> m_logger;

			// Base directory
			const OS::Path m_directory;
		};
	}
}
