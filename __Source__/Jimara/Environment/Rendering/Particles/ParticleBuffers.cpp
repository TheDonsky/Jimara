#include "ParticleBuffers.h"


namespace Jimara {
	ParticleBuffers::ParticleBuffers(SceneContext* context, size_t elemCount)
		: m_context(context), m_elemCount(elemCount) {
		assert(m_context != nullptr);
	}

	ParticleBuffers::~ParticleBuffers() { }

	SceneContext* ParticleBuffers::Context()const { return m_context; }

	Graphics::ArrayBuffer* ParticleBuffers::GetBuffer(const BufferId* bufferId) {
		if (this == nullptr || m_context == nullptr || bufferId == nullptr) return nullptr;
		
		std::unique_lock<std::mutex> lock(m_bufferLock);

		{
			decltype(m_buffers)::const_iterator it = m_buffers.find(bufferId);
			if (it != m_buffers.end())
				return it->second;
		}

		Reference<Graphics::ArrayBuffer> buffer = m_context->Graphics()->Device()->CreateArrayBuffer(bufferId->ElemSize(), m_elemCount, bufferId->CPUAccess());
		if (buffer == nullptr) {
			m_context->Graphics()->Device()->Log()->Error(
				"ParticleBuffers::GetBuffer - Failed to create buffer for Id '", 
				bufferId->Name(), "' at ", ((void*)bufferId), "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		m_buffers[bufferId] = buffer;
		return buffer;
	}
}
