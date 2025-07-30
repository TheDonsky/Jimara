#include "SceneAccelerationStructures.h"


namespace Jimara {
	struct SceneAccelerationStructures::Helpers {
#pragma warning(disable: 4250)
		class Queues : public virtual Object {
			// TODO: Implement this crap!
		};

		struct DependencyCollector : public virtual Object {
			EventInstance<Callback<JobSystem::Job*>> collectionEvents;
		};

		struct BlasInstance final : public virtual Blas, public virtual ObjectCache<BlasDesc>::StoredObject {
			Reference<Graphics::BottomLevelAccelerationStructure> blas;
			std::atomic_bool initialized = false;

			inline static Reference<BlasInstance> Create(const BlasDesc& desc, SceneContext* context, Queues* queues) {
				auto fail = [&](const auto... message) {
					context->Log()->Error("SceneAccelerationStructures::Helpers::BlasInstance::Create - ", message...);
					return nullptr;
				};
				return fail("Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline static const BlasInstance* Self(const Blas* selfPtr) {
				return dynamic_cast<const BlasInstance*>(selfPtr);
			}
		};

		class BlasCache final : public virtual ObjectCache<BlasDesc> {
		public:
			inline Reference<BlasInstance> GetInstance(const BlasDesc& desc, SceneContext* context, Queues* queues) {
				return GetCachedOrCreate(desc, [&]() { return BlasInstance::Create(desc, context, queues); });
			}
		};

		struct Instance final : public virtual SceneAccelerationStructures, public virtual ObjectCache<Reference<SceneContext>>::StoredObject {
			const Reference<Queues> queues;
			const Reference<BlasCache> cache = Object::Instantiate<BlasCache>();
			const Reference<DependencyCollector> dependencyCollector = Object::Instantiate<DependencyCollector>();

			inline static Reference<Instance> Create(SceneContext* context) {
				auto fail = [&](const auto... message) {
					context->Log()->Error("SceneAccelerationStructures::Helpers::Instance::Create - ", message...);
					return nullptr;
				};
				return fail("Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline static Instance* Self(SceneAccelerationStructures* selfPtr) {
				return dynamic_cast<Instance*>(selfPtr);
			}
		};

		struct InstanceCache final : public virtual ObjectCache<Reference<SceneContext>> {
			static Reference<Instance> Get(SceneContext* context) {
				static InstanceCache cache;
				return cache.GetCachedOrCreate(context, [&]() { return Instance::Create(context); });
			}
		};
#pragma warning(default: 4250)
	};

	SceneAccelerationStructures::SceneAccelerationStructures() {}

	SceneAccelerationStructures::~SceneAccelerationStructures() {}

	Reference<SceneAccelerationStructures> SceneAccelerationStructures::Get(SceneContext* context) {
		if (context == nullptr)
			return nullptr;
		if (!context->Graphics()->Device()->PhysicalDevice()->HasFeatures(Graphics::PhysicalDevice::DeviceFeatures::RAY_TRACING)) {
			context->Log()->Error("SceneAccelerationStructures::Get - ",
				"Graphics device does not support hardware Ray-Tracing features! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		else return Helpers::InstanceCache::Get(context);
	}

	Reference<SceneAccelerationStructures::Blas> SceneAccelerationStructures::GetBlas(BlasDesc& desc) {
		Helpers::Instance* const self = Helpers::Instance::Self(this);
		return self->cache->GetInstance(desc, self->ObjectCacheKey(), self->queues);
	}

	Event<Callback<JobSystem::Job*>>& SceneAccelerationStructures::OnCollectBuildDependencies() {
		return Helpers::Instance::Self(this)->dependencyCollector->collectionEvents;
	}

	void SceneAccelerationStructures::CollectRebuildJobs(const Callback<JobSystem::Job*> report) {
		// __TODO__: Report simulation-jobs.
		// __TODO__: Report re-build jobs.
	}

	const SceneAccelerationStructures::BlasDesc& SceneAccelerationStructures::Blas::Descriptor()const {
		return Helpers::BlasInstance::Self(this)->ObjectCacheKey();
	}

	Graphics::BottomLevelAccelerationStructure* SceneAccelerationStructures::Blas::AcccelerationStructure()const {
		const Helpers::BlasInstance* const self = Helpers::BlasInstance::Self(this);
		return self->initialized.load() ? self->blas.operator->() : nullptr;
	}
}
