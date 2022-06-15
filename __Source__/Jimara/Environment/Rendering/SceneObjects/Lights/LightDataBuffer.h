#pragma once
#include "SceneLightInfo.h"


namespace Jimara {
#pragma warning(disable: 4250)
#pragma warning(disable: 4275)
	/// <summary>
	/// Wrapper around a buffer that is updated with current light data each update cycle
	/// </summary>
	class JIMARA_API LightDataBuffer : public virtual JobSystem::Job, public virtual ObjectCache<Reference<Object>>::StoredObject {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="context"> "Owner" context </param>
		LightDataBuffer(SceneContext* context);

		/// <summary> Virtual destructor </summary>
		virtual ~LightDataBuffer();

		/// <summary>
		/// Singleton instance per graphics context
		/// </summary>
		/// <param name="context"> "Owner" context </param>
		/// <returns> Instance, tied to the context </returns>
		static Reference<LightDataBuffer> Instance(SceneContext* context);

		/// <summary> Buffer, containing light data </summary>
		Reference<Graphics::ArrayBuffer> Buffer()const;

	protected:
		/// <summary> Updates content if dirty </summary>
		virtual void Execute() override;

		/// <summary>
		/// This jobs depends on SceneLightInfo
		/// </summary>
		/// <param name="addDependency"> Callback for reporting dependencies </param>
		virtual void CollectDependencies(Callback<Job*> addDependency) override;

	private:
		// Scene light info
		const Reference<SceneLightInfo> m_info;

		// Number of worker threads for updates
		const size_t m_threadCount;

		// Update lock
		std::mutex m_lock;

		// Thread block for workers
		ThreadBlock m_block;

		// Data back and front buffers to keep track of the changes
		std::vector<uint8_t> m_data[2];

		// Current back buffer index (flipped on each update)
		size_t m_dataBackBufferId;

		// Underlying buffer
		Reference<Graphics::ArrayBuffer> m_buffer;

		// True, if dirty
		std::atomic<bool> m_dirty = true;

		// Update function
		void UpdateLights(const LightDescriptor::LightInfo* info, size_t count);

		// Sets dirty status function
		inline void OnUpdateLights(const LightDescriptor::LightInfo*, size_t) { m_dirty = true; }
	};
#pragma warning(default: 4275)
#pragma warning(default: 4250)
}
