#include "Scene.h"


namespace Jimara {
	namespace {
		class RootComponent : public virtual Component {
		public:
			inline RootComponent(SceneContext* context) : Component(context) {}

			inline virtual void SetParent(Component*) override {
				Context()->Log()->Fatal("Scene Root Object can not have a parent!");
			}
		};
	}

	Scene::Scene(AppContext* context) 
		: m_context(Object::Instantiate<SceneContext>(context)) {
		m_rootObject = Object::Instantiate<RootComponent>(m_context);
	}

	Scene::~Scene() { m_rootObject->Destroy(); }

	SceneContext* Scene::Context()const { return m_context; }

	Component* Scene::RootObject()const { return m_rootObject; }
}
