#pragma once
#include "../../../Core/Object.h"


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Attribute, telling the editor to create dropdown menu folders for stuff like enumeration attributes 
		/// ('sub/folders' divided by '/' symbold)
		/// </summary>
		class JIMARA_API DrawDropdownMenuFoldersAttribute final : public virtual Object {
		public:
			/// <summary> Singleton instance (not necessary to use this, but it's kinda useful) </summary>
			inline static Reference<const DrawDropdownMenuFoldersAttribute> Instance() {
				static const DrawDropdownMenuFoldersAttribute instance;
				return &instance;
			}
		};
	}
}
