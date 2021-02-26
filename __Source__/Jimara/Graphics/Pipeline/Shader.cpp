#include "Shader.h"
#include "../../Core/Function.h"
#include <fstream>
#include <cassert>

namespace Jimara {
	namespace Graphics {
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

			class ShaderInstantiator {
			private:
				const std::string* m_filename;
				OS::Logger* m_logger;
				const Function<Reference<Shader>, char*, size_t> m_shaderCreateFn;

			public:
				inline ShaderInstantiator(const std::string& filename, OS::Logger* logger, const Function<Reference<Shader>, char*, size_t>& shaderCreateFn)
					: m_filename(&filename), m_logger(logger), m_shaderCreateFn(shaderCreateFn) {}

				inline Reference<Shader> operator()()const {
					std::vector<char> fileData = LoadFile(*m_filename);
					if (fileData.size() <= 0) {
						m_logger->Fatal("ShaderCache - Could not read shader from file: \"" + (*m_filename) + "\"");
						return nullptr;
					}
					Reference<Shader> shader = m_shaderCreateFn(fileData.data(), fileData.size());
					if (shader == nullptr)
						m_logger->Fatal("ShaderCache - Could not create shader from file: \"" + (*m_filename) + "\"");
					return shader;
				}
			};
		}

		Reference<Shader> ShaderCache::GetShader(const std::string& file, bool storePermanently) {
			ShaderInstantiator instantiator(file, m_device->Log(), Function<Reference<Shader>, char*, size_t>(&ShaderCache::CreateShader, this));
			return GetCachedOrCreate("FILE::" + file, storePermanently, instantiator);
		}

		GraphicsDevice* ShaderCache::Device()const {
			return m_device;
		}

		ShaderCache::ShaderCache(GraphicsDevice* device)
			: m_device(device) {}
	}
}
