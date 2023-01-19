#pragma once
#include "../../../Core/Object.h"


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Simple attribute, telling the editor not to create a dropdown for a serializer list
		/// </summary>
		class JIMARA_API InlineSerializerListAttribute final : public virtual Object{
		public:
			/// <summary> Singleton instance (not necessary to use this, but it's kinda useful) </summary>
			inline static Reference<const InlineSerializerListAttribute> Instance() {
				static const InlineSerializerListAttribute instance;
				return &instance;
			}
		};
	}
}
