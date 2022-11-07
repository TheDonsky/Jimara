#pragma once
namespace Jimara {
	namespace Graphics {
		class Shader;
		class ShaderCache;
	}
}
#include "../GraphicsDevice.h"
#include "../../Core/Collections/ObjectCache.h"
#include "../Data/ShaderBinaries/SPIRV_Binary.h"



#pragma warning(disable: 4250)
#pragma warning(disable: 4275)
namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Shader (can be any vertex/fragment/compute, does not really matter)
		/// </summary>
		class JIMARA_API Shader : public virtual ObjectCache<Reference<const Object>>::StoredObject {
		public:
			/// <summary> SPIRV-Binary, this shader was generated from </summary>
			const SPIRV_Binary* Binary()const { return m_binary; }

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="binary"> SPIRV-Binary, this shader was generated from </param>
			Shader(const SPIRV_Binary* binary) : m_binary(binary) {}

		private:
			// SPIRV-Binary, this shader was generated from
			const Reference<const SPIRV_Binary> m_binary;
		};


		/// <summary>
		/// Shader cache for shader module reuse (ei you don't have to load the same shader more times than necessary when allocating through the same cache)
		/// </summary>
		class JIMARA_API ShaderCache : public virtual ObjectCache<Reference<GraphicsDevice>>::StoredObject, public virtual ObjectCache<Reference<const Object>> {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="device"> "Owner" device </param>
			ShaderCache(GraphicsDevice* device);

			/// <summary>
			/// Creates a shader or returns a cached one.
			/// </summary>
			/// <param name="spirvFilename"> Name of a SPIR-V binary file (bytecode will be loaded for you) </param>
			/// <param name="storePermanently"> If true, the shader will be stored in in cache indefinately </param>
			/// <param name="storeBytecodePermanently"> If true, the bytecode will be stored in in global cache till the program exits </param>
			/// <returns> Shader module or nullptr if failed </returns>
			Reference<Shader> GetShader(const std::string& spirvFilename, bool storePermanently = false, bool storeBytecodePermanently = false);

			/// <summary>
			/// Creates a shader or returns a cached one.
			/// </summary>
			/// <param name="binary"> SPIR-V binary </param>
			/// <param name="storePermanently"> If true, the shader will be stored in cache indefinately </param>
			/// <returns> Shader module or nullptr if failed </returns>
			Reference<Shader> GetShader(const SPIRV_Binary* binary, bool storePermanently = false);

			/// <summary> "Owner" device </summary>
			GraphicsDevice* Device()const;

			/// <summary>
			/// Instance of a shader cache for given device (you can create manually, but this one may be slightly more convenient in most cases)
			/// </summary>
			/// <param name="device"> Device, the cache belongs to </param>
			/// <returns> Singleton instance of the cache for the device </returns>
			static Reference<ShaderCache> ForDevice(GraphicsDevice* device);


		private:
			// "Owner" device
			Reference<GraphicsDevice> m_device;
		};
	}
}
#pragma warning(default: 4275)
#pragma warning(default: 4250)
