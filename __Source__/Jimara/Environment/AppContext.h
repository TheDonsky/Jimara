#pragma once
#include "../Graphics/GraphicsDevice.h"


namespace Jimara {
	class AppContext : public virtual Object {
	public:
		AppContext(Graphics::GraphicsDevice* device);

		OS::Logger* Log()const;

		Graphics::GraphicsDevice* GraphicsDevice()const;

	private:
		const Reference<Graphics::GraphicsDevice> m_device;
	};
}
