#include "SceneLightInfo.h"


namespace Jimara {
	SceneLightInfo::SceneLightInfo(GraphicsContext* context) 
		: m_context(context), m_threadCount(std::thread::hardware_concurrency()) {
		{
			GraphicsContext::ReadLock lock(m_context);
			OnGraphicsSynched();
		}
		m_context->OnPostGraphicsSynch() += Callback<>(&SceneLightInfo::OnGraphicsSynched, this);
	}

	SceneLightInfo::~SceneLightInfo() {
		m_context->OnPostGraphicsSynch() -= Callback<>(&SceneLightInfo::OnGraphicsSynched, this);
	}

	namespace {
		class Cache : public virtual ObjectCache<GraphicsContext*> {
		public:
			inline static Reference<SceneLightInfo> Instance(GraphicsContext* context) {
				static Cache cache;
				return cache.GetCachedOrCreate(context, false,
					[&]() ->Reference<SceneLightInfo> { return Object::Instantiate<SceneLightInfo>(context); });
			}
		};
	}

	Reference<SceneLightInfo> SceneLightInfo::Instance(GraphicsContext* context) { return Cache::Instance(context); }

	GraphicsContext* SceneLightInfo::Context()const { return m_context; }

	Event<const LightDescriptor::LightInfo*, size_t>& SceneLightInfo::OnUpdateLightInfo() { return m_onUpdateLightInfo; }

	void SceneLightInfo::ProcessLightInfo(const Callback<const LightDescriptor::LightInfo*, size_t>& processCallback) {
		std::unique_lock<std::mutex> lock(m_lock);
		processCallback(m_info.data(), m_info.size());
	}

	namespace {
		struct Updater {
			std::vector<LightDescriptor::LightInfo>* info;
			const Reference<LightDescriptor>* lights;
			size_t count;
		};
	}

	void SceneLightInfo::OnGraphicsSynched() {
		std::unique_lock<std::mutex> lock(m_lock);
		Updater updater = {};
		updater.info = &m_info;
		m_context->GetSceneLightDescriptors(updater.lights, updater.count);
		if (m_info.size() != updater.count) m_info.resize(updater.count);
		auto job = [](ThreadBlock::ThreadInfo threadInfo, void* dataAddr) {
			Updater info = *((Updater*)dataAddr);
			for (size_t i = threadInfo.threadId; i < info.count; i += threadInfo.threadCount)
				info.info->operator[](i) = info.lights[i]->GetLightInfo();
		};
		if (updater.count < 128) {
			ThreadBlock::ThreadInfo info;
			info.threadCount = 1;
			info.threadId = 0;
			job(info, &updater);
		}
		else {
			size_t threadCount = (updater.count + 127) / 128;
			if (threadCount > m_threadCount) threadCount = m_threadCount;
			m_block.Execute(threadCount, &updater, Callback<ThreadBlock::ThreadInfo, void*>(job));
		}
		m_onUpdateLightInfo(m_info.data(), m_info.size());
	}
}
