#include "LightmapperJobs.h"


namespace Jimara {
	Reference<LightmapperJobs> LightmapperJobs::GetInstance(SceneContext* context) {
		const Reference<LightDescriptor::Set> set = LightDescriptor::Set::GetInstance(context);
		return GetInstance(set);
	}

	Reference<LightmapperJobs> LightmapperJobs::GetInstance(LightDescriptor::Set* lightSet) {
		if (lightSet == nullptr) return nullptr;
#pragma warning(disable: 4250)
		struct Instance : public virtual LightmapperJobs, public virtual ObjectCache<Reference<const Object>>::StoredObject {
			inline Instance(LightDescriptor::Set* set) : LightmapperJobs(set->Context()) {}
			inline virtual ~Instance() {}
		};
		struct Cache : public virtual ObjectCache<Reference<const Object>> {
			inline static Reference<Instance> Get(LightDescriptor::Set* set) {
				static Cache cache;
				return cache.GetCachedOrCreate(set, [&]() -> Reference<Instance> { return Object::Instantiate<Instance>(set); });
			}
		};
#pragma warning(default: 4250)
		return Cache::Get(lightSet);
	}
}
