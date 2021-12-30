#pragma once
#include "../../../Core/Object.h"


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Simple attribute, telling the editor to hide the field
		/// </summary>
		class HideInEditorAttribute final : public virtual Object {};
	}
}
