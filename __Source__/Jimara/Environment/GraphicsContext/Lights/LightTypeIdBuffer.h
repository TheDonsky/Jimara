#pragma once
#include "SceneLightInfo.h"


namespace Jimara {
	class LightTypeIdBuffer : public virtual ObjectCache<GraphicsContext*>::StoredObject {
	public:
		LightTypeIdBuffer(GraphicsContext* context);

		virtual ~LightTypeIdBuffer();

		static Reference<LightTypeIdBuffer> Instance(GraphicsContext* context);

		Graphics::ArrayBufferReference<uint32_t> Buffer()const;


	private:
		const Reference<SceneLightInfo> m_info;

		std::mutex m_lock;
		std::vector<uint32_t> m_data[2];
		size_t m_dataBackBufferId;
		Graphics::ArrayBufferReference<uint32_t> m_buffer;

		void OnUpdateLights(const LightDescriptor::LightInfo* info, size_t count);
	};
}
