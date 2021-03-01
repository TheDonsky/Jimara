#pragma once
#include "AppContext.h"
#include "../Graphics/Rendering/GraphicsPipelineSet.h"

namespace Jimara {
	class SceneContext : public virtual Object {
	public:
		SceneContext(AppContext* Context);

		AppContext* Context()const;

		OS::Logger* Log()const;

		Graphics::GraphicsDevice* GraphicsDevice()const;

		Graphics::GraphicsMeshCache* GraphicsMeshCache()const;

		Graphics::GraphicsObjectSet* GraphicsPipelineSet()const;


	private:
		const Reference<AppContext> m_context;
		const Reference<Graphics::GraphicsObjectSet> m_graphicsPipelineSet;
	};
}
