#include "SceneLightInfo.h"


namespace Jimara {
	SceneLightInfo::SceneLightInfo(SceneContext* context, const ViewportDescriptor* viewport)
		: m_context(context)
		, m_lights(LightDescriptor::Set::GetInstance(context))
		, m_viewLights(viewport == nullptr ? ViewportLightSet::For(context) : ViewportLightSet::For(viewport))
		, m_threadCount(std::thread::hardware_concurrency()) {
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
				return cache.GetCachedOrCreate(key, false,
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
		std::unique_lock<std::mutex> lock(m_lock);
		processCallback(m_info.data(), m_info.size());
	}

	namespace {
		struct Updater {
			std::vector<LightDescriptor::LightInfo>* info;
			const Reference<const LightDescriptor::ViewportData>* lights;
			size_t count;
		};
	}

	void SceneLightInfo::Execute() {
		std::unique_lock<std::mutex> lock(m_lock);
		if (!m_dirty.load()) return;

		Updater updater = {};
		updater.info = &m_info;
		m_descriptors.clear();
		{
			ViewportLightSet::Reader reader(m_viewLights);
			size_t lightCount = reader.LightCount();
			for (size_t i = 0; i < lightCount; i++) {
				const LightDescriptor::ViewportData* lightData = reader.LightData(i);
				if (lightData == nullptr) continue;
				else m_descriptors.push_back(lightData);
			}
		}
		updater.lights = m_descriptors.data();
		updater.count = m_descriptors.size();
		if (m_info.size() != updater.count) m_info.resize(updater.count);
		auto job = [](ThreadBlock::ThreadInfo threadInfo, void* dataAddr) {
			Updater info = *((Updater*)dataAddr);
			for (size_t i = threadInfo.threadId; i < info.count; i += threadInfo.threadCount)
				info.info->operator[](i) = info.lights[i]->GetLightInfo();
		};
		if (updater.count < 128) {
			ThreadBlock::ThreadInfo info = {};
			info.threadCount = 1;
			info.threadId = 0;
			job(info, &updater);
		}
		else {
			size_t threadCount = (updater.count + 127) / 128;
			if (threadCount > m_threadCount) threadCount = m_threadCount;
			m_block.Execute(threadCount, &updater, Callback<ThreadBlock::ThreadInfo, void*>(job));
		}
		m_dirty = false;
		m_onUpdateLightInfo(m_info.data(), m_info.size());
	}

	void SceneLightInfo::OnGraphicsSynched() {
		m_dirty = true;
	}
}
