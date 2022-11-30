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
		m_indirectionBufferId = GetBuffer(IndirectionBufferId());
		if (m_indirectionBufferId == nullptr)
			m_context->Log()->Fatal("ParticleBuffers::ParticleBuffers - Indirection buffer could not be initialized! [File: ", __FILE__, "; Line: ", __LINE__, "]");
	}

	ParticleBuffers::~ParticleBuffers() { }

	ParticleBuffers::BufferInfo ParticleBuffers::GetBufferInfo(const BufferId* bufferId) {
		if (this == nullptr || m_context == nullptr || bufferId == nullptr) return BufferInfo{};
		else if (bufferId == LiveParticleCountBufferId()) 
			return BufferInfo{ LiveParticleCountBuffer(), nullptr };
		else if (bufferId == IndirectionBufferId() && m_indirectionBufferId != nullptr) 
			return BufferInfo{ m_indirectionBufferId.operator->(), nullptr };
		
		std::unique_lock<std::mutex> lock(m_bufferLock);

		{
			decltype(m_buffers)::const_iterator it = m_buffers.find(bufferId);
			if (it != m_buffers.end())
				return BufferInfo{ it->second.bindlessBinding.operator->(), it->second.allocationTask.operator->() };
		}

		const Reference<Graphics::ArrayBuffer> buffer = m_context->Graphics()->Device()->CreateArrayBuffer(bufferId->ElemSize(), m_elemCount, bufferId->CPUAccess());
		if (buffer == nullptr) {
			m_context->Graphics()->Device()->Log()->Error(
				"ParticleBuffers::GetBuffer - Failed to create buffer for Id '", 
				bufferId->Name(), "' at ", ((void*)bufferId), "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return BufferInfo{};
		}

		const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> binding = m_context->Graphics()->Bindless().Buffers()->GetBinding(buffer);
		if (binding == nullptr) {
			m_context->Graphics()->Device()->Log()->Error(
				"ParticleBuffers::GetBuffer - Failed to create bindless buffer Id '",
				bufferId->Name(), "' at ", ((void*)bufferId), "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return BufferInfo{};
		}

		Reference<AllocationTask> allocationTask;
		if (bufferId->BufferAllocationKernel() != nullptr) {
			allocationTask = bufferId->BufferAllocationKernel()->CreateTask(
				m_context, static_cast<uint32_t>(m_elemCount),
				binding, m_indirectionBufferId, m_liveParticleCountBuffer);
			if (allocationTask != nullptr) {
				allocationTask->m_numSpawned = &m_spawnedParticleCount;
				m_allocationTasks.push_back(allocationTask);
			}
		}

		m_buffers[bufferId] = BufferData{ binding, allocationTask };
		return BufferInfo{ binding.operator->(), allocationTask.operator->() };
	}


	const ParticleBuffers::BufferId* ParticleBuffers::LiveParticleCountBufferId() {
		static const Reference<const BufferId> liveParticleCountBufferId = BufferId::Create<uint32_t>("Live Particle Count");
		return liveParticleCountBufferId;
	}

	const ParticleBuffers::BufferId* ParticleBuffers::IndirectionBufferId() {
		static const Reference<const BufferId> indirectionBufferId = BufferId::Create<uint32_t>("Indirection Buffer");
		return indirectionBufferId;
	}
}
