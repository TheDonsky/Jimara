#pragma once
#include "../../GraphicsSimulation/GraphicsSimulation.h"


namespace Jimara {
	/// <summary>
	/// Collection of compute buffers for simulating a single particle system
	/// </summary>
	class JIMARA_API ParticleBuffers : public virtual Object {
	public:
		/// <summary>
		/// Graphics simulation task responsible for initializing individual particle buffers for newly spawned particles.
		/// <para/> Note: AllocationTasks are automatically created using AllocationKernel::CreateTask method 
		/// which receives particle budget, the buffer that needs to be initialized, indirection buffer and live particle count buffer.
		/// Kernel implementation should fill in the newly spawned region of the buffer with default values.
		/// <para/>
		/// <para/> For example, let's say we have some floating point buffer that has to be filled with zeroes for new particles.
		/// CombinedGraphicsSimulationKernel for the implementation would look something like this:
		/// <para/>
		/// <para/> void ExecuteSimulationTask(in SimulationTaskSettings settings, uint taskThreadId) {
		/// <para/>		// SimulationTaskSettings.liveParticleCountBufferId is liveParticleCount->Index() from AllocationKernel::CreateTask;
		/// <para/>		// liveCountBuffers is just an alias for bindless buffers;
		/// <para/>		// here we read the value:
		/// <para/>		const uint liveParticleCount = liveCountBuffers[settings.liveParticleCountBufferId].count[0];
		/// <para/>
		/// <para/>		// SimulationTaskSettings.taskThreadCount should be set to AllocationTask::SpawnedParticleCount();
		/// <para/>		// SimulationTaskSettings.particleBudget should be set to the particleBudget from AllocationKernel::CreateTask;
		/// <para/>		// Sum with liveParticleCount and taskThreadId is the 'real index' of the particle; 
		/// <para/>		// however, we need to make sure not to go beyond the actual particle count:
		/// <para/>		const uint particleIndex = liveParticleCount + taskThreadId;
		/// <para/>		if (particleIndex >= settings.particleBudget) return;
		/// <para/>
		/// <para/>		// SimulationTaskSettings.particleIndirectionBufferId should be set to indirectionBuffer->Index() from AllocationKernel::CreateTask;
		/// <para/>		// SimulationTaskSettings.bufferId should be set to buffer->Index() from AllocationKernel::CreateTask;
		/// <para/>		// indirectionBuffers and bindlessData are just an aliase for bindless buffers;
		/// <para/>		// Particle order is shaffled using the indirection buffer; we need to 'translate' the index using it:
		/// <para/>		const uint indirectParticleId = indirectionBuffers[settings.particleIndirectionBufferId].indices[particleIndex];
		/// <para/>		bindlessData[settings.bufferId].values[indirectParticleId] = 0.0;
		/// <para/> }
		/// <para/>
		/// </summary>
		class JIMARA_API AllocationTask : public virtual GraphicsSimulation::Task {
		public:
			/// <summary> Virtual destructor </summary>
			virtual ~AllocationTask() = 0;

			/// <summary> 
			/// Number of particles that need to be initialized 
			/// (kernel shader has to take full responsiobility for making sure particle index does not go beyond particleBudget) 
			/// </summary>
			inline uint32_t SpawnedParticleCount()const { return m_numSpawned->load(); }

			/// <summary>
			/// Reports Wrangle Step of the indirection buffer as dependency;
			/// <para/> If overriden, it's essential to invoke parent class function as well for the indirection buffer to be ready for use!
			/// </summary>
			/// <param name="recordDependency"> Each task this one depends on should be reported through this callback </param>
			inline virtual void GetDependencies(const Callback<Task*>& recordDependency)const { recordDependency(m_wrangleStep); }

		private:
			static const std::shared_ptr<std::atomic<uint32_t>>& PreInitializedSpawnedCount() {
				static const std::shared_ptr<std::atomic<uint32_t>> count = std::make_shared<std::atomic<uint32_t>>(0u);
				return count;
			}
			// Number of particles that need to be initialized
			std::shared_ptr<const std::atomic<uint32_t>> m_numSpawned = PreInitializedSpawnedCount();

			// Wrangle step (dependency)
			Reference<GraphicsSimulation::Task> m_wrangleStep;

			// m_numSpawned and m_wrangleStep can only be altered by ParticleBuffers
			friend class ParticleBuffers;
		};

		/// <summary>
		/// Kernel for initializing default values of the buffer for newly spawned particles
		/// <para/> Note: Read through the documentation of AllocationTask for further details.
		/// </summary>
		class JIMARA_API AllocationKernel : public virtual GraphicsSimulation::Kernel {
		public:
			/// <summary>
			/// Should create an instance of AllocationTask for this kernel
			/// </summary>
			/// <param name="context"> Scene context </param>
			/// <param name="particleBudget"> ParticleBuffers::ParticleBudget(); Same as the number of elements within the buffer </param>
			/// <param name="buffer"> Buffer, the AllocationTask should take responsibilty of initializing </param>
			/// <param name="indirectionBuffer"> Indirection buffer for 'index wrangling' </param>
			/// <param name="liveParticleCount"> Single element buffer holding count of 'alive' particles at the end of the last frame </param>
			/// <returns> A new instance of AllocationTask for given buffer </returns>
			virtual Reference<AllocationTask> CreateTask(
				SceneContext* context, 
				uint32_t particleBudget,
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* buffer,
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* indirectionBuffer,
				Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* liveParticleCount)const = 0;
		};

		/// <summary>
		/// Unique identifier of an individual compute buffer for particle simulation (can be used as a key and you will normally have a bounch of singletons)
		/// </summary>
		class JIMARA_API BufferId : public virtual Object {
		public:
			/// <summary>
			/// Creates a BufferId for a buffer of given type
			/// </summary>
			/// <typeparam name="BufferType"> Type of the buffer elements </typeparam>
			/// <param name="name"> Buffer name (does not have to be unique) </param>
			/// <param name="allocationKernel"> Kernel for initializing default values of the buffer for newly spawned particles (optional) </param>
			/// <param name="cpuAccess"> CPU-access flags </param>
			/// <returns> Buffer identifier </returns>
			template<typename BufferType>
			inline static Reference<const BufferId> Create(
				const std::string_view& name = "", 
				const AllocationKernel* allocationKernel = nullptr,
				Graphics::Buffer::CPUAccess cpuAccess = Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY) {
				Reference<BufferId> instance = new BufferId(TypeId::Of<BufferType>(), sizeof(BufferType), cpuAccess, allocationKernel, name);
				instance->ReleaseRef();
				return instance;
			}

			/// <summary> Virtual destructor </summary>
			inline virtual ~BufferId() {}

			/// <summary> Buffer element type </summary>
			inline TypeId ElemType()const { return m_elemType; }

			/// <summary> Size of the buffer elements </summary>
			inline size_t ElemSize()const { return m_elemSize; }

			/// <summary> CPU-access flags </summary>
			inline Graphics::Buffer::CPUAccess CPUAccess()const { return m_cpuAccess; }

			/// <summary> Default value Allocation kernel for newly spawned particles (can be null) </summary>
			const AllocationKernel* BufferAllocationKernel()const { return m_allocationKernel; }

			/// <summary> Name of the buffer (technically, does not have to be unique) </summary>
			inline const std::string& Name()const { return m_name; }

		private:
			// Buffer element type
			const TypeId m_elemType;
			
			// Size of the buffer elements
			const size_t m_elemSize;

			// CPU-access flags
			const Graphics::Buffer::CPUAccess m_cpuAccess;

			// Default value Allocation kernel for newly spawned particles
			const Reference<const AllocationKernel> m_allocationKernel;

			// Name of the buffer
			const std::string m_name;

			// Constructor needs to be private
			inline BufferId(const TypeId& type, size_t size, Graphics::Buffer::CPUAccess cpuAccess, 
				const AllocationKernel* allocationKernel, const std::string_view& name)
				: m_elemType(type), m_elemSize(size), m_cpuAccess(cpuAccess)
				, m_allocationKernel(allocationKernel), m_name(name) {}
		};

		/// <summary> Type definition for a generic buffer search function </summary>
		typedef Function<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding*, const ParticleBuffers::BufferId*> BufferSearchFn;



		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <param name="particleBudget"> Number of elements per each buffer (more or less the same as particle count limit) </param>
		ParticleBuffers(SceneContext* context, size_t particleBudget);

		/// <summary> Virtual destructor </summary>
		virtual ~ParticleBuffers();

		/// <summary> Scene context </summary>
		inline SceneContext* Context()const { return m_context; }

		/// <summary> Number of elements per each buffer (more or less the same as particle count limit) </summary>
		inline size_t ParticleBudget()const { return m_elemCount; }

		/// <summary> A single element buffer, holding the number of currently 'alive' particles </summary>
		inline Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* LiveParticleCountBuffer()const { return m_liveParticleCountBuffer; }

		/// <summary> Information about a bound buffer inside the ParticleBuffers </summary>
		struct BufferInfo {
			/// <summary> Buffer binding </summary>
			Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* buffer = nullptr;

			/// <summary> Buffer allocation task (can and will be null if the buffer does not require any spawn-time allocation) </summary>
			AllocationTask* allocationTask = nullptr;
		};

		/// <summary>
		/// Gets buffer binding and allocation task by identifier
		/// <para/> Note: BufferId address is used as the unique identifier
		/// </summary>
		/// <param name="bufferId"> Unique identifier of the buffer within the given buffer set </param>
		/// <returns> Buffer info if successful, {nullptr, nullptr} if bufferId is null or creation fails for some reason </returns>
		BufferInfo GetBufferInfo(const BufferId* bufferId);

		/// <summary>
		/// Gets ArrayBuffer by identifier
		/// <para/> Note: BufferId address is used as the unique identifier
		/// </summary>
		/// <param name="bufferId"> Unique identifier of the buffer within the given buffer set </param>
		/// <returns> ArrayBuffer id, corresponding to given BufferId (nullptr if bufferId is null) </returns>
		inline Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding* GetBuffer(const BufferId* bufferId) { return GetBufferInfo(bufferId).buffer; }

		/// <summary>
		/// Sets spawned particle count for all allocation tasks
		/// </summary>
		/// <param name="numSpawned"> Number of particles that should be spawned at the start of the frame </param>
		inline void SetSpawnedParticleCount(uint32_t numSpawned) { m_spawnedParticleCount->store(Math::Min(numSpawned, static_cast<uint32_t>(m_elemCount))); }

		/// <summary>
		/// Iterates over all allocation tasks for buffers and reports them
		/// <para/> Note: While GetAllocationTasks() is running, calling GetBufferInfo() and GetBuffer() from the same thread will cause a crash, 
		/// while waiting for some other threads making those queries will result in a deadlock. So, USE WITH CAUTION!
		/// </summary>
		/// <typeparam name="ReportFn"> Anything that can be called with AllocationTask* as an argument </typeparam>
		/// <param name="reportTask"> Report callback </param>
		template<typename ReportFn>
		inline void GetAllocationTasks(const ReportFn& reportTask)const {
			std::unique_lock<std::mutex> lock(m_bufferLock);
			const Reference<AllocationTask>* ptr = m_allocationTasks.data();
			const Reference<AllocationTask>* end = ptr + m_allocationTasks.size();
			while (ptr < end) {
				reportTask(ptr->operator->());
				ptr++;
			}
		}

		/// <summary>
		/// A particularily special BufferId, that does not map to any dynamically generated buffer and instead tells 
		/// GetBuffer() to return LiveParticleCountBuffer() instead.
		/// </summary>
		static const BufferId* LiveParticleCountBufferId();

		/// <summary>
		/// During spawning and instance buffer generation we use indirection buffer for index-wrangling.
		/// <para/> Indirection buffer is a buffer of uints that corresponds to the 'canonical' order of live particles;
		/// All particles that are still 'alive' will have lower indirection indices than the dead/unused ones 
		/// and the content is always updated at the begining of the particle simulation.
		/// </summary>
		/// <returns> Id of the indirection buffer </returns>
		static const BufferId* IndirectionBufferId();

	private:
		// SceneContext
		const Reference<SceneContext> m_context;
		
		// Number of elements per each buffer
		const size_t m_elemCount;

		// A single element buffer, holding the number of currently 'alive' particles
		const Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_liveParticleCountBuffer;

		// Indirection buffer id
		Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> m_indirectionBufferId;

		// Wrangle step of the indirection buffer
		Reference<GraphicsSimulation::Task> m_wrangleStep;

		// Spawned particle count
		const std::shared_ptr<std::atomic<uint32_t>> m_spawnedParticleCount = std::make_shared<std::atomic<uint32_t>>(0u);

		// Lock for the internal buffer collection
		mutable std::mutex m_bufferLock;

		// BufferId to Buffer mappings
		struct BufferData {
			Reference<Graphics::BindlessSet<Graphics::ArrayBuffer>::Binding> bindlessBinding;
			Reference<AllocationTask> allocationTask;
		};
		std::unordered_map<Reference<const BufferId>, BufferData> m_buffers;

		// List of all AllocationTask-s
		std::vector<Reference<AllocationTask>> m_allocationTasks;
	};
}
