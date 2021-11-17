#pragma once
#include "ShaderSet.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Infrustructure, that loads shader sets, based on their identifiers
		/// </summary>
		class ShaderLoader : public virtual Object {
		public:
			/// <summary>
			/// Loads shader set
			/// </summary>
			/// <param name="setIdentifier"> Set identifier (for example, path to the lighting model shader) </param>
			/// <returns> Shader set if found, nullptr otherwise </returns>
			virtual Reference<ShaderSet> LoadShaderSet(const OS::Path& setIdentifier) = 0;
		};

		/// <summary>
		/// ShaderLoader, that searches for SPIRV binaries in a folder structure, that directly resembles the set identifiers
		/// </summary>
		class ShaderDirectoryLoader : public virtual ShaderLoader, public virtual ObjectCache<OS::Path> {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="baseDirectory"> Root shader directory </param>
			/// <param name="logger"> Logger for error reporting </param>
			ShaderDirectoryLoader(const OS::Path& baseDirectory, OS::Logger* logger);

			/// <summary>
			/// Loads shader set
			/// </summary>
			/// <param name="setIdentifier"> Set identifier (for example, path to the lighting model shader) </param>
			/// <returns> Shader set if found, nullptr otherwise (basically, ShaderDirectory(join(baseDirectory, setIdentifier))) </returns>
			virtual Reference<ShaderSet> LoadShaderSet(const OS::Path& setIdentifier) override;

		private:
			// Root shader directory
			const OS::Path m_baseDirectory;

			// Logger for error reporting
			const Reference<OS::Logger> m_logger;
		};
	}
}
