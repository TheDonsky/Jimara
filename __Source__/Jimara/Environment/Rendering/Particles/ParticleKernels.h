#pragma once
#include "ParticleTaskSet.h"
#include "../../../Core/TypeRegistration/ObjectFactory.h"


namespace Jimara {
	/**
	In order to add a custom initialization or timestep kernel to particle simulation, one can implement a new ParticleInitializationTask/ParticleTimestepTask as follows:
	
	(Note that both cases are very similar and we have distinguished the code differences with WE_HAVE_AN_INITIALIZATION_TASK #ifdef blocks)
	_________________________________________________________________
	// Glsl shader definition (OurTask_shader):
	#version 450
	
	// Define the parameters each task gets inside the kernel:
	struct SimulationTaskSettings {
	#if WE_HAVE_AN_INITIALIZATION_TASK
		uint liveParticleCountBufferId;		// Required for ParticleInitializationTask; ParticleTimestepTask does not need it
		uint particleIndirectionBufferId;	// Required for ParticleInitializationTask; ParticleTimestepTask does not need it
		uint particleBudget;				// Required for ParticleInitializationTask; ParticleTimestepTask does not need it
	#endif
		uint taskThreadCount;				// Required both for ParticleInitializationTask and ParticleTimestepTask
		// Fields below are dependent on what the kernel needs to do:
		uint stateBufferId;
		// More fields can go here... 
	};
		
	// CombinedParticleInitializationKernel uses some bindless bindings and one should specify the binding set for those here:
	#define BINDLESS_BUFFER_BINDING_SET 0 // Can be other indices if necessary, something has to be provided though
	
	// Kernels depend on the combined graphics simulatin kernel and it has to have a disignated binding set and binding index:
	#define COMBINED_KERNEL_BINDING_SET 1 // Can be other indices if necessary, something has to be provided though
	#define COMBINED_KERNEL_BINDING 0	// Can be other indices if necessary, something has to be provided though
	
	// Optional binding if you want to access RNG_STATE:
	#define COMBINED_KERNEL_RNG_BINDING 1 // Can be other indices if necessary, something has to be provided though
	
	// Optional binding if you want to access Jimara_DeltaTime and/or Jimara_TotalTime functions:
	#define COMBINED_KERNEL_TIME_BINDING 2 // Can be other indices if necessary, something has to be provided though
	
	// Necessary include for the "magic to happen":
	#if WE_HAVE_AN_INITIALIZATION_TASK
	#include "Jimara/Environment/Rendering/Particles/InitializationTasks/CombinedParticleInitializationKernel.glh"
	#else
	#include "Jimara/Environment/Rendering/Particles/TimestepTasks/CombinedParticleTimestepKernel.glh"
	#endif
	
	// If our function operates on state, one would include access it through this:
	#include "Jimara/Environment/Rendering/Particles/ParticleState.glh"
	 layout(set = BINDLESS_BUFFER_BINDING_SET, binding = 0) buffer StateBuffers {
	 	ParticleState[] state;
	 } stateBuffers[];
	
	void UpdateParticle(in SimulationTaskSettings settings, uint particleIndex) {
		// This method is executed per new particle;
		// Target state would be accessed as: stateBuffers[nonuniformEXT(settings.stateBufferId)].state[particleIndex] and your kernel can do whatever it wants with it.
	}
	
	
	_________________________________________________________________
	// Cpp class definition (can be split into header and cpp; here is merged for convenience):
	namespace OurNamespace {
		// Let the system know about existance of out type:
		JIMARA_REGISTER_TYPE(OurNamespace::OurParticleTask);
		
		// Define task class:
		class OurParticleTask : public virtual 
	#if WE_HAVE_AN_INITIALIZATION_TASK
			ParticleInitializationTask 
	#else
			ParticleTimestepTask
	#endif
		{
		public:
			// Constructor with ParticleSystemInfo as argument:
			OurParticleTask(const ParticleSystemInfo* systemInfo) 
				: GraphicsSimulation::Task(CombinedParticleKernel::GetCached<SimulationTaskSettings>("Project/Path/To/OurTask_shader"), systemInfo->Context()) {}
				
			// Particle Task is a standard serializable object and fields are exposed in standard way...
			virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override {
				// Expose parameters from here...
			}
		
		protected:
			// Invoked whenevr the particle simulation buffers change; one should extract all relevant buffer id-s here:
	#if WE_HAVE_AN_INITIALIZATION_TASK
			virtual void SetBuffers(uint32_t particleBudget, uint32_t indirectionBuffer, uint32_t liveParticleCountBuffer, const BufferSearchFn& findBuffer)override {
				m_simulationSettings.liveParticleCountBufferId = liveParticleCountBuffer;
				m_simulationSettings.particleIndirectionBufferId = indirectionBuffer;
				m_simulationSettings.particleBudget = particleBudget;
				// Fields above are mandatory, you can add any other parameters to your liking...
				m_simulationSettings.stateBufferId = findBuffer(ParticleState::BufferId());
			}
	#else
			void SetBuffers(uint32_t particleBudget, const BufferSearchFn& findBuffer)override {
				m_simulationSettings.taskThreadCount = particleBudget;
				// Field above is mandatory, you can add any other parameters to your liking...
				m_simulationSettings.stateBufferId = findBuffer(ParticleState::BufferId());
			}
	#endif
		
			// Invoked by graphics simulation engine on graphics synch point. Should set task settings so that the kernel instance may access them:
			virtual void UpdateSettings()override {
				m_simulationSettings.taskThreadCount = SpawnedParticleCount();
				// taskThreadCount is mandatory, you can add/change any other parameters to your liking...
				SetSettings(m_simulationSettings);
			}
	
		private:
			// Define the parameters each task gets inside the kernel (Should be binary-equivalent of the structure defined on GPU side):
			struct SimulationTaskSettings {
	#if WE_HAVE_AN_INITIALIZATION_TASK
				alignas(4) uint32_t liveParticleCountBufferId = 0u;
				alignas(4) uint32_t particleIndirectionBufferId = 0u;
				alignas(4) uint32_t particleBudget = 0u;
	#endif
				alignas(4) uint32_t taskThreadCount = 0u;
				// Fields below are dependent on what the kernel needs to do:
				alignas(4) uint32_t stateBufferId = 0u;
				// More fields can go here... 
			} m_simulationSettings;
		};
	}
	 
	namespace Jimara {
		// Report OurParticleTask factory through attributes and make it available to the Editor and save/load functionality:
		template<> void TypeIdDetails::GetTypeAttributesOf<OurNamespace::OurParticleTask>(const Callback<const Object*>& report) {
			static const Reference<const Object> factory = OurNamespace::OurParticleTask::Factory::Create<OurNamespace::OurParticleTask>(
				"OurParticleTask", "OurNamespace/OurParticleTask", "Hint about what our task does");
			report(factory);
		}
	}
	*/

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
