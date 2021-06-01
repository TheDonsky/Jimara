#pragma once
#include "../../Core/Object.h"

namespace Jimara {
	class PrePhysicsSynchUpdater : public virtual Object {
	public:
		inline virtual void PrePhysicsSynch() = 0;
	};

	class PostPhysicsSynchUpdater : public virtual Object {
	public:
		inline virtual void PostPhysicsSynch() = 0;
	};
}
