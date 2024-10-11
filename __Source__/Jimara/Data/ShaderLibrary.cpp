#include "ShaderLibrary.h"
#include "Serialization/Helpers/SerializeToJson.h"
#include "Materials/LitShaderSetSerializer.h"
#include "../OS/IO/MMappedFile.h"

namespace Jimara {
	struct FileSystemShaderLibrary::Helpers {
		inline static Reference<Graphics::SPIRV_Binary> LoadShader(
			const FileSystemShaderLibrary* self,
			const std::string_view& modelPath, const std::string_view& modelStage,
			const std::filesystem::path& shaderPath, const Graphics::PipelineStage graphicsStage) {
			const auto modelIt = self->m_lightingModelDirectories.find(std::string(modelPath));
			if (modelIt == self->m_lightingModelDirectories.end()) {
				if (self->m_logger != nullptr)
					self->m_logger->Error("FileSystemShaderLibrary::Load - Unknown lighting model: ", modelPath, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}
			std::wstringstream nameStream;

			// Fill-in base directory
			{
				const std::wstring baseDirectory = self->m_baseDirectory;
				nameStream << self->m_baseDirectory;
				if (baseDirectory.length() > 0
					&& baseDirectory[baseDirectory.length() - 1] != L'/'
					&& baseDirectory[baseDirectory.length() - 1] != L'\\')
					nameStream << L'/';
				nameStream << modelIt->second;
				if (modelIt->second.length() > 0
					&& modelIt->second[modelIt->second.length() - 1] != L'/'
					&& modelIt->second[modelIt->second.length() - 1] != L'\\')
					nameStream << L'/';
			}

			const std::wstring shaderSubPath = shaderPath.wstring();
			if (shaderSubPath.length() > 0)
				nameStream << shaderSubPath;
			else {
				if (self->m_logger != nullptr)
					self->m_logger->Error("FileSystemShaderLibrary::Load - Shader Path empty! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}

			// Model stage:
			if (modelStage.length() > 0u)
				nameStream << L"." << std::filesystem::path(modelStage).wstring();

			// Graphics stage:
			switch (graphicsStage) {
			case Graphics::PipelineStage::NONE:
				break;
			case Graphics::PipelineStage::COMPUTE:
				nameStream << L".comp";
				break;
			case Graphics::PipelineStage::VERTEX:
				nameStream << L".vert";
				break;
			case Graphics::PipelineStage::FRAGMENT:
				nameStream << L".frag";
				break;
			case Graphics::PipelineStage::RAY_GENERATION:
				nameStream << L".rgen";
				break;
			case Graphics::PipelineStage::RAY_MISS:
				nameStream << L".rmiss";
				break;
			case Graphics::PipelineStage::RAY_ANY_HIT:
				nameStream << L".rahit";
				break;
			case Graphics::PipelineStage::RAY_CLOSEST_HIT:
				nameStream << L".rchit";
				break;
			case Graphics::PipelineStage::RAY_INTERSECTION:
				nameStream << L".rint";
				break;
			case Graphics::PipelineStage::CALLABLE:
				nameStream << L".rcall";
				break;
			default:
				if (self->m_logger != nullptr)
					self->m_logger->Error("FileSystemShaderLibrary::Load - Invalid pipeline stage! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}

			// Load cached SPV:
			nameStream << L".spv";
			return Graphics::SPIRV_Binary::FromSPVCached(nameStream.str(), self->m_logger);
		}
	};

	Reference<FileSystemShaderLibrary> FileSystemShaderLibrary::Create(const OS::Path& shaderDirectory, OS::Logger* logger) {
		auto error = [&](const auto&... message) {
			if (logger != nullptr) 
				logger->Error("FileSystemShaderLibrary::Create - ", message...);
			return nullptr;
		};

		// Parse ShaderData.json:
		nlohmann::json json;
		{
			const OS::Path shaderDataPath = shaderDirectory / "ShaderData.json";
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

		// Get LightTypes:
		std::unordered_map<std::string, uint32_t> lightTypeIdentifiers;
		{
			const constexpr std::string_view LIGHT_TYPES_KEY = "LightTypes";
			const auto lightTypes = json.find(LIGHT_TYPES_KEY);
			if (lightTypes == json.end())
				return error(LIGHT_TYPES_KEY, " not present in ShaderData! [File:", __FILE__, "; Line: ", __LINE__, "]");
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
			const auto perLightDataSizeIt = json.find(PER_LIGET_DATA_SIZE_KEY);
			if (perLightDataSizeIt == json.end())
				return error(PER_LIGET_DATA_SIZE_KEY, " not present in ShaderData! [File:", __FILE__, "; Line: ", __LINE__, "]");
			else if (!perLightDataSizeIt->is_number_unsigned())
				return error(PER_LIGET_DATA_SIZE_KEY, " is not an unsigned number! [File:", __FILE__, "; Line: ", __LINE__, "]");
			perLightDataSize = *perLightDataSizeIt;
		}

		// Get LightingModels:
		std::unordered_map<std::string, std::string> lightingModelDirs;
		{
			const constexpr std::string_view LIGHTING_MODELS_KEY = "LightingModels";
			const auto lightingModelsIt = json.find(LIGHTING_MODELS_KEY);
			if (lightingModelsIt == json.end())
				return error(LIGHTING_MODELS_KEY, " not present in ShaderData! [File:", __FILE__, "; Line: ", __LINE__, "]");
			else if (!lightingModelsIt->is_object())
				return error(LIGHTING_MODELS_KEY, " is not a json object! [File:", __FILE__, "; Line: ", __LINE__, "]");
			for (const auto& item : lightingModelsIt->items()) {
				if (!item.value().is_string())
					return error(LIGHTING_MODELS_KEY, " contains an element that is not a string! [File:", __FILE__, "; Line: ", __LINE__, "]");
				lightingModelDirs[item.key()] = static_cast<std::string>(item.value());
			}
		}

		// Load lit-shaders:
		Reference<const Material::LitShaderSet> shaderSet = {};
		if (json.contains("LitShaders")) {
			static const LitShaderSetSerializer serializer("LitShaders", "Available Lit-Shader list");
			if (!Serialization::DeserializeFromJson(serializer.Serialize(shaderSet), json["LitShaders"], logger,
				[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
					if (logger != nullptr)
						logger->Error("ShaderDirectoryLoader::Create - LitShaders node should not contain any object references! ",
							"[File:", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}))
				return error("ShaderDirectoryLoader::Create - LitShaders node could not be deserialized!, [File:", __FILE__, "; Line: ", __LINE__, "]");
			if (shaderSet == nullptr)
				shaderSet = Object::Instantiate<Material::LitShaderSet>(std::set<Reference<const Material::LitShader>>{});
		}

		// Create library:
		const Reference<FileSystemShaderLibrary> loader = new FileSystemShaderLibrary(
			logger, shaderDirectory, shaderSet,
			std::move(lightTypeIdentifiers), perLightDataSize,
			std::move(lightingModelDirs));
		loader->ReleaseRef();
		return loader;
	}

	FileSystemShaderLibrary::FileSystemShaderLibrary(OS::Logger* logger,
		const OS::Path& directory, const Material::LitShaderSet* litShaders,
		std::unordered_map<std::string, uint32_t>&& lightTypeIds, size_t perLightDataSize,
		std::unordered_map<std::string, std::string>&& lightingModelDirectories) 
		: m_logger(logger), m_baseDirectory(directory), m_litShaders(litShaders)
		, m_lightTypeIds(std::move(lightTypeIds)), m_perLightDataSize(perLightDataSize)
		, m_lightingModelDirectories(std::move(lightingModelDirectories)) {}

	FileSystemShaderLibrary::~FileSystemShaderLibrary() {}

	const Material::LitShaderSet* FileSystemShaderLibrary::LitShaders()const {
		return m_litShaders;
	}

	Reference<Graphics::SPIRV_Binary> FileSystemShaderLibrary::LoadLitShader(
		const std::string_view& lightingModelPath, const std::string_view& lightingModelStage,
		const Material::LitShader* litShader, Graphics::PipelineStage graphicsStage) {
		if (litShader != nullptr)
			return Helpers::LoadShader(this, lightingModelPath, lightingModelStage, litShader->LitShaderPath(), graphicsStage);
		std::filesystem::path lmName;
		try {
			lmName = OS::Path(lightingModelPath).filename().replace_extension();
		}
		catch (const std::exception& e) {
			if (m_logger != nullptr)
				m_logger->Error("FileSystemShaderLibrary::LoadLitShader - ", e.what());
		}
		return Helpers::LoadShader(this, lightingModelPath, lightingModelStage, lmName, graphicsStage);
	}

	Reference<Graphics::SPIRV_Binary> FileSystemShaderLibrary::LoadShader(const std::string_view& directCompiledShaderPath) {
		return Helpers::LoadShader(this, "", "", directCompiledShaderPath, Graphics::PipelineStage::NONE);
	}

	bool FileSystemShaderLibrary::GetLightTypeId(const std::string& lightTypeName, uint32_t& lightTypeId)const {
		const auto it = m_lightTypeIds.find(lightTypeName);
		if (it == m_lightTypeIds.end()) return false;
		lightTypeId = it->second;
		return true;
	}

	size_t FileSystemShaderLibrary::PerLightDataSize()const {
		return m_perLightDataSize;
	}
}
