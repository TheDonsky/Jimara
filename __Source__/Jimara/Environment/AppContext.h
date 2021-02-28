#pragma once
#include "../Graphics/GraphicsDevice.h"
#include "../Data/Caches/GraphicsMeshCache.h"


namespace Jimara {
	class AppContext : public virtual Object {
	public:
		AppContext(Graphics::GraphicsDevice* device);

		OS::Logger* Log()const;

		Graphics::GraphicsDevice* GraphicsDevice()const;

		Cache::GraphicsMeshCache* GraphicsMeshCache()const;

	private:
		const Reference<Graphics::GraphicsDevice> m_device;
		const Reference<Cache::GraphicsMeshCache> m_graphicsMeshCache;
	};
}
