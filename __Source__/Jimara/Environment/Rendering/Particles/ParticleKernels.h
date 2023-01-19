#pragma once
#include "ParticleTaskSet.h"
#include "../../../Core/TypeRegistration/ObjectFactory.h"


namespace Jimara {
	/// <summary>
	/// A parent class of all graphics simulation tasks that can be executed during new particle initialization.
	/// <para/> Corresponding kernels are responsible for matters like particle initial position, scale or velocity initialization as well as setting any other starting parameters.
	/// </summary>
	class JIMARA_API ParticleInitializationTask 
		: public virtual GraphicsSimulation::Task
		, public virtual Serialization::Serializable
		, public virtual ParticleTaskSet<ParticleInitializationTask>::TaskSetEntry {
	public:
		/// <summary> Type definition for the registered factories of concreate implementations </summary>
		typedef ObjectFactory<ParticleInitializationTask, const ParticleSystemInfo*> Factory;

		/// <summary> Type definition for a function that searches for a bindless buffer index by ParticleBuffers::BufferId* </summary>
		typedef Function<uint32_t, const ParticleBuffers::BufferId*> BufferSearchFn;

		/// <summary>
		/// Sets ParticleBuffers
		/// <para/> Invokes SetBuffers() abstract method and records default-value dependencies
		/// </summary>
		/// <param name="buffers"> New particle buffer collection to use </param>
		void SetBuffers(ParticleBuffers* buffers);

		/// <summary> Makes sure to keep alive particle buffers from the last configuration and invokes UpdateSettings() method </summary>
		virtual void Synchronize()final override;

		/// <summary>
		/// Reports all relevant default value kernels as dependencies.
		/// <para/> Note: If overriden, the parent class method should still be invoked or bad things will happen!
		/// </summary>
		/// <param name="recordDependency"> Dependencies are reported through this callback </param>
		virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const override;

		/// <summary> Number of particles spawned duiring the last update cycle (safe to use inside UpdateSettings()) </summary>
		uint32_t SpawnedParticleCount()const;

	protected:
		/// <summary>
		/// Should update SimulationTaskSettings with new taskThreadCount and bindless buffer identifiers.
		/// </summary>
		/// <param name="particleBudget"> Number of particles within the owner particle system (for combined kernels, SimulationTaskSettings::taskThreadCount should be set to this) </param>
		/// <param name="indirectionBuffer"> Bindless index of the indirection buffer (for combined kernels, SimulationTaskSettings::particleIndirectionBufferId should be set to this) </param>
		/// <param name="liveParticleCountBuffer"> Bindless index of the 'live particle count' buffer (for combined kernels, SimulationTaskSettings::liveParticleCountBufferId should be set to this) </param>
		/// <param name="findBuffer"> If simulation task settings require additional bindless buffers from ParticleBuffers (I know, they always do), this is the callback for querying the bindless indices </param>
		virtual void SetBuffers(uint32_t particleBudget, uint32_t indirectionBuffer, uint32_t liveParticleCountBuffer, const BufferSearchFn& findBuffer) = 0;

		/// <summary> Should synchronize simulation task settings with the scene logic </summary>
		virtual void UpdateSettings() = 0;

	private:
		mutable SpinLock m_dependencyLock;
		Reference<ParticleBuffers> m_buffers;
		Reference<ParticleBuffers> m_lastBuffers;
		Stacktor<Reference<GraphicsSimulation::Task>, 4u> m_dependencies;
	};



	/// <summary>
	/// A parent class of all graphics simulation tasks that effect live particles each frame.
	/// <para/> Corresponding kernels are responsible for matters like applying gravity to particles, changing their color/size/shape over time, causing arbitrary motion and basically whatever the hell they want.
	/// </summary>
	class JIMARA_API ParticleTimestepTask 
		: public virtual GraphicsSimulation::Task
		, public virtual Serialization::Serializable
		, public virtual ParticleTaskSet<ParticleTimestepTask>::TaskSetEntry {
	public:
		/// <summary> 
		/// Type definition for the registered factories of concreate implementations.
		/// <para/> Constructor argument is ParticleSystemInfo. Do whatever you want with it.
		/// </summary>
		typedef ObjectFactory<ParticleTimestepTask, const ParticleSystemInfo*> Factory;

		/// <summary> Type definition for a function that searches for a bindless buffer index by ParticleBuffers::BufferId* </summary>
		typedef Function<uint32_t, const ParticleBuffers::BufferId*> BufferSearchFn;

		/// <summary>
		/// Sets ParticleBuffers
		/// <para/> Invokes SetBuffers() abstract method and saves the reference of the ParticleBuffers.
		/// </summary>
		/// <param name="buffers"> New particle buffer collection to use </param>
		void SetBuffers(ParticleBuffers* buffers);

		/// <summary> Makes sure to keep alive particle buffers from the last configuration and invokes UpdateSettings() method </summary>
		virtual void Synchronize()final override;

		/// <summary>
		/// Reports Spawning Step as dependency
		/// <para/> Note: If overriden, the parent class method should still be invoked or bad things will happen!
		/// </summary>
		/// <param name="recordDependency"> Dependencies are reported through this callback </param>
		virtual void GetDependencies(const Callback<GraphicsSimulation::Task*>& recordDependency)const override;

	protected:
		/// <summary>
		/// Should update SimulationTaskSettings with new taskThreadCount and bindless buffer identifiers.
		/// </summary>
		/// <param name="particleBudget"> Number of particles within the owner particle system (for combined kernels, SimulationTaskSettings::taskThreadCount should be set to this) </param>
		/// <param name="findBuffer"> If simulation task settings require additional bindless buffers from ParticleBuffers (I know, they always do), this is the callback for querying the bindless indices </param>
		virtual void SetBuffers(uint32_t particleBudget, const BufferSearchFn& findBuffer) = 0;

		/// <summary> Should synchronize simulation task settings with the scene logic </summary>
		virtual void UpdateSettings() = 0;

	private:
		Reference<ParticleBuffers> m_buffers;
		Reference<ParticleBuffers> m_lastBuffers;
	};
}
