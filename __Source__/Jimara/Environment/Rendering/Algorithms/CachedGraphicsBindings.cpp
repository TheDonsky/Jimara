#include "CachedGraphicsBindings.h"


namespace Jimara {
	Reference<CachedGraphicsBindings> CachedGraphicsBindings::Create(
		const Graphics::SPIRV_Binary* const* shaders, size_t shaderCount,
		const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, OS::Logger* log) {
		const Reference<CachedGraphicsBindings> bindings = new CachedGraphicsBindings();
		bindings->ReleaseRef();

		size_t setCount = 0u;
		for (size_t shaderId = 0u; shaderId < shaderCount; shaderId++) {
			const Graphics::SPIRV_Binary* shader = shaders[shaderId];
			setCount = Math::Max(setCount, shader == nullptr ? 0u : shader->BindingSetCount());
		}

		bindings->m_sets.Resize(setCount);
		for (size_t setId = 0u; setId < setCount; setId++)
			if (!bindings->m_sets[setId].Build(shaders, shaderCount, setId, search, log))
				return nullptr;

		return bindings;
	}

	bool CachedGraphicsBindings::SetBindings::Build(
		const Graphics::SPIRV_Binary* const* shaders, size_t shaderCount, size_t setId,
		const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, OS::Logger* log) {
		static const auto tryFind = [](const Graphics::SPIRV_Binary::BindingInfo& info, auto& bindings, const auto& searchFn) {
			{
				const auto it = bindings.find(info.binding);
				if (it != bindings.end()) return;
			}
			{
				Graphics::BindingSet::BindingDescriptor desc = {};
				desc.set = info.set;
				desc.binding = info.binding;
				desc.name = info.name;
				const auto binding = searchFn(desc);
				if (binding == nullptr) return;
				bindings.insert(std::make_pair(desc.binding, binding));
			}
		};
		static const auto hasEntry = [](const Graphics::SPIRV_Binary::BindingInfo& info, const auto& bindings) {
			return bindings.find(info.binding) != bindings.end();
		};

		typedef void(*TryFindBinding)(const Graphics::SPIRV_Binary::BindingInfo&, const Graphics::BindingSet::Descriptor::BindingSearchFunctions&, SetBindings*);
		typedef bool(*Verify)(const Graphics::SPIRV_Binary::BindingInfo&, SetBindings*);
		struct TryFindAndVerify {
			TryFindBinding tryFind = nullptr;
			Verify verify = nullptr;
		};

		static const TryFindAndVerify* findByType = []() -> const TryFindAndVerify* {
			static TryFindAndVerify findFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::TYPE_COUNT)];

			findFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::CONSTANT_BUFFER)] = {
				[](const Graphics::SPIRV_Binary::BindingInfo& info, const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, SetBindings* self) {
					tryFind(info, self->constantBuffers, search.constantBuffer);
				}, [](const Graphics::SPIRV_Binary::BindingInfo& info, SetBindings* self) -> bool { return hasEntry(info, self->constantBuffers); }
			};

			findFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER)] = {
				[](const Graphics::SPIRV_Binary::BindingInfo& info, const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, SetBindings* self) {
					tryFind(info, self->structuredBuffers, search.structuredBuffer);
				}, [](const Graphics::SPIRV_Binary::BindingInfo& info, SetBindings* self) -> bool { return hasEntry(info, self->structuredBuffers); }
			};

			findFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER)] = {
				[](const Graphics::SPIRV_Binary::BindingInfo& info, const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, SetBindings* self) {
					tryFind(info, self->textureSamplers, search.textureSampler);
				}, [](const Graphics::SPIRV_Binary::BindingInfo& info, SetBindings* self) -> bool { return hasEntry(info, self->textureSamplers); }
			};

			findFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::STORAGE_TEXTURE)] = {
				[](const Graphics::SPIRV_Binary::BindingInfo& info, const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, SetBindings* self) {
					tryFind(info, self->textureViews, search.textureView);
				}, [](const Graphics::SPIRV_Binary::BindingInfo& info, SetBindings* self) -> bool { return hasEntry(info, self->textureViews); }
			};

			findFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::STRUCTURED_BUFFER_ARRAY)] = {
				[](const Graphics::SPIRV_Binary::BindingInfo& info, const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, SetBindings* self) {
					tryFind(info, self->bindlessBuffers, search.bindlessStructuredBuffers);
				}, [](const Graphics::SPIRV_Binary::BindingInfo& info, SetBindings* self) -> bool { return hasEntry(info, self->bindlessBuffers); }
			};

			findFunctions[static_cast<size_t>(Graphics::SPIRV_Binary::BindingInfo::Type::TEXTURE_SAMPLER_ARRAY)] = {
				[](const Graphics::SPIRV_Binary::BindingInfo& info, const Graphics::BindingSet::Descriptor::BindingSearchFunctions& search, SetBindings* self) {
					tryFind(info, self->bindlessSamplers, search.bindlessTextureSamplers);
				}, [](const Graphics::SPIRV_Binary::BindingInfo& info, SetBindings* self) -> bool { return hasEntry(info, self->bindlessSamplers); }
			};

			return findFunctions;
		}();

		for (size_t shaderId = 0u; shaderId < shaderCount; shaderId++) {
			const Graphics::SPIRV_Binary* shader = shaders[shaderId];
			if (shader == nullptr || shader->BindingSetCount() <= setId) continue;
			const Graphics::SPIRV_Binary::BindingSetInfo& setInfo = shader->BindingSet(setId);
			for (size_t bindingIndex = 0u; bindingIndex < setInfo.BindingCount(); bindingIndex++) {
				const Graphics::SPIRV_Binary::BindingInfo& info = setInfo.Binding(bindingIndex);
				if (info.type >= Graphics::SPIRV_Binary::BindingInfo::Type::TYPE_COUNT) continue;
				const TryFindAndVerify findFn = findByType[static_cast<size_t>(info.type)];
				if (findFn.tryFind != nullptr)
					findFn.tryFind(info, search, this);
			}
		}

		for (size_t shaderId = 0u; shaderId < shaderCount; shaderId++) {
			const Graphics::SPIRV_Binary* shader = shaders[shaderId];
			if (shader == nullptr || shader->BindingSetCount() <= setId) continue;
			const Graphics::SPIRV_Binary::BindingSetInfo& setInfo = shader->BindingSet(setId);
			for (size_t bindingIndex = 0u; bindingIndex < setInfo.BindingCount(); bindingIndex++) {
				const Graphics::SPIRV_Binary::BindingInfo& info = setInfo.Binding(bindingIndex);
				if (info.type >= Graphics::SPIRV_Binary::BindingInfo::Type::TYPE_COUNT) continue;
				const TryFindAndVerify findFn = findByType[static_cast<size_t>(info.type)];
				if (findFn.verify != nullptr && (!findFn.verify(info, this))) {
					if (log != nullptr)
						log->Error("SegmentTreeGenerationKernel::Helpers::SetBindings::Build - ",
							"Failed to find resource binding for set ", info.set, " binding ", info.binding,
							"! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}
			}
		}

		return true;
	}

	Graphics::BindingSet::Descriptor::BindingSearchFunctions CachedGraphicsBindings::SetBindings::SearchFunctions()const {
		static const auto find = [](const auto& set, const Graphics::BindingSet::BindingDescriptor& desc) {
			const auto it = set.find(desc.binding);
			return (it == set.end()) ? nullptr : it->second;
		};
		static const Find<Graphics::Buffer>::Fn constantBuffer =
			[](const SetBindings* self, const Graphics::BindingSet::BindingDescriptor& desc) { return find(self->constantBuffers, desc); };
		static const Find<Graphics::ArrayBuffer>::Fn structuredBuffer =
			[](const SetBindings* self, const Graphics::BindingSet::BindingDescriptor& desc) { return find(self->structuredBuffers, desc); };
		static const Find<Graphics::TextureSampler>::Fn textureSampler =
			[](const SetBindings* self, const Graphics::BindingSet::BindingDescriptor& desc) { return find(self->textureSamplers, desc); };
		static const Find<Graphics::TextureView>::Fn textureView =
			[](const SetBindings* self, const Graphics::BindingSet::BindingDescriptor& desc) { return find(self->textureViews, desc); };
		static const Find<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>::Fn bindlessStructuredBuffers =
			[](const SetBindings* self, const Graphics::BindingSet::BindingDescriptor& desc) { return find(self->bindlessBuffers, desc); };
		static const Find<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>::Fn bindlessTextureSamplers =
			[](const SetBindings* self, const Graphics::BindingSet::BindingDescriptor& desc) { return find(self->bindlessSamplers, desc); };

		Graphics::BindingSet::Descriptor::BindingSearchFunctions result = {};
		result.constantBuffer = Graphics::BindingSet::Descriptor::BindingSearchFn<Graphics::Buffer>(constantBuffer, this);
		result.structuredBuffer = Graphics::BindingSet::Descriptor::BindingSearchFn<Graphics::ArrayBuffer>(structuredBuffer, this);
		result.textureSampler = Graphics::BindingSet::Descriptor::BindingSearchFn<Graphics::TextureSampler>(textureSampler, this);
		result.textureView = Graphics::BindingSet::Descriptor::BindingSearchFn<Graphics::TextureView>(textureView, this);
		result.bindlessStructuredBuffers = Graphics::BindingSet::Descriptor::BindingSearchFn<
			Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>(bindlessStructuredBuffers, this);
		result.bindlessTextureSamplers = Graphics::BindingSet::Descriptor::BindingSearchFn<
			Graphics::BindlessSet<Graphics::TextureSampler>::Instance>(bindlessTextureSamplers, this);
		return result;
	}
}
