#pragma once
#include "SceneContext.h"
#include "../Components/Component.h"
#include <unordered_set>

#include "SceneConfig.h"

namespace Jimara {
#ifdef USE_REFACTORED_SCENE
namespace LegacyScene {
#endif
	class Scene_t : public virtual Object {
	public:
		Scene_t(AppContext* context, Graphics::ShaderLoader* shaderLoader, const OS::Input* input,
			const std::unordered_map<std::string, uint32_t>& lightTypeIds, size_t perLightDataSize,
			LightingModel* defaultLightingModel = nullptr);

		virtual ~Scene_t();

		SceneContext* Context()const;

		Component* RootObject()const;

		void SynchGraphics();

		void Update();

		std::recursive_mutex& UpdateLock();

	private:
		const Reference<SceneContext> m_context;
		Reference<Object> m_sceneData;
		Reference<Object> m_sceneGraphicsData;
		Reference<Component> m_rootObject;
	};

	typedef Scene_t Scene;
#ifdef USE_REFACTORED_SCENE
}
#endif
}
