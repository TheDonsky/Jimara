#pragma once
#include "../GraphicsContext.h"
#include "../../../Core/ObjectCache.h"
#include "../../../Core/Collections/ThreadBlock.h"
#include <vector>
#include <mutex>


namespace Jimara {
	/// <summary>
	/// Fetches scene graphics information on each update cycle
	/// </summary>
	class SceneLightInfo : public virtual ObjectCache<GraphicsContext*>::StoredObject {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="context"> "Owner" graphics context </param>
		SceneLightInfo(GraphicsContext* context);

		/// <summary> Virtual destructor </summary>
		virtual ~SceneLightInfo();

		/// <summary>
		/// Singleton instance per graphics context
		/// </summary>
		/// <param name="context"> "Owner" graphics context </param>
		/// <returns> Instance, tied to the context </returns>
		static Reference<SceneLightInfo> Instance(GraphicsContext* context);

		/// <summary> "Owner" graphics contex </summary>
		GraphicsContext* Context()const;

		/// <summary> Invoked each time the data is refreshed </summary>
		Event<const LightDescriptor::LightInfo*, size_t>& OnUpdateLightInfo();

		/// <summary>
		/// Safetly invokes given callback with current lighting information
		/// </summary>
		/// <param name="processCallback"> Callback to invoke </param>
		void ProcessLightInfo(const Callback<const LightDescriptor::LightInfo*, size_t>& processCallback);


	private:
		// "Owner" graphics contex
		const Reference<GraphicsContext> m_context;

		// Number of update threads
		const size_t m_threadCount;

		// Update lock
		std::mutex m_lock;

		// Worker thread block for updates
		ThreadBlock m_block;

		// Current light info
		std::vector<LightDescriptor::LightInfo> m_info;

		// Invoked each time the data is refreshed
		EventInstance<const LightDescriptor::LightInfo*, size_t> m_onUpdateLightInfo;

		// Update function
		void OnGraphicsSynched();
	};
}
