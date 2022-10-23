#include "ParticleBuffers.h"


namespace Jimara {
	ParticleBuffers::ParticleBuffers(Graphics::GraphicsDevice* device, size_t elemCount)
		: m_device(device), m_elemCount(elemCount) {
		assert(m_device != nullptr);
	}

	ParticleBuffers::~ParticleBuffers() { }

	Graphics::ArrayBuffer* ParticleBuffers::GetBuffer(const BufferId* bufferId) {
		if (this == nullptr || m_device == nullptr || bufferId == nullptr) return nullptr;
		
		std::unique_lock<std::mutex> lock(m_bufferLock);

		{
			decltype(m_buffers)::const_iterator it = m_buffers.find(bufferId);
			if (it != m_buffers.end())
				return it->second;
		}

		Reference<Graphics::ArrayBuffer> buffer = m_device->CreateArrayBuffer(bufferId->ElemSize(), m_elemCount, bufferId->CPUAccess());
		if (buffer == nullptr) {
			m_device->Log()->Error(
				"ParticleBuffers::GetBuffer - Failed to create buffer for Id '", 
				bufferId->Name(), "' at ", ((void*)bufferId), "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		m_buffers[bufferId] = buffer;
		return buffer;
	}
}
