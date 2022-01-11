#include "SceneObjectCollection.h"


namespace Jimara {
#ifndef USE_REFACTORED_SCENE
namespace Refactor_TMP_Namespace {
#endif
	namespace {
		class Cache : public virtual ObjectCache<SceneCachedInstances::InstanceId> {
		public:
			inline static Reference<Object> Get(const SceneCachedInstances::InstanceId& instanceId, Reference<SceneCachedInstances::InstanceType>(*createFn)(Scene::LogicContext*)) {
				static Cache cache;
				return cache.GetCachedOrCreate(instanceId, false, [&]() -> Reference<SceneCachedInstances::InstanceType> {
					Reference<SceneCachedInstances::InstanceType> instance = createFn(instanceId.context);
					if (instance == nullptr) {
						instanceId.context->Log()->Error("SceneCachedInstances::GetObjectInstance - createFn failed to create an object!");
						return nullptr;
					}
					else if (!instanceId.typeId.CheckType(instance)) {
						instanceId.context->Log()->Error("SceneCachedInstances::GetObjectInstance - createFn created an object of an incompatible type!");
						return nullptr;
					}
					else return instance;
					});
			}
		};
	}

	namespace SceneCachedInstances {
		Reference<InstanceType> GetObjectInstance(const InstanceId& instanceId, Reference<InstanceType>(*createFn)(Scene::LogicContext*)) {
			if (instanceId.context == nullptr) return nullptr;
			else return Cache::Get(instanceId, createFn);
		}
	}
#ifndef USE_REFACTORED_SCENE
}
#endif
}
