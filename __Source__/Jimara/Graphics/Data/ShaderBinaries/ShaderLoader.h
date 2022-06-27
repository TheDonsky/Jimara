#pragma once
#include "ShaderSet.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Infrustructure, that loads shader sets, based on their identifiers
		/// </summary>
		class JIMARA_API ShaderLoader : public virtual Object {
		public:
			/// <summary>
			/// Loads shader set
			/// </summary>
			/// <param name="setIdentifier"> Set identifier (for example, path to the lighting model shader) </param>
			/// <returns> Shader set if found, nullptr otherwise </returns>
			virtual Reference<ShaderSet> LoadShaderSet(const OS::Path& setIdentifier) = 0;

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
		/// ShaderLoader, that searches for SPIRV binaries in a folder structure, that directly resembles the set identifiers
		/// </summary>
		class JIMARA_API ShaderDirectoryLoader : public virtual ShaderLoader, public virtual ObjectCache<OS::Path> {
		public:
			/// <summary>
			/// Creates a ShaderDirectoryLoader for given root directory
			/// </summary>
			/// <param name="baseDirectory"> Root shader directory </param>
			/// <param name="logger"> Logger for error reporting </param>
			static Reference<ShaderDirectoryLoader> Create(const OS::Path& baseDirectory, OS::Logger* logger);

			/// <summary>
			/// Loads shader set
			/// </summary>
			/// <param name="setIdentifier"> Set identifier (for example, path to the lighting model shader) </param>
			/// <returns> Shader set if found, nullptr otherwise (basically, ShaderDirectory(join(baseDirectory, setIdentifier))) </returns>
			virtual Reference<ShaderSet> LoadShaderSet(const OS::Path& setIdentifier) override;

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
			// Root shader directory
			const OS::Path m_baseDirectory;

			// Logger for error reporting
			const Reference<OS::Logger> m_logger;

			// Light type name to Id:
			const std::unordered_map<std::string, uint32_t> m_lightTypeIds;

			// Maximal size of a single light data buffer
			const size_t m_perLightDataSize;

			// Constructor is private:
			inline ShaderDirectoryLoader(OS::Logger* logger, const OS::Path& directory,
				std::unordered_map<std::string, uint32_t>&& lightTypeIds, size_t perLightDataSize)
				: m_baseDirectory(directory), m_logger(logger) 
				, m_lightTypeIds(std::move(lightTypeIds)), m_perLightDataSize(perLightDataSize) {}
		};
	}
}
