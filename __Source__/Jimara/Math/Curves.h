#pragma once
#include "../Core/Object.h"


namespace Jimara {
	/// <summary>
	/// General definition of an arbitrary parametric curve object
	/// Note: We could use Function<ValueType, ParameterTypes...>, but our general approach is that Functions are just "pointers" and are not supposed to hold any data themselves
	/// </summary>
	/// <typeparam name="ValueType"> Curve value type </typeparam>
	/// <typeparam name="...ParameterTypes"> Curve 'Coordinate' type </typeparam>
	template<typename ValueType, typename... ParameterTypes>
	class ParametricCurve : public virtual Object {
	public:
		/// <summary>
		/// Evaluates the curve based on the parameters/coordinates
		/// </summary>
		/// <param name="...params"> parameters/coordinates </param>
		/// <returns> Curve value at params... </returns>
		virtual ValueType Value(const ParameterTypes&... params) = 0;
	};
}
