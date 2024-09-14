#include "LightDataBuffer.h"


namespace Jimara {
	LightDataBuffer::LightDataBuffer(SceneContext* context, const ViewportDescriptor* viewport) 
		: m_info(viewport == nullptr ? SceneLightInfo::Instance(context) : SceneLightInfo::Instance(viewport))
#ifdef SceneLightInfo_USE_BLOCK
		, m_threadCount(std::thread::hardware_concurrency())
#endif
		, m_dataBackBufferId(0) {
		m_info->OnUpdateLightInfo() += Callback<const LightDescriptor::LightInfo*, size_t>(&LightDataBuffer::OnUpdateLights, this);
		Execute();
	}

	LightDataBuffer::LightDataBuffer(SceneContext* context) : LightDataBuffer(context, nullptr) {}

	LightDataBuffer::LightDataBuffer(const ViewportDescriptor* viewport) : LightDataBuffer(viewport->Context(), viewport) {}

	LightDataBuffer::~LightDataBuffer() {
		m_info->OnUpdateLightInfo() -= Callback<const LightDescriptor::LightInfo*, size_t>(&LightDataBuffer::OnUpdateLights, this);
	}

	struct LightDataBuffer::Helpers {
		class Cache : public virtual ObjectCache<Reference<const Object>> {
		public:
			inline static Reference<LightDataBuffer> Instance(const Object* key, SceneContext* context, const ViewportDescriptor* viewport) {
				if (context == nullptr) return nullptr;
				static Cache cache;
				return cache.GetCachedOrCreate(key,
					[&]() ->Reference<LightDataBuffer> {
						const Reference<LightDataBuffer> instance = new LightDataBuffer(context, viewport);
						instance->ReleaseRef();
						return instance;
					});
			}
		};
	};

	Reference<LightDataBuffer> LightDataBuffer::Instance(SceneContext* context) { 
		return Helpers::Cache::Instance(context, context, nullptr); 
	}

	Reference<LightDataBuffer> LightDataBuffer::Instance(const ViewportDescriptor* viewport) { 
		return Helpers::Cache::Instance(viewport, viewport->Context(), viewport); 
	}

	Reference<Graphics::ArrayBuffer> LightDataBuffer::Buffer()const { return m_buffer; }

	void LightDataBuffer::Execute() {
		m_info->ProcessLightInfo(Callback<const LightDescriptor::LightInfo*, size_t>(&LightDataBuffer::UpdateLights, this));
	}

	void LightDataBuffer::CollectDependencies(Callback<Job*> addDependency) {
		addDependency(m_info);
	}

	void LightDataBuffer::UpdateLights(const LightDescriptor::LightInfo* info, size_t count) {
		std::unique_lock<std::mutex> lock(m_lock);
		if (!m_dirty.load()) return;

		std::vector<uint8_t>& dataBackBuffer = m_data[m_dataBackBufferId];
		m_dataBackBufferId = ((m_dataBackBufferId + 1) & 1);
		std::vector<uint8_t>& dataFrontBuffer = m_data[m_dataBackBufferId];

		const size_t elemSize = Math::Max(m_info->Context()->Configuration().ShaderLibrary()->PerLightDataSize(), size_t(1u));
		bool bufferDirty = false;
		
		size_t bytesNeeded = (elemSize * count);
		if (dataBackBuffer.size() < bytesNeeded) {
			dataBackBuffer.resize(bytesNeeded);
			memset(dataBackBuffer.data(), 0, dataBackBuffer.size());
			dataFrontBuffer.resize(bytesNeeded);
			memset(dataFrontBuffer.data(), 0, dataFrontBuffer.size());
			bufferDirty = true;
		}
		else bufferDirty = ((m_buffer == nullptr) || (m_buffer->ObjectSize() != elemSize) || (m_buffer->ObjectCount() != count));
		uint8_t* const cpuBackBuffer = dataBackBuffer.data();
		const uint8_t* const cpuFrontBuffer = dataFrontBuffer.data();

		uint8_t* cpuData = cpuBackBuffer;
		const LightDescriptor::LightInfo* infoPtr = info;
		const LightDescriptor::LightInfo* const end = infoPtr + count;
		while (infoPtr < end) {
			const LightDescriptor::LightInfo& light = (*infoPtr);
			memcpy(cpuData, light.data, Math::Min(light.dataSize, elemSize));
			infoPtr++;
			cpuData += elemSize;
		}
		if (!bufferDirty)
			bufferDirty = (memcmp(cpuBackBuffer, cpuFrontBuffer, bytesNeeded) != 0);
			
		if (bufferDirty) {
			Reference<Graphics::ArrayBuffer> buffer = m_info->Context()->Device()->CreateArrayBuffer(elemSize, count);
			if (count > 0) {
				memcpy(buffer->Map(), dataBackBuffer.data(), bytesNeeded);
				buffer->Unmap(true);
			}
			m_buffer = buffer;
		}
		m_dirty = true;
	}
}
