#include "ShaderResourceBindings.h"
#include <string_view>
#include <optional>


namespace Jimara {
	namespace Graphics {
		namespace Legacy {
		namespace ShaderResourceBindings {
			namespace {
				template<typename BindingType>
				const NamedShaderBinding<BindingType>* FindBinding(const NamedShaderBinding<BindingType>* const* bindings, size_t count, const std::string_view& name) {
					if (bindings == nullptr) return nullptr;
					for (size_t i = 0; i < count; i++) {
						const NamedShaderBinding<BindingType>* binding = bindings[i];
						if (binding->BindingName() == name) return binding;
					}
					return nullptr;
				}
			}

			Reference<const ConstantBufferBinding> ShaderBindingDescription::FindConstantBufferBinding(const std::string_view& name)const {
				return FindBinding(constantBufferBindings, constantBufferBindingCount, name);
			}

			Reference<const StructuredBufferBinding> ShaderBindingDescription::FindStructuredBufferBinding(const std::string_view& name)const {
				return FindBinding(structuredBufferBindings, structuredBufferBindingCount, name);
			}

			Reference<const TextureSamplerBinding> ShaderBindingDescription::FindTextureSamplerBinding(const std::string_view& name)const {
				return FindBinding(textureSamplerBindings, textureSamplerBindingCount, name);
			}

			Reference<const TextureViewBinding> ShaderBindingDescription::FindTextureViewBinding(const std::string_view& name)const {
				return FindBinding(textureViewBindings, textureViewBindingCount, name);
			}

			Reference<const BindlessStructuredBufferSetBinding> ShaderBindingDescription::FindBindlessStructuredBufferSetBinding(const std::string_view& name)const {
				return FindBinding(bindlessStructuredBufferBindings, bindlessStructuredBufferBindingCount, name);
			}

			Reference<const BindlessTextureSamplerSetBinding> ShaderBindingDescription::FindBindlessTextureSamplerSetBinding(const std::string_view& name)const {
				return FindBinding(bindlessTextureSamplerBindings, bindlessTextureSamplerBindingCount, name);
			}

			Reference<const BindlessTextureViewSetBinding> ShaderBindingDescription::FindBindlessTextureViewSetBinding(const std::string_view& name)const {
				return FindBinding(bindlessTextureViewBindings, bindlessTextureViewBindingCount, name);
			}

			bool GenerateShaderBindings(
				const SPIRV_Binary* const* shaderBinaries, size_t shaderBinaryCount,
				const ShaderResourceBindingSet& bindings,
				Callback<const BindingSetInfo&> addDescriptor,
				OS::Logger* logger) {
				static thread_local std::vector<ShaderModuleBindingSet> moduleBindingSets;
				moduleBindingSets.clear();
				for (size_t i = 0; i < shaderBinaryCount; i++) {
					const SPIRV_Binary* binary = shaderBinaries[i];
					PipelineStageMask stage = binary->ShaderStages();
					for (size_t j = 0; j < binary->BindingSetCount(); j++)
						moduleBindingSets.push_back(ShaderModuleBindingSet(&binary->BindingSet(j), stage));
				}
				return GenerateShaderBindings(moduleBindingSets.data(), moduleBindingSets.size(), bindings, addDescriptor, logger);
			}

			namespace {
				typedef std::pair<PipelineDescriptor::BindingSetDescriptor::BindingInfo, Reference<const ConstantBufferBinding>> ConstantBufferBindingInfo;
				typedef std::pair<PipelineDescriptor::BindingSetDescriptor::BindingInfo, Reference<const StructuredBufferBinding>> StructuredBufferBindingInfo;
				typedef std::pair<PipelineDescriptor::BindingSetDescriptor::BindingInfo, Reference<const TextureSamplerBinding>> TextureSamplerBindingInfo;
				typedef std::pair<PipelineDescriptor::BindingSetDescriptor::BindingInfo, Reference<const TextureViewBinding>> TextureViewBindingInfo;

				enum class BindingSetFlags : uint8_t {
					NONE = 0,
					IS_STRUCTURED_BUFFER_ARRAY = 1 << 0,
					IS_TEXTURE_SAMPLER_ARRAY = 1 << 1,
					IS_TEXTURE_VIEW_ARRAY = 1 << 2
				};

				class BindingSetDescriptor : public virtual PipelineDescriptor::BindingSetDescriptor {
				private:
					template<typename BindingType>
					struct BindingInformation {
						Reference<const BindingType> binding;
						PipelineDescriptor::BindingSetDescriptor::BindingInfo info;

						typedef std::pair<PipelineDescriptor::BindingSetDescriptor::BindingInfo, Reference<const BindingType>> BindingInput;

						inline BindingInformation(const BindingInput& input = BindingInput({}, nullptr)) : binding(input.second), info(input.first) {}

						static std::vector<BindingInformation> MakeList(const std::vector<BindingInput>& input) {
							std::vector<BindingInformation> list(input.size());
							for (size_t i = 0; i < input.size(); i++)
								list[i] = BindingInformation(input[i]);
							return list;
						}
					};

					const std::vector<BindingInformation<ConstantBufferBinding>> m_constantBuffers;
					const std::vector<BindingInformation<StructuredBufferBinding>> m_structuredBuffers;
					const std::vector<BindingInformation<TextureSamplerBinding>> m_textureSamplers;
					const std::vector<BindingInformation<TextureViewBinding>> m_textureViews;
					const Reference<const BindlessStructuredBufferSetBinding> m_bindlessBufferSet;
					const Reference<const BindlessTextureSamplerSetBinding> m_bindlessSamplerSet;
					const BindingSetFlags m_flags;


				public:
					inline BindingSetDescriptor(
						const std::vector<ConstantBufferBindingInfo>& constantBuffers,
						const std::vector<StructuredBufferBindingInfo>& structuredBuffers,
						const std::vector<TextureSamplerBindingInfo>& textureSamplers,
						const std::vector<TextureViewBindingInfo>& textureViews,
						const BindlessStructuredBufferSetBinding* bindlessBufferSet,
						const BindlessTextureSamplerSetBinding* bindlessSamplerSet,
						BindingSetFlags flags)
						: m_constantBuffers(BindingInformation<ConstantBufferBinding>::MakeList(constantBuffers))
						, m_structuredBuffers(BindingInformation<StructuredBufferBinding>::MakeList(structuredBuffers))
						, m_textureSamplers(BindingInformation<TextureSamplerBinding>::MakeList(textureSamplers))
						, m_textureViews(BindingInformation<TextureViewBinding>::MakeList(textureViews))
						, m_bindlessBufferSet(bindlessBufferSet), m_bindlessSamplerSet(bindlessSamplerSet), m_flags(flags) {}

					inline virtual bool SetByEnvironment()const override { return false; }

					inline virtual size_t ConstantBufferCount()const override { return m_constantBuffers.size(); }
					inline virtual BindingInfo ConstantBufferInfo(size_t index)const override { return m_constantBuffers[index].info; }
					inline virtual Reference<Buffer> ConstantBuffer(size_t index)const override { return m_constantBuffers[index].binding->BoundObject(); }

					inline virtual size_t StructuredBufferCount()const override { return m_structuredBuffers.size(); }
					inline virtual BindingInfo StructuredBufferInfo(size_t index)const override { return m_structuredBuffers[index].info; }
					inline virtual Reference<ArrayBuffer> StructuredBuffer(size_t index)const override { return m_structuredBuffers[index].binding->BoundObject(); }

					inline virtual size_t TextureSamplerCount()const override { return m_textureSamplers.size(); }
					inline virtual BindingInfo TextureSamplerInfo(size_t index)const override { return m_textureSamplers[index].info; }
					inline virtual Reference<TextureSampler> Sampler(size_t index)const override { return m_textureSamplers[index].binding->BoundObject(); }

					inline virtual size_t TextureViewCount()const override { return m_textureViews.size(); }
					inline virtual BindingInfo TextureViewInfo(size_t index)const override { return m_textureViews[index].info; }
					inline virtual Reference<TextureView> View(size_t index)const override { return m_textureViews[index].binding->BoundObject(); }

					inline virtual bool IsBindlessArrayBufferArray()const override { return (static_cast<uint8_t>(m_flags) & static_cast<uint8_t>(BindingSetFlags::IS_STRUCTURED_BUFFER_ARRAY)) != 0; }
					inline virtual Reference<BindlessSet<ArrayBuffer>::Instance> BindlessArrayBuffers()const override { return m_bindlessBufferSet->BoundObject(); }

					inline virtual bool IsBindlessTextureSamplerArray()const override { return (static_cast<uint8_t>(m_flags) & static_cast<uint8_t>(BindingSetFlags::IS_TEXTURE_SAMPLER_ARRAY)) != 0; }
					inline virtual Reference<BindlessSet<TextureSampler>::Instance> BindlessTextureSamplers()const override { return m_bindlessSamplerSet->BoundObject(); }
				};
			}

			bool GenerateShaderBindings(
				const ShaderModuleBindingSet* binaryBindingSets, size_t bindingSetCount,
				const ShaderResourceBindingSet& bindings,
				const Callback<const BindingSetInfo&>& addDescriptor,
				OS::Logger* logger) {

				// Sort binaryBindingSets by set id-s (setEntries holds linked lists of ShaderModuleBindingSet-s from bindingSetEntries per binding set id):
				typedef std::pair<const ShaderModuleBindingSet*, std::optional<size_t>> BindingSetEntry;
				static thread_local std::vector<BindingSetEntry> bindingSetEntries;
				const std::vector<std::optional<size_t>>& setEntries = [&]() -> const std::vector<std::optional<size_t>>&{
					static thread_local std::vector<std::optional<size_t>> entries;
					bindingSetEntries.clear();
					entries.clear();
					for (size_t i = 0; i < bindingSetCount; i++) {
						const ShaderModuleBindingSet* setIfo = (binaryBindingSets + i);
						const size_t setId = setIfo->set->Id();
						if (entries.size() <= setId) entries.resize(setId + 1);
						std::optional<size_t>& entry = entries[setId];
						size_t lastIndex = bindingSetEntries.size();
						bindingSetEntries.push_back(BindingSetEntry(setIfo, entry));
						entry = lastIndex;
					}
					return entries;
				}();


				// Extract binding infos per set (bindingsPerSet holds mappings from binding id to a linked list of bindings from bindingEntries per binding set id):
				typedef std::pair<std::pair<const SPIRV_Binary::BindingInfo*, PipelineStageMask>, std::optional<size_t>> BindingEntry;
				static thread_local std::vector<BindingEntry> bindingEntries;
				const std::unordered_map<size_t, std::optional<size_t>>* const bindingsPerSet = [&]()-> const std::unordered_map<size_t, std::optional<size_t>>*{
					static thread_local std::vector<std::unordered_map<size_t, std::optional<size_t>>> entriesPerSet;
					bindingEntries.clear();
					entriesPerSet.clear();
					entriesPerSet.resize(setEntries.size());
					for (size_t setId = 0; setId < setEntries.size(); setId++) {
						std::unordered_map<size_t, std::optional<size_t>>& entries = entriesPerSet[setId];
						const std::optional<size_t>* ptr = &setEntries[setId];
						while (ptr->has_value()) {
							const BindingSetEntry& setEntry = bindingSetEntries[ptr->value()];
							const SPIRV_Binary::BindingSetInfo* set = setEntry.first->set;
							const PipelineStageMask stage = setEntry.first->stages;
							for (size_t bindingId = 0; bindingId < set->BindingCount(); bindingId++) {
								const SPIRV_Binary::BindingInfo* binding = &set->Binding(bindingId);
								std::optional<size_t>& entry = entries[binding->binding];
								size_t lastIndex = bindingEntries.size();
								bindingEntries.push_back(BindingEntry(std::make_pair(binding, stage), entry));
								entry = lastIndex;
							}
							ptr = &setEntry.second;
						}
					}
					return entriesPerSet.data();
				}();

				// Build actual descriptors:
				static thread_local std::vector<BindingSetInfo> descriptors;
				for (size_t setId = 0; setId < setEntries.size(); setId++) {
					if (!setEntries[setId].has_value()) continue;

					// Temporary storage for found resource binding information for given set:
					static thread_local std::vector<ConstantBufferBindingInfo> constantBufferBindingInfos;
					static thread_local std::vector<StructuredBufferBindingInfo> structuredBufferBindingInfos;
					static thread_local std::vector<TextureSamplerBindingInfo> textureSamplerBindingInfos;
					static thread_local std::vector<TextureViewBindingInfo> textureViewBindingInfos;
					static thread_local Reference<const BindlessStructuredBufferSetBinding> bindlessStructuredBufferSet;
					static thread_local Reference<const BindlessTextureSamplerSetBinding> bindlessTextureSamplerSet;
					static thread_local Reference<const BindlessTextureViewSetBinding> bindlessTextureViewSet;
					static thread_local BindingSetFlags bindingSetFlags;
					bindingSetFlags = BindingSetFlags::NONE;

					// We need to clear the temporary buffers..
					auto clearBuffers = [&]() {
						constantBufferBindingInfos.clear();
						structuredBufferBindingInfos.clear();
						textureSamplerBindingInfos.clear();
						textureViewBindingInfos.clear();
						bindlessStructuredBufferSet = nullptr;
						bindlessTextureSamplerSet = nullptr;
						bindlessTextureViewSet = nullptr;
					};
					clearBuffers();

					// Counters for found/missing bindings for the set:
					size_t bindingsFound = 0;
					size_t bindingsMissing = 0;

					// Analize all bindings for the set:
					const std::unordered_map<size_t, std::optional<size_t>>& entries = bindingsPerSet[setId];
					for (std::unordered_map<size_t, std::optional<size_t>>::const_iterator it = entries.begin(); it != entries.end(); ++it) {
						// Define binding info to fill in:
						PipelineDescriptor::BindingSetDescriptor::BindingInfo bindingInfo = {};
						bindingInfo.stages = PipelineStage::NONE;
						bindingInfo.binding = static_cast<uint32_t>(it->first);

						// Find binding stages and type:
						SPIRV_Binary::BindingInfo::Type bindingType = SPIRV_Binary::BindingInfo::Type::UNKNOWN;
						for (const std::optional<size_t>* ptr = &it->second; ptr->has_value(); ptr = &bindingEntries[ptr->value()].second) {
							const BindingEntry& entry = bindingEntries[ptr->value()];
							bindingInfo.stages |= entry.first.second;
							SPIRV_Binary::BindingInfo::Type type = entry.first.first->type;
							if (type != SPIRV_Binary::BindingInfo::Type::UNKNOWN) {
								if (bindingType == SPIRV_Binary::BindingInfo::Type::UNKNOWN)
									bindingType = type;
								else if (bindingType != type) {
									if (logger != nullptr)
										logger->Error(
											"Jimara::Graphics::ShaderResourceBindings::GenerateShaderBindings - Type mismatch for binding ",
											bindingInfo.binding, " of set ", setId, "!");
									descriptors.clear();
									clearBuffers();
									return false;
								}
							}
						}

						// Report an error if binding could not be found:
						if ((bindingType == SPIRV_Binary::BindingInfo::Type::UNKNOWN) || (bindingType >= SPIRV_Binary::BindingInfo::Type::TYPE_COUNT)) {
							if (logger != nullptr)
								logger->Error("Jimara::Graphics::ShaderResourceBindings::GenerateShaderBindings - Type unknown for ", bindingInfo.binding, " of set ", setId, "!");
							descriptors.clear();
							clearBuffers();
							return false;
						}

						// Try to find and record resource binding informations:
						typedef bool (*RecordBindingFn)(const std::string&, const PipelineDescriptor::BindingSetDescriptor::BindingInfo&, const ShaderResourceBindingSet&);
						static const RecordBindingFn* const RECORD_FN = []() -> const RecordBindingFn* {
							static const uint8_t TYPE_COUNT = static_cast<uint8_t>(SPIRV_Binary::BindingInfo::Type::TYPE_COUNT);
							static RecordBindingFn functions[TYPE_COUNT];
							for (uint8_t i = 0; i < TYPE_COUNT; i++)
								functions[i] = [](const std::string&, const PipelineDescriptor::BindingSetDescriptor::BindingInfo&, const ShaderResourceBindingSet&) -> bool { return false; };
							functions[static_cast<uint8_t>(SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER)] =
								[](const std::string& name, const PipelineDescriptor::BindingSetDescriptor::BindingInfo& info, const ShaderResourceBindingSet& bindings) -> bool {
								Reference<const ConstantBufferBinding> binding = bindings.FindConstantBufferBinding(name);
								if (binding == nullptr) return false;
								constantBufferBindingInfos.push_back(std::make_pair(info, binding));
								return true;
							};
							functions[static_cast<uint8_t>(SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER)] =
								[](const std::string& name, const PipelineDescriptor::BindingSetDescriptor::BindingInfo& info, const ShaderResourceBindingSet& bindings) -> bool {
								Reference<const StructuredBufferBinding> binding = bindings.FindStructuredBufferBinding(name);
								if (binding == nullptr) return false;
								structuredBufferBindingInfos.push_back(std::make_pair(info, binding));
								return true;
							};
							functions[static_cast<uint8_t>(SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER)] =
								[](const std::string& name, const PipelineDescriptor::BindingSetDescriptor::BindingInfo& info, const ShaderResourceBindingSet& bindings) -> bool {
								Reference<const TextureSamplerBinding> binding = bindings.FindTextureSamplerBinding(name);
								if (binding == nullptr) return false;
								textureSamplerBindingInfos.push_back(std::make_pair(info, binding));
								return true;
							};
							functions[static_cast<uint8_t>(SPIRV_Binary::BindingInfo::Type::STORAGE_TEXTURE)] =
								[](const std::string& name, const PipelineDescriptor::BindingSetDescriptor::BindingInfo& info, const ShaderResourceBindingSet& bindings) -> bool {
								Reference<const TextureViewBinding> binding = bindings.FindTextureViewBinding(name);
								if (binding == nullptr) return false;
								textureViewBindingInfos.push_back(std::make_pair(info, binding));
								return true;
							};
							functions[static_cast<uint8_t>(SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER_ARRAY)] =
								[](const std::string& name, const PipelineDescriptor::BindingSetDescriptor::BindingInfo& info, const ShaderResourceBindingSet& bindings) -> bool {
								if (info.binding != 0 || bindlessStructuredBufferSet != nullptr) return false;
								bindingSetFlags = static_cast<BindingSetFlags>(static_cast<uint8_t>(bindingSetFlags) | static_cast<uint8_t>(BindingSetFlags::IS_STRUCTURED_BUFFER_ARRAY));
								bindlessStructuredBufferSet = bindings.FindBindlessStructuredBufferSetBinding(name);
								return (bindlessStructuredBufferSet != nullptr);
							};
							functions[static_cast<uint8_t>(SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER_ARRAY)] =
								[](const std::string& name, const PipelineDescriptor::BindingSetDescriptor::BindingInfo& info, const ShaderResourceBindingSet& bindings) -> bool {
								if (info.binding != 0 || bindlessTextureSamplerSet != nullptr) return false;
								bindingSetFlags = static_cast<BindingSetFlags>(static_cast<uint8_t>(bindingSetFlags) | static_cast<uint8_t>(BindingSetFlags::IS_TEXTURE_SAMPLER_ARRAY));
								bindlessTextureSamplerSet = bindings.FindBindlessTextureSamplerSetBinding(name);
								return (bindlessTextureSamplerSet != nullptr);
							};
							functions[static_cast<uint8_t>(SPIRV_Binary::BindingInfo::Type::STORAGE_TEXTURE_ARRAY)] =
								[](const std::string& name, const PipelineDescriptor::BindingSetDescriptor::BindingInfo& info, const ShaderResourceBindingSet& bindings) -> bool {
								if (info.binding != 0 || bindlessTextureViewSet != nullptr) return false;
								bindingSetFlags = static_cast<BindingSetFlags>(static_cast<uint8_t>(bindingSetFlags) | static_cast<uint8_t>(BindingSetFlags::IS_TEXTURE_VIEW_ARRAY));
								bindlessTextureViewSet = bindings.FindBindlessTextureViewSetBinding(name);
								return (bindlessTextureViewSet != nullptr);
							};
							return functions;
						}();
						bool found = false;
						const RecordBindingFn recordFn = RECORD_FN[static_cast<uint8_t>(bindingType)];
						for (const std::optional<size_t>* ptr = &it->second; ptr->has_value(); ptr = &bindingEntries[ptr->value()].second)
							if (recordFn(bindingEntries[ptr->value()].first.first->name, bindingInfo, bindings)) {
								found = true;
								break;
							}
						if (found) bindingsFound++;
						else bindingsMissing++;
					}

					// Create binding set descriptor if no bindings are missing, 
					// ignore binding index if no bindings are found (ei consider 'set by environment') 
					// and report error if the bindings are partially present:
					if (bindingsMissing <= 0) {
						descriptors.push_back({
							Object::Instantiate<BindingSetDescriptor>(
								constantBufferBindingInfos, structuredBufferBindingInfos, 
								textureSamplerBindingInfos, textureViewBindingInfos,
								bindlessStructuredBufferSet, bindlessTextureSamplerSet, 
								bindingSetFlags),
							setId });
						clearBuffers();
					}
					else if (bindingsFound > 0) {
						if (logger != nullptr)
							logger->Error("Jimara::Graphics::ShaderResourceBindings::GenerateShaderBindings - Binding set ", setId, " incomplete!");
						descriptors.clear();
						clearBuffers();
						return false;
					}
				}

				// Notify the callback about the binding sets and return:
				for (size_t i = 0; i < descriptors.size(); i++)
					addDescriptor(descriptors[i]);
				descriptors.clear();
				return true;
			}
		}
	}
	}
}
