#include "LogicContext.h"


namespace Jimara {
namespace Refactor_TMP_Namespace {

	void Scene::LogicContext::Update(float deltaTime) {
		Reference<Data> data = m_data;
		if (data == nullptr) return;
		
		{
			// __TODO__: Check which components got added/enabled/disabled, reorganize internal structures and invoke relevant callbacks
		}

		{
			// __TODO__: Update all updating components;
		}

		m_onUpdate();
	}
}
}
