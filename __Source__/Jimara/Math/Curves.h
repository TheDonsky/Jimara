#pragma once
#include "../Core/Object.h"
#include "../Core/Property.h"
#include "Math.h"
#include <optional>
#include <map>


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
		virtual ValueType Value(ParameterTypes... params) = 0;
	};


	/// <summary>
	/// Curve, describing some value evolving over time using simple bezier curves
	/// </summary>
	/// <typeparam name="ValueType"> Curve value does not have to be a floaing point, we have generalized it as a generic scalar/vector type </typeparam>
	template<typename ValueType>
	class BezierCurve : public virtual ParametricCurve<ValueType, float> {
	public:
		/// <summary>
		/// Key Frame descriptor
		/// </summary>
		struct KeyFrame {
			/// <summary> Curve value associated with KeyFrame's time point </summary>
			ValueType value = ValueType(0.0f);

			/// <summary> 
			/// Left tangent 
			/// Notes: 
			///		0. Between KeyFrame a and b adds the second control point (b.value + (1.0f/3.0f) * leftTangent); 
			///		1. No value causes entire segment to be equal to a.value;
			///		2. If neither a.rightTangent nor b.leftTangent have values, entire segment to be to b.value;
			/// </summary>
			std::optional<ValueType> leftTangent;

			/// <summary> 
			/// Right tangent
			/// Notes:
			///		0. Between KeyFrame a and b adds the second control point (a.value + (1.0f/3.0f) * rightTangent);
			///		1. No value causes entire segment to be equal to b.value;
			///		2. If neither a.rightTangent nor b.leftTangent have values, entire segment to be to b.value;
			/// </summary>
			std::optional<ValueType> rightTangent;
		};

		/// <summary>
		/// Removes a KeyFrame for time point
		/// </summary>
		/// <param name="time"> Time point to erase </param>
		inline void EraseKeyframe(float time) { m_timeline.erase(time); }

		/// <summary>
		/// KeyFrame for time point
		/// </summary>
		/// <param name="time"> Time point </param>
		/// <returns> KeyFrame associated with the time point </returns>
		inline KeyFrame& operator[](float time) { return m_timeline[time]; }

		/// <summary>
		/// KeyFrame for time point
		/// </summary>
		/// <param name="time"> Time point </param>
		/// <returns> KeyFrame associated with the time point </returns>
		inline const KeyFrame& operator[](float time)const { return m_timeline[time]; }

		/// <summary>
		/// Evaluates BezierCurve at the given time point
		/// </summary>
		/// <param name="time"> Time point to evaluate the curve at </param>
		/// <returns> Interpolated value </returns>
		inline virtual ValueType Value(float time) override {
			// If timeline is empty, we return zero by default:
			if (m_timeline.size() <= 0) return ValueType(0.0f);
			
			// Let's get the lower bound (later interpreted as the bezier segment end):
			typename std::map<float, KeyFrame>::const_iterator low = m_timeline.lower_bound(time);
			
			// If the lower bound happens to go beyond timeline or be less than the minimal time value, we just return the boundary point:
			if (low == m_timeline.end()) return m_timeline.rbegin()->second.value;
			else if (low == m_timeline.begin() || time == low->first) return low->second.value;
			
			// If we have a valid range, we establish low to be start of the bezier segment and high to be the end:
			const typename std::map<float, KeyFrame>::const_iterator high = low;
			low--;

			// If we do not have tangents, we can apply a "special" case and interpret as one of two possible constant interpolation modes:
			const ValueType lowValue = low->second.value;
			const ValueType highValue = high->second.value;
			if (!low->second.rightTangent.has_value()) return highValue;
			else if (!high->second.leftTangent.has_value()) return lowValue;

			// For normal bezier segments we calculate the phase:
			const float segmentLength = (high->first - low->first);
			const float segmentTime = (time - low->first);
			const float phase = (segmentLength > std::numeric_limits<float>::epsilon() ? (segmentTime / segmentLength) : 0.0f);
			const float invPhase = (1.0f - phase);
			const float tangentScale = 1.0f / 3.0f;

			// Finally, we evaluate and return the bezier point:
			// Note: general assumption is that t-coordinates of the tangents are (1.0 / 3.0) and (2.0 / 3.0) since for this combination phase remains linear
			return
				(lowValue * (invPhase * invPhase * invPhase)) +
				((lowValue + (tangentScale * low->second.rightTangent.value())) * (3.0f * invPhase * invPhase * phase)) +
				((highValue + (tangentScale * high->second.leftTangent.value())) * (3.0f * invPhase * phase * phase)) +
				(highValue * (phase * phase * phase));
		}

	private:
		// Timeline keyframes per time point
		std::map<float, KeyFrame> m_timeline;
	};
}
