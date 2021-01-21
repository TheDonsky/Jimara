#include "Shader.h"
#include <fstream>
#include <cassert>

namespace Jimara {
	namespace Graphics {
		void Shader::AddRef()const {
			if (m_cache != nullptr) 
				m_cache->AddRef();
			m_referenceCount++;
		}

		void Shader::ReleaseRef()const {
			std::size_t count = m_referenceCount.fetch_sub(1);
			assert(count > 0);
			bool shouldDelete;
			if (count == 1) {
				if (m_cache != nullptr) {
					std::unique_lock<std::mutex> lock(m_cache->m_cacheLock);
					if (m_referenceCount > 0 || m_permanentStorage) 
						shouldDelete = false;
					else {
						m_cache->m_shaders.erase(m_cacheId);
						shouldDelete = true;
					}
				}
				else shouldDelete = true;
			}
			else shouldDelete = false;

			if (m_cache != nullptr) 
				m_cache->ReleaseRef();

			if (shouldDelete)
				delete this;
		}

		Shader::Shader() : m_referenceCount(0), m_cache(nullptr), m_permanentStorage(false) {}

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
				if (cached != nullptr) return cached;
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
					shader->m_cache = this;
					shader->m_cacheId = shaderId;
					shader->m_permanentStorage = storePermanently;
					m_shaders[shaderId] = shader;
					returnValue = shader;
				}
			}

			if (shader != returnValue && shader != nullptr)
				delete shader;
			
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
