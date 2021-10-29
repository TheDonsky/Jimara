#pragma once
#include "../../../Core/Object.h"


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Simple attribute, telling that Vector3 ValueSerializer is an euler angle-type input
		/// </summary>
		class EulerAnglesAttribute : public virtual Object {
		public:
		};
	}
}

