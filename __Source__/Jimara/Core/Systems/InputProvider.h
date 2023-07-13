#pragma once
#include "../WeakReference.h"
#include <optional>


namespace Jimara {
	/// <summary>
	/// Interface for a generic "input" source
	/// </summary>
	/// <typeparam name="ValueType"> Provided input/impulse type </typeparam>
	template<typename ValueType>
	class InputProvider : public virtual WeaklyReferenceable {
	public:
		/// <summary> Virtual destructor </summary>
		inline virtual ~InputProvider() {}

		/// <summary>
		/// Provides "input" value;
		/// <para/> Return type is optional and, therefore, the input is allowed to be empty.
		/// </summary>
		virtual std::optional<ValueType> GetInput() = 0;
	};
}
