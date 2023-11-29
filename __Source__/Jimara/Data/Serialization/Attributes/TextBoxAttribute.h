#pragma once
#include "../../../Core/Object.h"


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Attribute that tells Editor application to show text box instead of a text field for string types
		/// </summary>
		class JIMARA_API TextBoxAttribute final : public virtual Object {
		public:
			/// <summary> Singleton instance (not necessary to use this, but it's kinda useful) </summary>
			inline static Reference<const TextBoxAttribute> Instance() {
				static const TextBoxAttribute instance;
				return &instance;
			}
		};
	}
}
