#include "TransientBuffer.h"
#include "../../Math/Helpers.h"


namespace Jimara {
	namespace {
		struct Jimara_TransientBuffer_Key {
			Reference<Graphics::GraphicsDevice> device;
			size_t index = 0u;

			inline bool operator==(const Jimara_TransientBuffer_Key& other)const {
				return device == other.device && index == other.index;
			}
		};
	}
}

namespace std {
	template<>
	struct hash<Jimara::Jimara_TransientBuffer_Key> {
		inline size_t operator()(const Jimara::Jimara_TransientBuffer_Key& desc)const {
			return Jimara::MergeHashes(
				std::hash<Jimara::Reference<Jimara::Graphics::GraphicsDevice>>()(desc.device),
				std::hash<size_t>()(desc.index));
		}
	};
}

namespace Jimara {
	Reference<TransientBuffer> TransientBuffer::Get(Graphics::GraphicsDevice* device, size_t index) {
		if (device == nullptr) 
			return nullptr;

#pragma warning(disable: 4250)
		struct CachedInstance : public virtual TransientBuffer, public virtual ObjectCache<Jimara_TransientBuffer_Key>::StoredObject {
			inline CachedInstance(Graphics::GraphicsDevice* device) : TransientBuffer(device) {}
		};
#pragma warning(default: 4250)

		struct Cache : public virtual ObjectCache<Jimara_TransientBuffer_Key> {
			static Reference<CachedInstance> Get(const Jimara_TransientBuffer_Key& key) {
				static Cache cache;
				return cache.GetCachedOrCreate(key, false, [&]() { return Object::Instantiate<CachedInstance>(key.device); });
			}
		};

		return Cache::Get(Jimara_TransientBuffer_Key{ device, index });
	}

	Reference<Graphics::ArrayBuffer> TransientBuffer::GetBuffer(size_t minSize) {
		Reference<Graphics::ArrayBuffer> buffer;

		// Fetch existing buffer:
		{
			std::unique_lock<SpinLock> lock(m_lock);
			buffer = m_buffer;
			if (buffer != nullptr && buffer->ObjectCount() >= minSize)
				return buffer;
		}

		// Create new buffer:
		{
			size_t elemCount = (buffer == nullptr) ? size_t(1u) : buffer->ObjectCount();
			while (elemCount < minSize)
				elemCount <<= 1u;
			buffer = m_device->CreateArrayBuffer(1u, elemCount, Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY);
			if (buffer == nullptr) {
				m_device->Log()->Error("TransientBuffer::GetBuffer - Failed to allocate new buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}
			else {
				std::unique_lock<SpinLock> lock(m_lock);
				if (m_buffer != nullptr && m_buffer->ObjectCount() >= buffer->ObjectCount())
					buffer = m_buffer;
				else m_buffer = buffer;
			}
		}

		return buffer;
	}
}
