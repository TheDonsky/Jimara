#pragma once
#include "../../Core/Object.h"

namespace Jimara {
	class PrePhysicsSynchUpdater : public virtual Object {
	public:
		virtual void PrePhysicsSynch() = 0;
	};

	class PostPhysicsSynchUpdater : public virtual Object {
	public:
		virtual void PostPhysicsSynch() = 0;
	};
}
