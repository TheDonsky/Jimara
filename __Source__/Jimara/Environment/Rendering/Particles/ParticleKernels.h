#pragma once
#include "ParticleBuffers.h"
#include "../../../Core/TypeRegistration/ObjectFactory.h"


namespace Jimara {
	class JIMARA_API ParticleInitializationTask : public virtual GraphicsSimulation::Task, public virtual Serialization::Serializable {
	public:
		typedef ObjectFactory<ParticleInitializationTask, SceneContext*> Factory;

		void SetBuffers(ParticleBuffers* buffers);

		virtual void Synchronize()final override;

		virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const override;

		uint32_t SpawnedParticleCount()const;

		uint32_t ParticleBudget()const;

	protected:
		virtual void SetBuffers(
			Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* indirectionBuffer,
			Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* liveParticleCountBuffer,
			const ParticleBuffers::BufferSearchFn& findBuffer) = 0;

		virtual void UpdateSettings() = 0;

	private:
		mutable SpinLock m_dependencyLock;
		Reference<ParticleBuffers> m_buffers;
		Reference<ParticleBuffers> m_lastBuffers;
		Stacktor<Reference<GraphicsSimulation::Task>, 4u> m_dependencies;
	};



	class JIMARA_API ParticleTimestepTask : public virtual GraphicsSimulation::Task, public virtual Serialization::Serializable {
	public:
		typedef ObjectFactory<ParticleTimestepTask, GraphicsSimulation::Task*> Factory;

		void SetBuffers(ParticleBuffers* buffers);

		virtual void Synchronize()final override;

		virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const override;

	protected:
		virtual void SetBuffers(const ParticleBuffers::BufferSearchFn& findBuffer) = 0;

		virtual void UpdateSettings() = 0;

	private:
		Reference<ParticleBuffers> m_buffers;
		Reference<ParticleBuffers> m_lastBuffers;
		Reference<GraphicsSimulation::Task> m_spawningStep;
		friend class ParticleTimestepKernel;
	};
}
