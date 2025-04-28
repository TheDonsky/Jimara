#pragma once
#include "../../../Core/Object.h"


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Simple attribute, storing a default value, where applicable
		/// </summary>
		/// <typeparam name="Type"> Value type </typeparam>
		template<typename Type>
		class DefaultValueAttribute : public virtual Object {
		public:
			/// <summary> Default value </summary>
			const Type value;

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="defaultValue"> Default value </param>
			inline DefaultValueAttribute(const Type& defaultValue) : value(defaultValue) {}
		};
	}
}
