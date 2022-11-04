#pragma once
#include "../../Scene/Scene.h"


namespace Jimara {
	/// <summary>
	/// Collection of compute buffers for simulating a single particle system
	/// </summary>
	class JIMARA_API ParticleBuffers : public virtual Object {
	public:
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
			/// <param name="cpuAccess"> CPU-access flags </param>
			/// <returns> Buffer identifier </returns>
			template<typename BufferType>
			inline static Reference<const BufferId> Create(const std::string_view& name = "", Graphics::Buffer::CPUAccess cpuAccess = Graphics::Buffer::CPUAccess::CPU_WRITE_ONLY) {
				Reference<BufferId> instance = new BufferId(TypeId::Of<BufferType>(), sizeof(BufferType), name);
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

			/// <summary> Name of the buffer (technically, does not have to be unique) </summary>
			inline const std::string& Name()const { return m_name; }

		private:
			// Buffer element type
			const TypeId m_elemType;
			
			// Size of the buffer elements
			const size_t m_elemSize;

			// CPU-access flags
			const Graphics::Buffer::CPUAccess m_cpuAccess;

			// Name of the buffer
			const std::string m_name;

			// Constructor needs to be private
			inline BufferId(const TypeId& type, size_t size, Graphics::Buffer::CPUAccess cpuAccess, const std::string_view& name)
				: m_elemType(type), m_elemSize(size), m_cpuAccess(cpuAccess), m_name(name) {}
		};


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

		/// <summary>
		/// Gets ArrayBuffer by identifier
		/// <para/> Note: BufferId address is used as the unique identifier
		/// </summary>
		/// <param name="bufferId"> Unique identifier of the buffer within the given buffer set </param>
		/// <returns> ArrayBuffer, corresponding to given BufferId (nullptr if bufferId is null) </returns>
		Graphics::ArrayBuffer* GetBuffer(const BufferId* bufferId);

	private:
		// SceneContext
		const Reference<SceneContext> m_context;
		
		// Number of elements per each buffer
		const size_t m_elemCount;

		// Lock for the internal buffer collection
		std::mutex m_bufferLock;

		// BufferId to Buffer mappings
		std::unordered_map<Reference<const BufferId>, Reference<Graphics::ArrayBuffer>> m_buffers;
	};
}
