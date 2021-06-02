#pragma once
#include "../../Core/Object.h"

namespace Jimara {
	/// <summary>
	/// If a component needs to do some work right before each physics synch point, this is the interface to implement
	/// </summary>
	class PrePhysicsSynchUpdater : public virtual Object {
	public:
		/// <summary> Invoked by the environment right before each physics synch point </summary>
		virtual void PrePhysicsSynch() = 0;
	};

	/// <summary>
	/// If a component needs to do some work right after each physics synch point, this is the interface to implement
	/// </summary>
	class PostPhysicsSynchUpdater : public virtual Object {
	public:
		/// <summary> Invoked by the environment right after each physics synch point </summary>
		virtual void PostPhysicsSynch() = 0;
	};
}
