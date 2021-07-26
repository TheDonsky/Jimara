#pragma once
#include "AppContext.h"
#include "PhysicsContext.h"
#include "GraphicsContext/GraphicsContext.h"
#include "../Data/TypeRegistration/TypeRegistartion.h"
#include "../Audio/AudioInstance.h"

namespace Jimara {
	class Component;

	class SceneContext : public virtual Object {
	public:
		SceneContext(AppContext* Context, GraphicsContext* graphicsContext, PhysicsContext* physicsContext, const OS::Input* input, Audio::AudioScene* audioScene);

		AppContext* Context()const;

		OS::Logger* Log()const;

		GraphicsContext* Graphics()const;

		PhysicsContext* Physics()const;

		const OS::Input* Input()const;

		Audio::AudioScene* AudioScene()const;


	private:
		const Reference<BuiltInTypeRegistrator> m_registrator = BuiltInTypeRegistrator::Instance();
		const Reference<AppContext> m_context;
		const Reference<GraphicsContext> m_graphicsContext;
		const Reference<PhysicsContext> m_physicsContext;
		const Reference<const OS::Input> m_input;
		const Reference<Audio::AudioScene> m_audioScene;

	protected:
		virtual void ComponentInstantiated(Component* component) = 0;

		friend class Component;
	};
}
