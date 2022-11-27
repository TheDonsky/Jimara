#include "ParticleBuffers.h"


namespace Jimara {
	ParticleBuffers::ParticleBuffers(SceneContext* context, size_t particleBudget)
		: m_context(context), m_elemCount(particleBudget)
		, m_liveParticleCountBuffer([&]() -> Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> {
		assert(context != nullptr);
		const Graphics::ArrayBufferReference<uint32_t> buffer = context->Graphics()->Device()->CreateArrayBuffer<uint32_t>(1u);
		if (buffer == nullptr) {
			context->Log()->Fatal("ParticleBuffers::ParticleBuffers - Failed to create LiveParticleCountBuffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}
		const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> binding = context->Graphics()->Bindless().Buffers()->GetBinding(buffer);
		if (binding == nullptr)
			context->Log()->Fatal("ParticleBuffers::ParticleBuffers - Failed to create LiveParticleCountBuffer bindless binding! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		return binding;
			}()) {
		assert(m_liveParticleCountBuffer != nullptr);
		reinterpret_cast<uint32_t*>(m_liveParticleCountBuffer->BoundObject()->Map())[0u] = 0u;
		m_liveParticleCountBuffer->BoundObject()->Unmap(true);
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

	const ParticleBuffers::BufferId* ParticleBuffers::IndirectionBufferId() {
		static const Reference<const BufferId> indirectionBufferId = BufferId::Create<uint32_t>("Indirection Buffer");
		return indirectionBufferId;
	}
}
