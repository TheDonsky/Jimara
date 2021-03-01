#pragma once
#include "SceneContext.h"
#include "../Components/Component.h"

namespace Jimara {
	class Scene : public virtual Object {
	public:
		Scene(AppContext* context);

		virtual ~Scene();

		SceneContext* Context()const;

		Component* RootObject()const;

	private:
		const Reference<SceneContext> m_context;
		Reference<Component> m_rootObject;
	};
}
