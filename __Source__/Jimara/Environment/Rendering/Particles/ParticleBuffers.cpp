#include "ParticleBuffers.h"


namespace Jimara {
	ParticleBuffers::ParticleBuffers(SceneContext* context, size_t particleBudget)
		: m_context(context), m_elemCount(particleBudget) {
		assert(m_context != nullptr);
	}

	ParticleBuffers::~ParticleBuffers() { }

	Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* ParticleBuffers::GetBuffer(const BufferId* bufferId) {
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

		Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> binding = m_context->Graphics()->Bindless().Buffers()->GetBinding(buffer);
		if (binding == nullptr) {
			m_context->Graphics()->Device()->Log()->Error(
				"ParticleBuffers::GetBuffer - Failed to create bindless buffer Id '",
				bufferId->Name(), "' at ", ((void*)bufferId), "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		m_buffers[bufferId] = binding;
		return binding;
	}
}
