#include "LightTypeIdBuffer.h"


namespace Jimara {
	LightTypeIdBuffer::LightTypeIdBuffer(SceneContext* context, const ViewportDescriptor* viewport)
		: m_info(viewport == nullptr ? SceneLightInfo::Instance(context) : SceneLightInfo::Instance(viewport))
		, m_dataBackBufferId(0) {
		m_info->OnUpdateLightInfo() += Callback<const LightDescriptor::LightInfo*, size_t>(&LightTypeIdBuffer::OnUpdateLights, this);
		Execute();
	}

	LightTypeIdBuffer::LightTypeIdBuffer(SceneContext* context) : LightTypeIdBuffer(context, nullptr) {}

	LightTypeIdBuffer::LightTypeIdBuffer(const ViewportDescriptor* viewport) : LightTypeIdBuffer(viewport->Context(), viewport) {}

	LightTypeIdBuffer::~LightTypeIdBuffer() {
		m_info->OnUpdateLightInfo() -= Callback<const LightDescriptor::LightInfo*, size_t>(&LightTypeIdBuffer::OnUpdateLights, this);
	}

	struct LightTypeIdBuffer::Helpers {
		class Cache : public virtual ObjectCache<Reference<const Object>> {
		public:
			inline static Reference<LightTypeIdBuffer> Instance(const Object* key, SceneContext* context, const ViewportDescriptor* viewport) {
				if (context == nullptr) return nullptr;
				static Cache cache;
				return cache.GetCachedOrCreate(key, false,
					[&]() ->Reference<LightTypeIdBuffer> {
						const Reference<LightTypeIdBuffer> instance = new LightTypeIdBuffer(context, viewport);
						instance->ReleaseRef();
						return instance;
					});
			}
		};
	};

	Reference<LightTypeIdBuffer> LightTypeIdBuffer::Instance(SceneContext* context) { 
		return Helpers::Cache::Instance(context, context, nullptr);
	}

	Reference<LightTypeIdBuffer> LightTypeIdBuffer::Instance(const ViewportDescriptor* viewport) {
		return Helpers::Cache::Instance(viewport, viewport->Context(), viewport);
	}

	Graphics::ArrayBufferReference<uint32_t> LightTypeIdBuffer::Buffer()const { return m_buffer; }

	void LightTypeIdBuffer::Execute() {
		m_info->ProcessLightInfo(Callback<const LightDescriptor::LightInfo*, size_t>(&LightTypeIdBuffer::UpdateLights, this));
	}

	void LightTypeIdBuffer::CollectDependencies(Callback<Job*> addDependency) {
		addDependency(m_info);
	}

	void LightTypeIdBuffer::UpdateLights(const LightDescriptor::LightInfo* info, size_t count) {
		std::unique_lock<std::mutex> lock(m_lock);
		if (!m_dirty.load()) return;

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
		m_dirty = false;
	}
}
