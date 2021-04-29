#pragma once
#include "SceneLightInfo.h"


namespace Jimara {
	/// <summary>
	/// Wrapper around a buffer that is updated with current light data each update cycle
	/// </summary>
	class LightDataBuffer : public virtual ObjectCache<Reference<Object>>::StoredObject {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="context"> "Owner" graphics context </param>
		LightDataBuffer(GraphicsContext* context);

		/// <summary> Virtual destructor </summary>
		virtual ~LightDataBuffer();

		/// <summary>
		/// Singleton instance per graphics context
		/// </summary>
		/// <param name="context"> "Owner" graphics context </param>
		/// <returns> Instance, tied to the context </returns>
		static Reference<LightDataBuffer> Instance(GraphicsContext* context);

		/// <summary> Buffer, containing light data </summary>
		Reference<Graphics::ArrayBuffer> Buffer()const;


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

		// Update function
		void OnUpdateLights(const LightDescriptor::LightInfo* info, size_t count);
	};
}
