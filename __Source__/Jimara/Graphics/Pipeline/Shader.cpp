#include "Shader.h"
#include "../../Core/Function.h"
#include <fstream>
#include <cassert>

namespace Jimara {
	namespace Graphics {
		ShaderCache::ShaderCache(GraphicsDevice* device) : m_device(device) {}

		Reference<Shader> ShaderCache::GetShader(const std::string& spirvFilename, bool storePermanently, bool storeBytecodePermanently) {
			Reference<SPIRV_Binary> binary = SPIRV_Binary::FromSPVCached(spirvFilename, m_device->Log(), storeBytecodePermanently);
			return GetShader(binary, storePermanently);
		}

		Reference<Shader> ShaderCache::GetShader(const SPIRV_Binary* binary, bool storePermanently) {
			if (binary == nullptr) {
				m_device->Log()->Error("ShaderCache::GetShader - null binary provided!");
				return nullptr;
			}
			else return GetCachedOrCreate(binary, storePermanently, [&]() ->Reference<Shader> { return m_device->CreateShader(binary); });
		}

		GraphicsDevice* ShaderCache::Device()const { return m_device; }

		namespace {
			class CacheOfCaches : public virtual ObjectCache<Reference<GraphicsDevice>> {
			public:
				static Reference<ShaderCache> GetCache(GraphicsDevice* device) {
					static CacheOfCaches cache;
					if (device == nullptr) return nullptr;
					else return cache.GetCachedOrCreate(device, false, [&]()->Reference<ShaderCache> { return Object::Instantiate<ShaderCache>(device); });
				}
			};
		}

		Reference<ShaderCache> ShaderCache::ForDevice(GraphicsDevice* device) { return CacheOfCaches::GetCache(device); }
	}
}
