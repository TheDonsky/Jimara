#include "LightTypeIdBuffer.h"


namespace Jimara {
	LightTypeIdBuffer::LightTypeIdBuffer(GraphicsContext* context) 
		: m_info(SceneLightInfo::Instance(context)), m_dataBackBufferId(0) {
		Callback<const LightDescriptor::LightInfo*, size_t> callback(&LightTypeIdBuffer::OnUpdateLights, this);
		m_info->ProcessLightInfo(callback);
		m_info->OnUpdateLightInfo() += callback;

	}

	LightTypeIdBuffer::~LightTypeIdBuffer() {
		m_info->OnUpdateLightInfo() -= Callback<const LightDescriptor::LightInfo*, size_t>(&LightTypeIdBuffer::OnUpdateLights, this);
	}

	namespace {
		class Cache : public virtual ObjectCache<Reference<Object>> {
		public:
			inline static Reference<LightTypeIdBuffer> Instance(GraphicsContext* context) {
				static Cache cache;
				return cache.GetCachedOrCreate(context, false,
					[&]() ->Reference<LightTypeIdBuffer> { return Object::Instantiate<LightTypeIdBuffer>(context); });
			}
		};
	}

	Reference<LightTypeIdBuffer> LightTypeIdBuffer::Instance(GraphicsContext* context) { return Cache::Instance(context); }

	Graphics::ArrayBufferReference<uint32_t> LightTypeIdBuffer::Buffer()const { return m_buffer; }

	void LightTypeIdBuffer::OnUpdateLights(const LightDescriptor::LightInfo* info, size_t count) {
		std::unique_lock<std::mutex> lock(m_lock);

		std::vector<uint32_t>& dataBackBuffer = m_data[m_dataBackBufferId];
		m_dataBackBufferId = ((m_dataBackBufferId + 1) & 1);
		std::vector<uint32_t>& dataFrontBuffer = m_data[m_dataBackBufferId];

		bool bufferDirty = ((m_buffer == nullptr) || (m_buffer->ObjectCount() != count));

		if (dataBackBuffer.size() < count) {
			dataBackBuffer.resize(count);
			dataFrontBuffer.resize(count);
		}

		for (size_t i = 0; i < count; i++)
			dataBackBuffer[i] = info[i].typeId;

		const size_t bufferSize = (sizeof(uint32_t) * count);
		if (!bufferDirty)
			if (memcmp(dataBackBuffer.data(), dataFrontBuffer.data(), bufferSize) != 0)
				bufferDirty = true;

		if (bufferDirty) {
			Graphics::ArrayBufferReference<uint32_t> buffer = m_info->Context()->Device()->CreateArrayBuffer<uint32_t>(count);
			if (count > 0) {
				memcpy(buffer->Map(), dataBackBuffer.data(), bufferSize);
				buffer->Unmap(true);
			}
			m_buffer = buffer;
		}
	}
}
