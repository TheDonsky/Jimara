#include "SPIRV_Binary.h"
#include "../../../OS/IO/MMappedFile.h"
#pragma warning(disable: 26812)
#include <spirv_reflect.h>
#pragma warning(disable: 26819)
#pragma warning(disable: 6385)
#pragma warning(disable: 6386)
#pragma warning(disable: 6387)
#pragma warning(disable: 6011)
#include <spirv_reflect.c>
#pragma warning(default: 26819)
#pragma warning(default: 6385)
#pragma warning(default: 6386)
#pragma warning(default: 6387)
#pragma warning(default: 6011)


namespace Jimara {
	namespace Graphics {
		size_t SPIRV_Binary::BindingSetInfo::Id()const { return m_id; }

		size_t SPIRV_Binary::BindingSetInfo::BindingCount()const { return m_bindings.size(); }

		const SPIRV_Binary::BindingInfo& SPIRV_Binary::BindingSetInfo::Binding(size_t index)const { return m_bindings[index]; }

		const bool SPIRV_Binary::BindingSetInfo::FindBinding(size_t bindingId, const BindingInfo*& binding)const {
			std::unordered_map<size_t, size_t>::const_iterator it = m_idToIndex.find(bindingId);
			if (it == m_idToIndex.end()) return false;
			binding = &m_bindings[it->second];
			return true;
		}

		const bool SPIRV_Binary::BindingSetInfo::FindBinding(const std::string& bindingName, const BindingInfo*& binding)const {
			std::unordered_map<std::string, size_t>::const_iterator it = m_nameToIndex.find(bindingName);
			if (it == m_nameToIndex.end()) return false;
			binding = &m_bindings[it->second];
			return true;
		}

		SPIRV_Binary::BindingSetInfo::BindingSetInfo(size_t id, const std::vector<BindingInfo>& bindings)
			: m_id(id)
			, m_bindings(std::move([&]() {
				std::vector<BindingInfo> list(bindings.begin(), bindings.end());
				for (size_t i = 0; i < bindings.size(); i++) {
					list[i].set = id;
					list[i].index = i;
				}
				return list;
			}())), m_idToIndex(std::move([&]() {
				std::unordered_map<size_t, size_t> map;
				for (size_t i = 0; i < bindings.size(); i++)
					map[bindings[i].binding] = i;
				return map;
			}())), m_nameToIndex(std::move([&]() {
				std::unordered_map<std::string, size_t> map;
				for (size_t i = 0; i < bindings.size(); i++)
					map[bindings[i].name] = i;
				return map;
			}())) {}



		Reference<SPIRV_Binary> SPIRV_Binary::FromSPV(const OS::Path& filename, OS::Logger* logger) {
			Reference<OS::MMappedFile> mappedFile = OS::MMappedFile::Create(filename, logger);
			if (mappedFile == nullptr) return nullptr;
			return FromData(*mappedFile, logger);
		}

		namespace {
			class SPIRV_Binary_Cache : public virtual ObjectCache<OS::Path> {
			public:
				static Reference<SPIRV_Binary> GetInstance(const OS::Path& filename, OS::Logger* logger, bool keepAlive) {
					static SPIRV_Binary_Cache cache;
					Reference<SPIRV_Binary> binary = cache.GetCachedOrCreate(filename, 
						[&]() ->Reference<SPIRV_Binary> { return SPIRV_Binary::FromSPV(filename, logger); });
					if (binary != nullptr && keepAlive) {
						std::mutex keepAliveLock;
						static std::unordered_set<Reference<SPIRV_Binary>> keepAliveSet;
						std::unique_lock<std::mutex> lock(keepAliveLock);
						keepAliveSet.insert(binary);
					}
					return binary;
				}
			};
		}

		Reference<SPIRV_Binary> SPIRV_Binary::FromSPVCached(const OS::Path& filename, OS::Logger* logger, bool keepAlive) {
			return SPIRV_Binary_Cache::GetInstance(filename, logger, keepAlive);
		}

		Reference<SPIRV_Binary> SPIRV_Binary::FromData(const MemoryBlock& data, OS::Logger* logger) {
			SpvReflectShaderModule spvModule;
			SpvReflectResult spvResult = spvReflectCreateShaderModule(data.Size(), data.Data(), &spvModule);
			if (spvResult != SPV_REFLECT_RESULT_SUCCESS) {
				if (logger != nullptr)
					logger->Error("SPIRV_Binary::FromData - spvReflectCreateShaderModule failed with code ", static_cast<int>(spvResult), "!");
				return nullptr;
			}
			
			std::string entryPoint(spvModule.entry_point_name);
			PipelineStageMask stageMask = 
				(((spvModule.shader_stage & SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT) != 0) ? PipelineStage::COMPUTE : PipelineStage::NONE) |
				(((spvModule.shader_stage & SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) != 0) ? PipelineStage::VERTEX : PipelineStage::NONE) |
				(((spvModule.shader_stage & SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT) != 0) ? PipelineStage::FRAGMENT : PipelineStage::NONE);
			
			static thread_local std::vector<const SpvReflectDescriptorSet*> sets;
			sets.clear();
			size_t numSets = 0;
			for (size_t i = 0; i < spvModule.descriptor_set_count; i++) {
				const SpvReflectDescriptorSet& set = spvModule.descriptor_sets[i];
				if (numSets <= set.set) {
					numSets = static_cast<size_t>(set.set) + 1u;
					if (sets.size() < numSets)
						sets.resize(numSets);
				}
				sets[set.set] = &set;
			}

			std::vector<BindingSetInfo> bindingSets;
			for (size_t i = 0; i < numSets; i++) {
				const SpvReflectDescriptorSet* set = sets[i];
				if (set == nullptr) {
					bindingSets.push_back(BindingSetInfo(i, std::vector<BindingInfo>()));
					continue;
				}
				static thread_local std::vector<BindingInfo> setBindings;
				setBindings.resize(set->binding_count);
				for (size_t bindingId = 0; bindingId < set->binding_count; bindingId++) {
					const SpvReflectDescriptorBinding* binding = set->bindings[bindingId];
					BindingInfo& info = setBindings[bindingId];
					info.name = binding->name;
					info.binding = binding->binding;
					if (binding->type_description == nullptr || (binding->type_description->op != SpvOpTypeArray && binding->type_description->op != SpvOpTypeRuntimeArray))
						info.type = (
							(binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ? BindingInfo::Type::CONSTANT_BUFFER :
							(binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ? BindingInfo::Type::TEXTURE_SAMPLER :
							(binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE) ? BindingInfo::Type::STORAGE_TEXTURE :
							(binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER) ? BindingInfo::Type::STRUCTURED_BUFFER :
							(binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) ? BindingInfo::Type::ACCELERATION_STRUCTURE :
							BindingInfo::Type::UNKNOWN);
					else info.type = (
						(binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) ? BindingInfo::Type::CONSTANT_BUFFER_ARRAY :
						(binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ? BindingInfo::Type::TEXTURE_SAMPLER_ARRAY :
						(binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE) ? BindingInfo::Type::STORAGE_TEXTURE_ARRAY :
						(binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER) ? BindingInfo::Type::STRUCTURED_BUFFER_ARRAY :
						(binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR) ? BindingInfo::Type::ACCELERATION_STRUCTURE_ARRAY :
						BindingInfo::Type::UNKNOWN);
				}
				bindingSets.push_back(std::move(BindingSetInfo(set->set, setBindings)));
			}

			std::vector<ShaderInputInfo> shaderInputs;
			for (size_t i = 0u; i < spvModule.input_variable_count; i++) {
				const SpvReflectInterfaceVariable* variable = spvModule.input_variables[i];
				if (variable->built_in >= 0 && variable->built_in < SpvBuiltInMax) continue;
				ShaderInputInfo inputInfo = {};
				inputInfo.name = variable->name;
				inputInfo.location = variable->location;
				inputInfo.index = shaderInputs.size();
				const SpvReflectFormat format = variable->format;
				const uint32_t columnCount = variable->numeric.matrix.column_count;

				inputInfo.format =
					(format == SPV_REFLECT_FORMAT_R32_SFLOAT) ? ShaderInputInfo::Type::FLOAT :
					(format == SPV_REFLECT_FORMAT_R32G32_SFLOAT) ? 
						((columnCount != 2u) ? ShaderInputInfo::Type::FLOAT2 : ShaderInputInfo::Type::MAT_2X2) :
					(format == SPV_REFLECT_FORMAT_R32G32B32_SFLOAT) ? 
						((columnCount != 3u) ? ShaderInputInfo::Type::FLOAT3 : ShaderInputInfo::Type::MAT_3X3) :
					(format == SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT) ? 
						((columnCount != 4u) ? ShaderInputInfo::Type::FLOAT4 : ShaderInputInfo::Type::MAT_4X4) :

					(format == SPV_REFLECT_FORMAT_R32_SINT) ? ShaderInputInfo::Type::INT :
					(format == SPV_REFLECT_FORMAT_R32G32_SINT) ? ShaderInputInfo::Type::INT2 :
					(format == SPV_REFLECT_FORMAT_R32G32B32_SINT) ? ShaderInputInfo::Type::INT3 :
					(format == SPV_REFLECT_FORMAT_R32G32B32A32_SINT) ? ShaderInputInfo::Type::INT4 :

					(format == SPV_REFLECT_FORMAT_R32_UINT) ? ShaderInputInfo::Type::UINT :
					(format == SPV_REFLECT_FORMAT_R32G32_UINT) ? ShaderInputInfo::Type::UINT2 :
					(format == SPV_REFLECT_FORMAT_R32G32B32_UINT) ? ShaderInputInfo::Type::UINT3 :
					(format == SPV_REFLECT_FORMAT_R32G32B32A32_UINT) ? ShaderInputInfo::Type::UINT4 :

					ShaderInputInfo::Type::TYPE_COUNT;

				shaderInputs.push_back(inputInfo);
			}
			
			spvReflectDestroyShaderModule(&spvModule);
			Reference<SPIRV_Binary> reference = new SPIRV_Binary(data, std::move(entryPoint), stageMask, std::move(bindingSets), std::move(shaderInputs), logger);
			reference->ReleaseRef();
			return reference;
		}

		SPIRV_Binary::~SPIRV_Binary() {}


		const void* SPIRV_Binary::Bytecode()const { return (void*)m_bytecode.Data(); }

		size_t SPIRV_Binary::BytecodeSize()const { return m_bytecode.Size(); }

		const std::string& SPIRV_Binary::EntryPoint()const { return m_entryPoint; }

		PipelineStageMask SPIRV_Binary::ShaderStages()const { return m_stageMask; }

		size_t SPIRV_Binary::BindingSetCount()const { return m_bindingSets.size(); }

		const SPIRV_Binary::BindingSetInfo& SPIRV_Binary::BindingSet(size_t index)const { return m_bindingSets[index]; }

		bool SPIRV_Binary::FindBinding(const std::string_view& bindingName, const BindingInfo*& binding)const {
			std::unordered_map<std::string_view, std::pair<size_t, size_t>>::const_iterator it = m_bindingNameToSetIndex.find(bindingName);
			if (it == m_bindingNameToSetIndex.end()) return false;
			binding = &m_bindingSets[it->second.first].Binding(it->second.second);
			return true;
		}

		size_t SPIRV_Binary::ShaderInputCount()const { return m_shaderInputs.size(); }

		const SPIRV_Binary::ShaderInputInfo& SPIRV_Binary::ShaderInput(size_t index)const { return m_shaderInputs[index]; }

		bool SPIRV_Binary::FindShaderInput(const std::string_view& inputName, const ShaderInputInfo*& input)const {
			std::unordered_map<std::string_view, size_t>::const_iterator it = m_shaderInputNameIndex.find(inputName);
			if (it == m_shaderInputNameIndex.end()) return false;
			input = &m_shaderInputs[it->second];
			return true;
		}


		SPIRV_Binary::SPIRV_Binary(
			const MemoryBlock& bytecode,
			std::string&& entryPoint,
			PipelineStageMask stageMask,
			std::vector<BindingSetInfo>&& bindingSets,
			std::vector<ShaderInputInfo>&& shaderInputs,
			OS::Logger* logger)
			: m_logger(logger)
			, m_bytecode(bytecode)
			, m_entryPoint(std::move(entryPoint))
			, m_stageMask(stageMask)
			, m_bindingSets(std::move(bindingSets))
			, m_shaderInputs(std::move(shaderInputs)) {
			for (size_t i = 0u; i < m_bindingSets.size(); i++) {
				const BindingSetInfo& info = m_bindingSets[i];
				for (size_t j = 0u; j < info.BindingCount(); j++)
					m_bindingNameToSetIndex[info.Binding(j).name] = std::make_pair(i, j);
			}
			for (size_t i = 0u; i < m_shaderInputs.size(); i++)
				m_shaderInputNameIndex[m_shaderInputs[i].name] = i;
		}

		std::ostream& operator<<(std::ostream& stream, const SPIRV_Binary& binary) {
			stream << "SPIRV_Binary::SPIRV_Binary:" << std::endl <<
				"    m_entryPoint = \"" << binary.m_entryPoint << "\"" << std::endl <<
				"    m_stageMask = " <<
				(binary.m_stageMask == StageMask(PipelineStage::NONE) ? std::string("NONE")
					: (std::string(((binary.m_stageMask & PipelineStage::COMPUTE) != PipelineStage::NONE) ? "COMPUTE " : "") +
						std::string(((binary.m_stageMask & PipelineStage::VERTEX) != PipelineStage::NONE) ? "VERTEX " : "") +
						std::string(((binary.m_stageMask & PipelineStage::FRAGMENT) != PipelineStage::NONE) ? "FRAGMENT " : ""))) << std::endl <<
				"    m_bindingSets = [" << std::endl;
			for (size_t i = 0u; i < binary.m_bindingSets.size(); i++) {
				const SPIRV_Binary::BindingSetInfo& set = binary.m_bindingSets[i];
				stream << "        " << i << ". set " << set.Id() << ": {" << std::endl;
				for (size_t j = 0u; j < set.BindingCount(); j++) {
					const SPIRV_Binary::BindingInfo& info = set.Binding(j);
					stream << "            <binding:" << info.binding << "; name:\"" << info.name << "\"; type:" << (
						(info.type == SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER) ? "CONSTANT_BUFFER" :
						(info.type == SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER) ? "TEXTURE_SAMPLER" :
						(info.type == SPIRV_Binary::BindingInfo::Type::STORAGE_TEXTURE) ? "STORAGE_TEXTURE" :
						(info.type == SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER) ? "STRUCTURED_BUFFER" :
						(info.type == SPIRV_Binary::BindingInfo::Type::ACCELERATION_STRUCTURE) ? "ACCELERATION_STRUCTURE" :
						(info.type == SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER_ARRAY) ? "CONSTANT_BUFFER_ARRAY" :
						(info.type == SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER_ARRAY) ? "TEXTURE_SAMPLER_ARRAY" :
						(info.type == SPIRV_Binary::BindingInfo::Type::STORAGE_TEXTURE_ARRAY) ? "STORAGE_TEXTURE_ARRAY" :
						(info.type == SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER_ARRAY) ? "STRUCTURED_BUFFER_ARRAY" :
						(info.type == SPIRV_Binary::BindingInfo::Type::ACCELERATION_STRUCTURE_ARRAY) ? "ACCELERATION_STRUCTURE_ARRAY" :
						"UNKNOWN") << ">" << std::endl;
				}
				stream << "        }" << std::endl;
			}
			stream << "    ]" << std::endl <<
				"    m_shaderInputs: [" << std::endl;
			for (size_t i = 0u; i < binary.m_shaderInputs.size(); i++) {
				const SPIRV_Binary::ShaderInputInfo& info = binary.m_shaderInputs[i];
				stream << "        <location: " << info.location << "; name: " << info.name << "; type:" << (
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::FLOAT) ? "FLOAT" :
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::FLOAT2) ? "FLOAT2" :
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::FLOAT3) ? "FLOAT3" :
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::FLOAT4) ? "FLOAT4" :
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::INT) ? "INT" :
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::INT2) ? "INT2" :
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::INT3) ? "INT3" :
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::INT4) ? "INT4" :
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::UINT) ? "UINT" :
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::UINT2) ? "UINT2" :
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::UINT3) ? "UINT3" :
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::UINT4) ? "UINT4" :
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::BOOL32) ? "BOOL32" :
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::MAT_2X2) ? "MAT_2X2" :
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::MAT_3X3) ? "MAT_3X3" :
					(info.format == SPIRV_Binary::ShaderInputInfo::Type::MAT_4X4) ? "MAT_4X4" :
					"UNKNOWN") << ">" << std::endl;
			}
			stream << "    ]" << std::endl;
			return stream;
		}
	}
}
#pragma warning(default: 26812)
