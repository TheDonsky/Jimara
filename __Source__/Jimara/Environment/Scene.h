#pragma once
#include "SceneContext.h"
#include "../Components/Component.h"
#include "../__Generated__/JIMARA_BUILT_IN_LIGHT_IDENTIFIERS.h"
#include "GraphicsContext/LightingModels/ForwardRendering/ForwardLightingModel.h"
#include <unordered_set>

namespace Jimara {
	class Scene : public virtual Object {
	public:
		Scene(AppContext* context, ShaderLoader* shaderLoader,
			const std::unordered_map<std::string, uint32_t>& lightTypeIds = LightRegistry::JIMARA_BUILT_IN_LIGHT_IDENTIFIERS.typeIds,
			size_t perLightDataSize = LightRegistry::JIMARA_BUILT_IN_LIGHT_IDENTIFIERS.perLightDataSize,
			LightingModel* defaultLightingModel = ForwardLightingModel::Instance());

		virtual ~Scene();

		SceneContext* Context()const;

		Component* RootObject()const;

		void SynchGraphics();

		void Update();

	private:
		const Reference<SceneContext> m_context;
		Reference<Object> m_sceneData;
		Reference<Object> m_sceneGraphicsData;
		Reference<Component> m_rootObject;
	};
}
