#pragma once
#include "SceneLightInfo.h"


namespace Jimara {
	/// <summary>
	/// Wrapper around a buffer that is updated with current light type identifiers each update cycle
	/// </summary>
	class LightTypeIdBuffer : public virtual ObjectCache<Reference<Object>>::StoredObject {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="context"> "Owner" context </param>
		LightTypeIdBuffer(SceneContext* context);

		/// <summary> Virtual destructor </summary>
		virtual ~LightTypeIdBuffer();

		/// <summary>
		/// Singleton instance per graphics context
		/// </summary>
		/// <param name="context"> "Owner" context </param>
		/// <returns> Instance, tied to the context </returns>
		static Reference<LightTypeIdBuffer> Instance(SceneContext* context);

		/// <summary> Buffer, containing light type identifiers </summary>
		Graphics::ArrayBufferReference<uint32_t> Buffer()const;


	private:
		// Scene light info
		const Reference<SceneLightInfo> m_info;

		// Update lock
		std::mutex m_lock;

		// Data back and front buffers to keep track of the changes
		std::vector<uint32_t> m_data[2];

		// Current back buffer index (flipped on each update)
		size_t m_dataBackBufferId;

		// Underlying buffer
		Graphics::ArrayBufferReference<uint32_t> m_buffer;

		// Update function
		void OnUpdateLights(const LightDescriptor::LightInfo* info, size_t count);
	};
}
