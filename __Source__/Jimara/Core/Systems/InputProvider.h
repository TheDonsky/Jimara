#pragma once
#include "../WeakReference.h"
#include <optional>


namespace Jimara {
	/// <summary>
	/// Interface for a generic "input" source
	/// </summary>
	/// <typeparam name="ValueType"> Provided input/impulse type </typeparam>
	/// <typeparam name="...Arguments"> Any arguments passed to GetInput() method </typeparam>
	template<typename ValueType, typename... Arguments>
	class InputProvider : public virtual WeaklyReferenceable {
	public:
		/// <summary> Virtual destructor </summary>
		inline virtual ~InputProvider() {}

		/// <summary>
		/// Provides "input" value;
		/// <para/> Return type is optional and, therefore, the input is allowed to be empty.
		/// </summary>
		/// <param name="...args"> Some contextual arguments </param>
		/// <returns> Input value if present </returns>
		virtual std::optional<ValueType> GetInput(Arguments... args) = 0;

		/// <summary>
		/// Safetly provides input with a default value
		/// </summary>
		/// <param name="inputProvider"> Input provider address (if nullptr, default value will be returned automatically) </param>
		/// <param name="...args"> Some contextual arguments for the GetInput method </param>
		/// <param name="defaultValue"> Default value to return when InputProvider is null or does not have a value </param>
		/// <returns> Input value or default value </returns>
		inline static ValueType GetInput(InputProvider* inputProvider, Arguments... args, const ValueType& defaultValue) {
			if (inputProvider == nullptr)
				return defaultValue;
			const std::optional<ValueType> rv = inputProvider->GetInput(args...);
			return rv.has_value() ? rv.value() : defaultValue;
		}

		/// <summary>
		/// Safetly provides input value with an internal null-check
		/// </summary>
		/// <param name="inputProvider"> Input provider address (if nullptr, no value will be returned) </param>
		/// <param name="...args"> Some contextual arguments for the GetInput method </param>
		/// <returns> Input value if inputProvider is valid and it's input is present </returns>
		inline static std::optional<ValueType> GetInput(InputProvider* inputProvider, Arguments... args) {
			if (inputProvider == nullptr)
				return std::optional<ValueType>();
			else return inputProvider->GetInput(args...);
		}
	};
}
