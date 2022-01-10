#pragma once
#include "../../Environment/Scene.h"

namespace Jimara {
#ifdef USE_REFACTORED_SCENE
	typedef Scene::LogicContext::UpdatingComponent Updatable;
#else
	/// <summary> 
	/// If a component needs to do work on per update basis, 
	/// it can simply implement this interface and the Update callback will be automatically invoked each time
	/// </summary>
	class Updatable : public virtual Object {
	public:
		/// <summary> Invoked each time the logical scene is updated </summary>
		virtual void Update() = 0;
	};
#endif
}
