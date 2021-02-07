#include "Shader.h"
#include <fstream>
#include <cassert>

namespace Jimara {
	namespace Graphics {
		Shader::Shader() : m_cache(nullptr), m_permanentStorage(false) {}

		void Shader::OnOutOfScope()const {
			bool shouldDelete;
			if (m_cache != nullptr) {
				std::unique_lock<std::mutex> lock(m_cache->m_cacheLock);
				if (RefCount() > 0)
					shouldDelete = false;
				else {
					if (m_permanentStorage)
						shouldDelete = false;
					else {
						m_cache->m_shaders.erase(m_cacheId);
						shouldDelete = true;
					}
					m_cache->ReleaseRef();
					m_cache = nullptr;
				}
			}
			else shouldDelete = true;

			if (shouldDelete)
				delete this;
		}



		ShaderCache::~ShaderCache() {
			for (std::unordered_map<std::string, Shader*>::const_iterator it = m_shaders.begin(); it != m_shaders.end(); ++it)
				delete it->second;
		}

		namespace {
			inline static std::vector<char> LoadFile(const std::string& filename) {
				std::ifstream file(filename, std::ios::ate | std::ios::binary);
				if (!file.is_open())
					return std::vector<char>();
				size_t fileSize = (size_t)file.tellg();
				std::vector<char> content(fileSize);
				file.seekg(0);
				file.read(content.data(), fileSize);
				file.close();
				return content;
			}
		}

		Reference<Shader> ShaderCache::GetShader(const std::string& file, bool storePermanently) {
			const std::string shaderId = "FILE::" + file;
			auto tryGetCached = [&]() -> Shader* {
				std::unordered_map<std::string, Shader*>::const_iterator it = m_shaders.find(shaderId);
				if (it != m_shaders.end()) {
					it->second->m_permanentStorage |= storePermanently;
					return it->second;
				}
				else return nullptr;
			};
			
			{
				std::unique_lock<std::mutex> lock(m_cacheLock);
				Reference<Shader> cached = tryGetCached();
				if (cached != nullptr) {
					if (cached->m_cache == nullptr) {
						cached->m_cache = this;
						AddRef();
					}
					return cached;
				}
			}

			std::vector<char> fileData = LoadFile(file);
			Shader* shader = fileData.size() > 0 ? CreateShader(fileData.data(), fileData.size()) : nullptr;
			
			Reference<Shader> returnValue;
			{
				std::unique_lock<std::mutex> lock(m_cacheLock);
				Shader* cached = tryGetCached();
				if (cached != nullptr)
					returnValue = cached;
				else if (shader != nullptr) {
					shader->m_cacheId = shaderId;
					shader->m_permanentStorage = storePermanently;
					m_shaders[shaderId] = shader;
					returnValue = shader;
					shader->ReleaseRef();
				}
				if (returnValue->m_cache == nullptr) {
					returnValue->m_cache = this;
					AddRef();
				}
			}

			if (shader != returnValue && shader != nullptr)
				shader->ReleaseRef();
			
			if (returnValue == nullptr)
				m_device->Log()->Error("ShaderCache - Could not load shader from file: \"" + file + "\"");

			return returnValue;
		}

		GraphicsDevice* ShaderCache::Device()const {
			return m_device;
		}

		ShaderCache::ShaderCache(GraphicsDevice* device)
			: m_device(device) {}
	}
}
