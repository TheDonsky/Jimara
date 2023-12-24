#include "SceneLightInfo.h"


namespace Jimara {
	SceneLightInfo::SceneLightInfo(SceneContext* context, const ViewportDescriptor* viewport)
		: m_context(context)
		, m_lights(LightDescriptor::Set::GetInstance(context))
		, m_viewLights(viewport == nullptr ? ViewportLightSet::For(context) : ViewportLightSet::For(viewport)) {
		assert(m_context != nullptr);
		OnGraphicsSynched();
		m_lights->OnFlushed() += Callback<>(&SceneLightInfo::OnGraphicsSynched, this);
	}

	SceneLightInfo::SceneLightInfo(SceneContext* context)
		: SceneLightInfo(context, nullptr) {}

	SceneLightInfo::SceneLightInfo(const ViewportDescriptor* viewport)
		: SceneLightInfo(viewport->Context(), viewport) {}

	SceneLightInfo::~SceneLightInfo() {
		m_lights->OnFlushed() -= Callback<>(&SceneLightInfo::OnGraphicsSynched, this);
	}

	struct SceneLightInfo::Helpers {
		class Cache : public virtual ObjectCache<Reference<const Object>> {
		public:
			inline static Reference<SceneLightInfo> Instance(const Object* key, SceneContext* context, const ViewportDescriptor* viewport) {
				if (context == nullptr)
					return nullptr;
				static Cache cache;
				return cache.GetCachedOrCreate(key,
					[&]() -> Reference<SceneLightInfo> {
						const Reference<SceneLightInfo> instance = new SceneLightInfo(context, viewport);
						instance->ReleaseRef();
						return instance;
					});
			}
		};
	};

	Reference<SceneLightInfo> SceneLightInfo::Instance(SceneContext* context) { 
		return Helpers::Cache::Instance(context, context, nullptr);
	}

	Reference<SceneLightInfo> SceneLightInfo::Instance(const ViewportDescriptor* viewport) {
		return Helpers::Cache::Instance(viewport, viewport->Context(), viewport);
	}

	Scene::GraphicsContext* SceneLightInfo::Context()const { return m_context->Graphics(); }

	Event<const LightDescriptor::LightInfo*, size_t>& SceneLightInfo::OnUpdateLightInfo() { return m_onUpdateLightInfo; }

	void SceneLightInfo::ProcessLightInfo(const Callback<const LightDescriptor::LightInfo*, size_t>& processCallback) {
		std::shared_lock<std::shared_mutex> lock(m_lock);
		processCallback(m_info.data(), m_info.size());
	}

	void SceneLightInfo::Execute() {
		std::unique_lock<std::shared_mutex> lock(m_lock);
		if (!m_dirty.load()) return;

		m_info.clear();
		{
			ViewportLightSet::Reader reader(m_viewLights);
			size_t lightCount = reader.LightCount();
			for (size_t i = 0; i < lightCount; i++) {
				const LightDescriptor::ViewportData* lightData = reader.LightData(i);
				if (lightData != nullptr)
					m_info.push_back(lightData->GetLightInfo());
			}
		}

		m_dirty = false;
		m_onUpdateLightInfo(m_info.data(), m_info.size());
	}

	void SceneLightInfo::OnGraphicsSynched() {
		m_dirty = true;
	}
}
