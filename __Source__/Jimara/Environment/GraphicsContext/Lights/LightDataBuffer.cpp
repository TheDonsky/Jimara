#include "LightDataBuffer.h"


namespace Jimara {
	LightDataBuffer::LightDataBuffer(GraphicsContext* context) 
		: m_info(SceneLightInfo::Instance(context)), m_threadCount(std::thread::hardware_concurrency()), m_dataBackBufferId(0) {
		Callback<const LightDescriptor::LightInfo*, size_t> callback(&LightDataBuffer::OnUpdateLights, this);
		m_info->ProcessLightInfo(callback);
		m_info->OnUpdateLightInfo() += callback;
	}

	LightDataBuffer::~LightDataBuffer() {
		m_info->OnUpdateLightInfo() -= Callback<const LightDescriptor::LightInfo*, size_t>(&LightDataBuffer::OnUpdateLights, this);
	}

	namespace {
		class Cache : public virtual ObjectCache<GraphicsContext*> {
		public:
			inline static Reference<LightDataBuffer> Instance(GraphicsContext* context) {
				static Cache cache;
				return cache.GetCachedOrCreate(context, false,
					[&]() ->Reference<LightDataBuffer> { return Object::Instantiate<LightDataBuffer>(context); });
			}
		};
	}

	Reference<LightDataBuffer> LightDataBuffer::Instance(GraphicsContext* context) { return Cache::Instance(context); }

	Reference<Graphics::ArrayBuffer> LightDataBuffer::Buffer()const { return m_buffer; }

	namespace {
		struct Updater {
			const LightDescriptor::LightInfo* info;
			size_t count;
			size_t elemSize;
			uint8_t* cpuBackBuffer;
			uint8_t* cpuFrontBuffer;
			mutable std::atomic<bool> bufferDirty;
		};

		struct ThreadRange {
			size_t start, end;

			inline ThreadRange(size_t totalCount, size_t threadId, size_t threadCount) {
				size_t workPerThread = (totalCount + threadCount - 1) / threadCount;
				start = (workPerThread * threadId);
				end = start + workPerThread;
				if (end > totalCount) end = totalCount;
			}
		};
	}

	void LightDataBuffer::OnUpdateLights(const LightDescriptor::LightInfo* info, size_t count) {
		std::unique_lock<std::mutex> lock(m_lock);

		std::vector<uint8_t>& dataBackBuffer = m_data[m_dataBackBufferId];
		m_dataBackBufferId = ((m_dataBackBufferId + 1) & 1);
		std::vector<uint8_t>& dataFrontBuffer = m_data[m_dataBackBufferId];

		Updater updater = {};
		updater.info = info;
		updater.count = count;
		updater.elemSize = m_info->Context()->PerLightDataSize();
		if (updater.elemSize < 1) updater.elemSize = 1;
		
		size_t bytesNeeded = (updater.elemSize * updater.count);
		if (dataBackBuffer.size() < bytesNeeded) {
			dataBackBuffer.resize(bytesNeeded);
			memset(dataBackBuffer.data(), 0, dataBackBuffer.size());
			dataFrontBuffer.resize(bytesNeeded);
			memset(dataFrontBuffer.data(), 0, dataFrontBuffer.size());
			updater.bufferDirty = true;
		}
		else updater.bufferDirty = ((m_buffer == nullptr) || (m_buffer->ObjectSize() != updater.elemSize) || (m_buffer->ObjectCount() != updater.count));
		updater.cpuBackBuffer = dataBackBuffer.data();
		updater.cpuFrontBuffer = dataFrontBuffer.data();

		auto updateCpuBuffer = [](ThreadBlock::ThreadInfo threadInfo, void* dataAddr) {
			const Updater& info = *((Updater*)dataAddr);
			ThreadRange range(info.count, threadInfo.threadId, threadInfo.threadCount);
			size_t bufferStart = (range.start * info.elemSize);
			uint8_t* cpuDataStart = info.cpuBackBuffer + bufferStart;
			uint8_t* cpuData = cpuDataStart;
			for (size_t i = range.start; i < range.end; i++) {
				const LightDescriptor::LightInfo& light = info.info[i];
				size_t copySize = light.dataSize;
				if (copySize > info.elemSize) copySize = info.elemSize;
				memcpy(cpuData, light.data, copySize);
				cpuData += info.elemSize;
			}
			if ((!info.bufferDirty) && (cpuData != cpuDataStart))
				if (memcmp(cpuDataStart, info.cpuFrontBuffer + bufferStart, cpuData - cpuDataStart))
					info.bufferDirty = true;
		};

		if (updater.count < 128) {
			ThreadBlock::ThreadInfo info;
			{
				info.threadCount = 1;
				info.threadId = 0;
			}
			updateCpuBuffer(info, &updater);
		}
		else {
			size_t threadCount = (updater.count + 127) / 128;
			if (threadCount > m_threadCount) threadCount = m_threadCount;
			m_block.Execute(threadCount, &updater, Callback<ThreadBlock::ThreadInfo, void*>(updateCpuBuffer));
		}
		if (updater.bufferDirty) {
			Reference<Graphics::ArrayBuffer> buffer = m_info->Context()->Device()->CreateArrayBuffer(updater.elemSize, updater.count);
			if (updater.count > 0) {
				memcpy(buffer->Map(), dataBackBuffer.data(), updater.elemSize * updater.count);
				buffer->Unmap(true);
			}
			m_buffer = buffer;
		}
	}
}
