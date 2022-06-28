#pragma once
#include "../../../Core/Object.h"


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Simple attribute, telling the editor to hide the field
		/// </summary>
		class JIMARA_API HideInEditorAttribute final : public virtual Object {
		public:
			/// <summary> Singleton instance (not necessary to use this, but it's kinda useful) </summary>
			inline static Reference<const HideInEditorAttribute> Instance() {
				static const HideInEditorAttribute instance;
				return &instance;
			}
		};
	}
}
