#include "MaterialInstanceCache.h"
#include "../../Math/Helpers.h"


namespace Jimara {
	Reference<const Material::Instance> MaterialInstanceCache::SharedInstance(
		Graphics::GraphicsDevice* device,
		Graphics::BindlessSet<Graphics::ArrayBuffer>* bindlessBuffers,
		Graphics::BindlessSet<Graphics::TextureSampler>* bindlessSamplers,
		const Material::LitShader* litShader,
		const Overrides* overrides) {
		struct Key {
			Reference<Graphics::GraphicsDevice> device;
			Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>> bindlessBuffers;
			Reference<Graphics::BindlessSet<Graphics::TextureSampler>> bindlessSamplers;
			Reference<const Material::LitShader> litShader;
			Reference<const Overrides> overrides;
			inline bool operator==(const Key& other)const {
				return
					device == other.device &&
					bindlessBuffers == other.bindlessBuffers &&
					bindlessSamplers == other.bindlessSamplers &&
					litShader == other.litShader &&
					overrides == other.overrides;
			}
		};
		struct Hash {
			inline size_t operator()(const Key& key)const {
				return MergeHashes(
					MergeHashes(
						std::hash<decltype(key.device)>()(key.device),
						std::hash<decltype(key.bindlessBuffers)>()(key.bindlessBuffers)),
					MergeHashes(
						std::hash<decltype(key.bindlessSamplers)>()(key.bindlessSamplers),
						std::hash<decltype(key.litShader)>()(key.litShader)),
					std::hash<decltype(key.overrides)>()(key.overrides));
			}
		};
		using CacheType = ObjectCache<Key, Hash>;
#pragma warning(disable: 4250)
		struct MaterialAsset : public virtual Asset::Of<Material::Instance>, public virtual CacheType::StoredObject {
			const Key key;
			inline MaterialAsset(const Key& k) : Asset(GUID::Generate()), key(k) {}
			inline virtual ~MaterialAsset() {}
			inline virtual Reference<Material::Instance> LoadItem() final override {
				const Reference<Material> mat = Object::Instantiate<Material>(key.device, key.bindlessBuffers, key.bindlessSamplers);
				{
					Material::Writer writer(mat);
					writer.SetShader(key.litShader);
					if (key.overrides != nullptr) {
						auto applyOverrides = [&](const auto& overrideList) {
							for (size_t i = 0u; i < overrideList.Size(); i++) {
								const auto& overrideVal = overrideList[i];
								const auto propertyId = key.litShader->PropertyIdByName(overrideVal.fieldName);
								if (propertyId != Material::NO_ID)
									writer.SetPropertyValue(overrideVal.fieldName, overrideVal.overrideValue);
							}
						};
						applyOverrides(key.overrides->fp32);
						applyOverrides(key.overrides->fp64);
						applyOverrides(key.overrides->int32);
						applyOverrides(key.overrides->uint32);
						applyOverrides(key.overrides->int64);
						applyOverrides(key.overrides->uint64);
						applyOverrides(key.overrides->bool32);
						applyOverrides(key.overrides->vec2);
						applyOverrides(key.overrides->vec3);
						applyOverrides(key.overrides->vec4);
						applyOverrides(key.overrides->mat4);
						applyOverrides(key.overrides->textures);
					}
				}
				Reference<Material::Instance> instance;
				{
					Material::Reader reader(mat);
					instance = reader.CreateSnapshot();
					assert(instance != nullptr);
					assert(instance->RefCount() == 1u);
				}
				return instance;
			}
		};
#pragma warning(default: 4250)
		struct Cache : public virtual CacheType {
			inline static Reference<const Material::Instance> GetFor(const Key& key) {
				static Cache cache;
				const Reference<MaterialAsset> asset = cache.GetCachedOrCreate(
					key, [&]() -> Reference<MaterialAsset> { return Object::Instantiate<MaterialAsset>(key); });
				return asset->Load();
			}
		};
		return Cache::GetFor(Key{
			device,
			bindlessBuffers,
			bindlessSamplers,
			litShader,
			overrides
			});
	}
}
