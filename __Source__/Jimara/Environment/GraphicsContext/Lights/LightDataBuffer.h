#pragma once
#include "SceneLightInfo.h"


namespace Jimara {
	class LightDataBuffer : public virtual ObjectCache<GraphicsContext*>::StoredObject {
	public:
		LightDataBuffer(GraphicsContext* context);

		virtual ~LightDataBuffer();

		static Reference<LightDataBuffer> Instance(GraphicsContext* context);

		Reference<Graphics::ArrayBuffer> Buffer()const;


	private:
		const Reference<SceneLightInfo> m_info;
		const size_t m_threadCount;

		std::mutex m_lock;
		ThreadBlock m_block;
		std::vector<uint8_t> m_data[2];
		size_t m_dataBackBufferId;
		Reference<Graphics::ArrayBuffer> m_buffer;

		void OnUpdateLights(const LightDescriptor::LightInfo* info, size_t count);
	};
}
