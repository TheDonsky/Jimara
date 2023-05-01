#include "SceneLightGrid.h"


namespace Jimara {
#pragma warning(disable: 4250)
	struct SceneLightGrid::Helpers {
		class Data : public virtual Object {
		private:
			struct WeakPtrObject : public virtual Object {
				mutable SpinLock lock;
				Data* data = nullptr;
			};
			const Reference<WeakPtrObject> m_weakPtrObject = Object::Instantiate<WeakPtrObject>();
			const Reference<const ViewportLightSet> m_lightSet;
			

		public:
			inline Data(const ViewportLightSet* lightSet) : m_lightSet(lightSet) {
				assert(m_lightSet != nullptr);
				m_weakPtrObject->data = this;
			}

			inline virtual ~Data() {
				assert(m_weakPtrObject->data == nullptr);
			}

			inline const Object* WeakPtr()const { return m_weakPtrObject; }

			inline static Reference<Data> StrongPtr(const Object* weakPtr) {
				const WeakPtrObject* const ptr = dynamic_cast<const WeakPtrObject*>(weakPtr);
				if (ptr == nullptr) return nullptr;
				std::unique_lock<SpinLock> lock(ptr->lock);
				const Reference<Data> data = ptr->data;
				return data;
			}

			inline const ViewportLightSet* LightSet()const { return m_lightSet; }

		protected:
			virtual void OnOutOfScope()const override {
				const Reference<WeakPtrObject> weakPtrObject(m_weakPtrObject);
				assert(weakPtrObject != nullptr);
				{
					std::unique_lock<SpinLock> lock(weakPtrObject->lock);
					weakPtrObject->data = nullptr;
					if (RefCount() > 0u) return;
				}
				Object::OnOutOfScope();
			}
		};

		class SharedInstance : public virtual SceneLightGrid, public virtual ObjectCache<Reference<const Object>>::StoredObject {
		private:
			const Reference<SceneContext> m_context;
			const Reference<const ViewportLightSet> m_lightSet;
			
			// Execute() should only occure once per frame and inside render jobs:
			std::atomic<uint32_t> m_canExecute = 1u;
			std::mutex m_updateLock;

			// m_localLigtIds and m_localLightBoundaries contains bounded lights and their boundaries, while m_globalLightIds stores indices of global lights:
			std::vector<uint32_t> m_localLigtIds;
			std::vector<AABB> m_localLightBoundaries;
			std::vector<uint32_t> m_globalLightIds;

			// Grid settings:
			struct GridSettings {
				alignas(16) Vector3 gridOrigin = Vector3(0.0f);
				alignas(16) Vector3 voxelSize = Vector3(1.0f);
				alignas(16) Size3 voxelGroupCount = Size3(0u);
				alignas(16) Size3 voxelGroupSize = Size3(16u);
				alignas(4) uint32_t globalLightCount = 0u;
			};
			GridSettings m_gridSettings;
			Size3 m_maxVoxelGroups = Size3(64);
			Vector3 m_targetVoxelCountPerLight = Vector3(2u);

			inline void OnFlushed() {
				m_canExecute = 1u;
			}

			inline void UpdateLightBoundaries() {
				m_localLightBoundaries.clear();
				m_localLigtIds.clear();
				m_globalLightIds.clear();
				const ViewportLightSet::Reader lights(m_lightSet);
				for (size_t i = 0u; i < lights.LightCount(); i++) {
					const LightDescriptor::ViewportData* const lightData = lights.LightData(i);
					if (lightData == nullptr) continue;
					AABB bounds = lightData->GetLightBounds();
					if (std::isfinite(bounds.start.x) && std::isfinite(bounds.start.y) && std::isfinite(bounds.start.z) &&
						std::isfinite(bounds.end.x) && std::isfinite(bounds.end.y) && std::isfinite(bounds.end.z)) {
						if (bounds.start.x > bounds.end.x) 
							std::swap(bounds.start.x, bounds.end.x);
						if (bounds.start.y > bounds.end.y) 
							std::swap(bounds.start.y, bounds.end.y);
						if (bounds.start.z > bounds.end.z) 
							std::swap(bounds.start.z, bounds.end.z);
						m_localLigtIds.push_back(static_cast<uint32_t>(i));
						m_localLightBoundaries.push_back(bounds);
					}
					else m_globalLightIds.push_back(static_cast<uint32_t>(i));
				}
				m_gridSettings.globalLightCount = static_cast<uint32_t>(m_globalLightIds.size());
			}

			inline void UpdateGridSettings() {
				if (m_localLightBoundaries.size() <= 0u) {
					m_gridSettings.gridOrigin = Vector3(0.0f);
					m_gridSettings.voxelGroupCount = Size3(0u);
					return;
				}

				const AABB* ptr = m_localLightBoundaries.data();
				const AABB* end = ptr + m_localLightBoundaries.size();

				AABB combinedBounds = *ptr;
				Vector3 avergeSize = ptr->end - ptr->start;
				size_t count = 1u;
				ptr++;

				while (ptr < end) {
					const AABB bounds = *ptr;
					ptr++;

					if (combinedBounds.start.x > bounds.start.x)
						combinedBounds.start.x = bounds.start.x;
					if (combinedBounds.start.y > bounds.start.y)
						combinedBounds.start.y = bounds.start.y;
					if (combinedBounds.start.z > bounds.start.z)
						combinedBounds.start.z = bounds.start.z;

					if (combinedBounds.end.x < bounds.end.x)
						combinedBounds.end.x = bounds.end.x;
					if (combinedBounds.end.y < bounds.end.y)
						combinedBounds.end.y = bounds.end.y;
					if (combinedBounds.end.z < bounds.end.z)
						combinedBounds.end.z = bounds.end.z;

					count++;
					avergeSize += (bounds.end - bounds.start - avergeSize) / static_cast<float>(count);
				}

				combinedBounds.start -= 0.1f * avergeSize;
				combinedBounds.end += 0.1f * avergeSize;
				m_gridSettings.gridOrigin = combinedBounds.start;
				const Vector3 totalSize = combinedBounds.end - combinedBounds.start;

				const Size3 maxVoxelCount = m_maxVoxelGroups * m_gridSettings.voxelGroupSize;
				const Vector3 minVoxelSize = totalSize / Vector3(maxVoxelCount);
				m_gridSettings.voxelSize = Vector3(
					Math::Max(minVoxelSize.x, avergeSize.x / m_targetVoxelCountPerLight.x),
					Math::Max(minVoxelSize.y, avergeSize.y / m_targetVoxelCountPerLight.y),
					Math::Max(minVoxelSize.z, avergeSize.z / m_targetVoxelCountPerLight.z));

				const Vector3 voxelGroupSize = m_gridSettings.voxelSize * Vector3(m_gridSettings.voxelGroupSize);
				m_gridSettings.voxelGroupCount = Size3(
					static_cast<uint32_t>(std::ceilf(totalSize.x / voxelGroupSize.x)),
					static_cast<uint32_t>(std::ceilf(totalSize.y / voxelGroupSize.y)),
					static_cast<uint32_t>(std::ceilf(totalSize.z / voxelGroupSize.z)));
			}

			inline bool CalculateGridGroupRanges(const Graphics::InFlightBufferInfo& buffer) {
				// __TODO__: Implement this crap!
				m_context->Log()->Error("SceneLightGrid::Helpers::SharedInstance::CalculateGridGroupRanges - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline bool ComputePerVoxelLightCounts(const Graphics::InFlightBufferInfo& buffer) {
				// __TODO__: Implement this crap!
				m_context->Log()->Error("SceneLightGrid::Helpers::SharedInstance::ComputePerVoxelLightCounts - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline bool ComputePerVoxelLightSegmentTree(const Graphics::InFlightBufferInfo& buffer) {
				// __TODO__: Implement this crap!
				m_context->Log()->Error("SceneLightGrid::Helpers::SharedInstance::ComputePerVoxelLightSegmentTree - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline bool ComputePerVoxelIndexRanges(const Graphics::InFlightBufferInfo& buffer) {
				// __TODO__: Implement this crap!
				m_context->Log()->Error("SceneLightGrid::Helpers::SharedInstance::ComputePerVoxelIndexRanges - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline bool ComputePerVoxelLightIndices(const Graphics::InFlightBufferInfo& buffer) {
				// __TODO__: Implement this crap!
				m_context->Log()->Error("SceneLightGrid::Helpers::SharedInstance::ComputePerVoxelLightIndices - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

		public:
			inline SharedInstance(SceneContext* context, const ViewportLightSet* lightSet)
				: m_context(context)
				, m_lightSet(lightSet) {
				assert(m_context != nullptr);
				assert(m_lightSet != nullptr);
				m_context->Graphics()->OnGraphicsSynch() += Callback(&SharedInstance::OnFlushed, this);
			}

			inline virtual ~SharedInstance() {
				m_context->Graphics()->OnGraphicsSynch() -= Callback(&SharedInstance::OnFlushed, this);
			}

		protected:
			inline virtual void Execute()override {
				std::unique_lock<std::mutex> lock(m_updateLock);
				if (m_canExecute.load() <= 0u) return;
				m_canExecute = 0u;
				
				UpdateLightBoundaries();
				UpdateGridSettings();

				auto cleanState = [&]() {
					m_gridSettings.voxelGroupCount = Size3(0u);
					m_gridSettings.globalLightCount = 0u;
				};

				const Graphics::InFlightBufferInfo buffer = m_context->Graphics()->GetWorkerThreadCommandBuffer();
				if (!CalculateGridGroupRanges(buffer)) return cleanState();
				if (!ComputePerVoxelLightCounts(buffer)) return cleanState();
				if (!ComputePerVoxelLightSegmentTree(buffer)) return cleanState();
				if (!ComputePerVoxelIndexRanges(buffer)) return cleanState();
				if (!ComputePerVoxelLightIndices(buffer)) return cleanState();
			}

			inline virtual void CollectDependencies(Callback<JobSystem::Job*>)override {}
		};
#pragma warning(default: 4250)

		class InstanceCache : public virtual ObjectCache<Reference<const Object>> {
		public:
			static Reference<SceneLightGrid> GetFor(const ViewportLightSet* lightSet, SceneContext* context) {
				static InstanceCache cache;
				return cache.GetCachedOrCreate(lightSet, false, [&]() -> Reference<SharedInstance> {
					Reference<Data> data = Object::Instantiate<Data>(lightSet);
					return Object::Instantiate<SharedInstance>(context, lightSet);
					});
			}
		};
	};



	SceneLightGrid::SceneLightGrid() {}

	SceneLightGrid::~SceneLightGrid() {}

	Reference<SceneLightGrid> SceneLightGrid::GetFor(SceneContext* context) {
		if (context == nullptr) return nullptr;
		const Reference<const ViewportLightSet> lightSet = ViewportLightSet::For(context);
		if (lightSet == nullptr) {
			context->Log()->Error("SceneLightGrid::GetFor - Failed to get ViewportLightSet for given context! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		return Helpers::InstanceCache::GetFor(lightSet, context);
	}

	Reference<SceneLightGrid> SceneLightGrid::GetFor(const ViewportDescriptor* viewport) {
		if (viewport == nullptr) return nullptr;
		const Reference<const ViewportLightSet> lightSet = ViewportLightSet::For(viewport);
		if (lightSet == nullptr) {
			viewport->Context()->Log()->Error("SceneLightGrid::GetFor - Failed to get ViewportLightSet for given viewport! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		return Helpers::InstanceCache::GetFor(lightSet, viewport->Context());
	}

	Graphics::BindingSet::BindingSearchFunctions SceneLightGrid::BindingDescriptor()const {
		Graphics::BindingSet::BindingSearchFunctions search = {};
		// __TODO__: Implement this crap!

		return search;
	}
}
