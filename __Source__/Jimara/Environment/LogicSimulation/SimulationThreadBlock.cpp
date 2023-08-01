#include "SimulationThreadBlock.h"


namespace Jimara {
	Reference<SimulationThreadBlock> SimulationThreadBlock::GetFor(SceneContext* context) {
		if (context == nullptr)
			return nullptr;

#pragma warning(disable: 4250)
		class Instance : public virtual SimulationThreadBlock, public virtual ObjectCache<Reference<const Object>>::StoredObject {
		public:
			inline Instance() {}
			inline virtual ~Instance() {}
		};
#pragma warning(default: 4250)

		class InstanceCache : public virtual ObjectCache<Reference<const Object>> {
		public:
			static Reference<SimulationThreadBlock> Get(SceneContext* context) {
				static InstanceCache cache;
				static std::mutex creationLock;
				std::unique_lock<std::mutex> lock(creationLock);
				return cache.GetCachedOrCreate(context, false, [&]() {
					const Reference<Instance> instance = Object::Instantiate<Instance>();
					context->StoreDataObject(instance);
					return instance;
					});
			}
		};

		return InstanceCache::Get(context);
	}
}
