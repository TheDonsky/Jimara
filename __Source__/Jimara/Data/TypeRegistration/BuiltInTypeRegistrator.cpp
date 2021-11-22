#include "TypeRegistartion.h"
#include "../../__Generated__/JIMARA_BUILT_IN_TYPE_REGISTRATOR.impl.h"

namespace Jimara {
	template<>
	TypeInheritance TypeInheritance::Of<BuiltInTypeRegistrator>() {
		static const TypeId objectType[] = { TypeId::Of<Object>() };
		return TypeInheritance(objectType);
	}
}
