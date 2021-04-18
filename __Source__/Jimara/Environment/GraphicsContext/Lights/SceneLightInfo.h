#pragma once
#include "../GraphicsContext.h"
#include "../../../Core/ObjectCache.h"
#include "../../../Core/Collections/ThreadBlock.h"
#include <vector>
#include <mutex>


namespace Jimara {
	class SceneLightInfo : public virtual ObjectCache<GraphicsContext*>::StoredObject {
	public:
		SceneLightInfo(GraphicsContext* context);

		virtual ~SceneLightInfo();

		static Reference<SceneLightInfo> Instance(GraphicsContext* context);

		GraphicsContext* Context()const;

		Event<const LightDescriptor::LightInfo*, size_t>& OnUpdateLightInfo();

		void ProcessLightInfo(const Callback<const LightDescriptor::LightInfo*, size_t>& processCallback);

	private:
		const Reference<GraphicsContext> m_context;
		const size_t m_threadCount;

		std::mutex m_lock;
		ThreadBlock m_block;
		std::vector<LightDescriptor::LightInfo> m_info;

		EventInstance<const LightDescriptor::LightInfo*, size_t> m_onUpdateLightInfo;

		void OnGraphicsSynched();
	};
}
