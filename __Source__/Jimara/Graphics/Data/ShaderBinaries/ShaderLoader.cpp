#include "ShaderLoader.h"
#include "../../../OS/IO/MMappedFile.h"
#pragma warning(disable: 26819)
#pragma warning(disable: 26495)
#pragma warning(disable: 28020)
#include <nlohmann/json.hpp>
#pragma warning(default: 26819)
#pragma warning(default: 28020)
#pragma warning(default: 26495)
#include <sstream>


namespace Jimara {
	namespace Graphics {
#pragma warning(disable: 4250)
		namespace {
			class Cache : public virtual ShaderDirectory, public virtual ObjectCache<OS::Path>::StoredObject {
			public:
				inline Cache(OS::Logger* logger, const OS::Path& path) : ShaderDirectory(path, logger) {}
			};
		}
#pragma warning(default: 4250)


		Reference<ShaderDirectoryLoader> ShaderDirectoryLoader::Create(const OS::Path& baseDirectory, OS::Logger* logger) {
			auto error = [&](const auto&... message) { 
				if (logger != nullptr) logger->Error("ShaderDirectoryLoader::Create - ", message...);
				return nullptr;
			};
			
			// Parse ShaderData.json:
			nlohmann::json json;
			{
				const OS::Path shaderDataPath = baseDirectory / "ShaderData.json";
				Reference<OS::MMappedFile> dataMapping = OS::MMappedFile::Create(shaderDataPath, logger);
				if (dataMapping == nullptr) return error("Failed to open file: '", shaderDataPath, "\'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				try {
					const MemoryBlock block(*dataMapping);
					json = nlohmann::json::parse(std::string_view(reinterpret_cast<const char*>(block.Data()), block.Size()));
				}
				catch (nlohmann::json::parse_error& err) {
					return error("Failed to parse file: '", shaderDataPath, "\'! Reason: ", err.what(), " [File: ", __FILE__, "; Line: ", __LINE__, "]");
				}
				if (!json.is_object()) return error("ShaderData does not contain Json object! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			// Get light type ids:
			const constexpr std::string_view LIGHT_TYPE_IDS_KEY = "LightTypeIds";
			const auto lightTypeIds = json.find(LIGHT_TYPE_IDS_KEY);
			if (lightTypeIds == json.end()) 
				return error(LIGHT_TYPE_IDS_KEY, " not present in ShaderData! [File:", __FILE__, "; Line: ", __LINE__, "]");
			else if (!lightTypeIds->is_object()) 
				return error(LIGHT_TYPE_IDS_KEY, " is not a json object! [File:", __FILE__, "; Line: ", __LINE__, "]");

			// Get LightTypes:
			std::unordered_map<std::string, uint32_t> lightTypeIdentifiers;
			{
				const constexpr std::string_view LIGHT_TYPES_KEY = "LightTypes";
				const auto lightTypes = lightTypeIds->find(LIGHT_TYPES_KEY);
				if (lightTypes == lightTypeIds->end()) 
					return error(LIGHT_TYPES_KEY, " not present in ", LIGHT_TYPE_IDS_KEY, "! [File:", __FILE__, "; Line: ", __LINE__, "]");
				else if (!lightTypes->is_object()) 
					return error(LIGHT_TYPES_KEY, " is not a json object! [File:", __FILE__, "; Line: ", __LINE__, "]");
				for (const auto& item : lightTypes->items()) {
					if (!item.value().is_number_unsigned())
						return error(LIGHT_TYPES_KEY, " contains an element that is not an unsigned number! [File:", __FILE__, "; Line: ", __LINE__, "]");
					lightTypeIdentifiers[item.key()] = item.value();
				}
			}

			// Get PerLightDataSize:
			size_t perLightDataSize = 0;
			{
				const constexpr std::string_view PER_LIGET_DATA_SIZE_KEY = "PerLightDataSize";
				const auto perLightDataSizeIt = lightTypeIds->find(PER_LIGET_DATA_SIZE_KEY);
				if (perLightDataSizeIt == lightTypeIds->end()) 
					return error(PER_LIGET_DATA_SIZE_KEY, " not present in ", LIGHT_TYPE_IDS_KEY, "! [File:", __FILE__, "; Line: ", __LINE__, "]");
				else if (!perLightDataSizeIt->is_number_unsigned()) 
					return error(PER_LIGET_DATA_SIZE_KEY, " is not an unsigned number! [File:", __FILE__, "; Line: ", __LINE__, "]");
				perLightDataSize = *perLightDataSizeIt;
			}

			// Create loader:
			const Reference<ShaderDirectoryLoader> loader = new ShaderDirectoryLoader(logger, baseDirectory, std::move(lightTypeIdentifiers), perLightDataSize);
			loader->ReleaseRef();
			return loader;
		}

		Reference<ShaderSet> ShaderDirectoryLoader::LoadShaderSet(const OS::Path& setIdentifier) {
			return GetCachedOrCreate(setIdentifier, false, [&]() ->Reference<ShaderSet> {
				const std::wstring baseDirectory = m_baseDirectory;
				std::wstringstream stream;
				stream << m_baseDirectory;
				if (baseDirectory.length() > 0
					&& baseDirectory[baseDirectory.length() - 1] != L'/'
					&& baseDirectory[baseDirectory.length() - 1] != L'\\') stream << L'/';
				stream << setIdentifier;
				return Object::Instantiate<Cache>(m_logger, stream.str());
				});
		}

		bool ShaderDirectoryLoader::GetLightTypeId(const std::string& lightTypeName, uint32_t& lightTypeId)const {
			const auto it = m_lightTypeIds.find(lightTypeName);
			if (it == m_lightTypeIds.end()) return false;
			lightTypeId = it->second;
			return true;
		}

		size_t ShaderDirectoryLoader::PerLightDataSize()const { return m_perLightDataSize; }
	}
}
