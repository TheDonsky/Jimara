#pragma once
#include "AppContext.h"
#include "../Graphics/Rendering/GraphicsPipelineSet.h"

namespace Jimara {
	class Scene : public virtual Object {
	public:
		Scene(AppContext* context);

		AppContext* Context()const;

		OS::Logger* Log()const;

		Graphics::GraphicsDevice* GraphicsDevice()const;

		Graphics::GraphicsObjectSet* GraphicsPipelineSet()const;

	private:
		const Reference<AppContext> m_context;
		const Reference<Graphics::GraphicsObjectSet> m_graphicsPipelineSet;
	};
}
