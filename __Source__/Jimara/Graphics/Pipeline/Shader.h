#pragma once
namespace Jimara {
	namespace Graphics {
		class Shader;
		class ShaderCache;
	}
}
#include "../GraphicsDevice.h"
#include <unordered_map>
#include <mutex>


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Shader (can be any vertex/fragment/compute, does not really matter)
		/// </summary>
		class Shader {
		public:
			/// <summary> Increments reference counter </summary>
			void AddRef()const;

			/// <summary> Decrements reference counter </summary>
			void ReleaseRef()const;


		protected:
			/// <summary> Only chaild objects are permitted </summary>
			Shader();

			/// <summary> Virtual destructor, because reasons </summary>
			inline virtual ~Shader() {}

		private:
			// Reference count
			mutable std::atomic<std::size_t> m_referenceCount;
			
			// "Owner" cache
			ShaderCache* m_cache;

			// Shader identifier within the cache
			std::string m_cacheId;

			// If true, the shader will stay in cache even if nobody has a reference to it
			mutable bool m_permanentStorage;

			// ShaderCache has to access some of the internals
			friend class ShaderCache;

			// Reference counter should not be able to be a reson of some random fuck-ups, so let us disable copy/move-constructor/assignments:
			Shader(const Shader&) = delete;
			Shader& operator=(const Shader&) = delete;
			Shader(Shader&&) = delete;
			Shader& operator=(Shader&&) = delete;
		};


		/// <summary>
		/// Shader cache for shader module reuse (ei you don't have to load the same shader more times than necessary when allocating through the same cache)
		/// </summary>
		class ShaderCache : public virtual Object {
		public:
			/// <summary> Virtual destructor </summary>
			virtual ~ShaderCache();

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
			virtual Shader* CreateShader(char* data, size_t bytes) = 0;

		private:
			// "Owner" device
			Reference<GraphicsDevice> m_device;

			// Lock for cache content
			std::mutex m_cacheLock;
			
			// Cached shaders
			std::unordered_map<std::string, Shader*> m_shaders;

			// Shader has to access cache when reference counter is altered
			friend class Shader;
		};
	}
}