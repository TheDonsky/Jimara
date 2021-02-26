#pragma once
namespace Jimara {
	namespace Graphics {
		class Shader;
		class ShaderCache;
	}
}
#include "../GraphicsDevice.h"
#include "../../Core/ObjectCache.h"
#include <unordered_map>
#include <mutex>


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Shader (can be any vertex/fragment/compute, does not really matter)
		/// </summary>
		class Shader : public virtual ObjectCache<std::string>::StoredObject {};


		/// <summary>
		/// Shader cache for shader module reuse (ei you don't have to load the same shader more times than necessary when allocating through the same cache)
		/// </summary>
		class ShaderCache : public virtual ObjectCache<std::string> {
		public:
			/// <summary>
			/// Loads shader from file or returns one previously loaded
			/// </summary>
			/// <param name="file"> File to load shader from </param>
			/// <param name="storePermanently"> If true, the shader will be marked as "Permanently stored" and will not go out of scope while the cache exists </param>
			/// <returns> Shader instance </returns>
			Reference<Shader> GetShader(const std::string& file, bool storePermanently = false);

			/// <summary> "Owner" device </summary>
			GraphicsDevice* Device()const;

		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="device"> "Owner" device </param>
			ShaderCache(GraphicsDevice* device);

			/// <summary>
			/// Instantiates a shader module
			/// </summary>
			/// <param name="data"> Shader file data </param>
			/// <param name="bytes"> Number of bytes within data </param>
			/// <returns> New shader instance </returns>
			virtual Reference<Shader> CreateShader(char* data, size_t bytes) = 0;

		private:
			// "Owner" device
			Reference<GraphicsDevice> m_device;
		};
	}
}
