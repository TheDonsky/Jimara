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
		virtual ValueType Value(ParameterTypes... params)const = 0;
	};


	/// <summary>
	/// Vertex of a cubic bezier curve
	/// </summary>
	/// <typeparam name="ValueType"> Vertex value type (normally, a vector/scalar of some kind, but anything will work, as long as 0-initialisation and arithmetic apply) </typeparam>
	template<typename ValueType>
	class BezierNode {
	public:
		/// <summary>
		/// Constant interpolation mode settings
		/// Note: Mostly useful for time-dependent curves
		/// </summary>
		struct ConstantInterpolation {
			/// <summary> If true, Interpolate() will return either start or end point, regardless of the phase </summary>
			bool active;
			
			/// <summary> If true, Interpolate() will return either end point, as long as active is also true </summary>
			bool next;

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="on"> 'active' field value </param>
			/// <param name="useNext"> 'next' field value </param>
			inline ConstantInterpolation(bool on = false, bool useNext = false) : active(on), next(useNext) {}
		};


		/// <summary>
		/// Constructor
		/// Note: Assumes no independent handles and auto-deduces prevHandle
		/// </summary>
		/// <param name="value"> Node location </param>
		/// <param name="nextHandle"> 'Next'/'Right' curve handle (used when this Node is start point) </param>
		inline BezierNode(const ValueType& value = ValueType(0.0f), const ValueType& nextHandle = ValueType(0.0f))
			: m_value(value) {
			SetNextHandle(nextHandle);
		}

		/// <summary>
		/// Constructor
		/// Note: Assumes independent handles
		/// </summary>
		/// <param name="value"> Node location </param>
		/// <param name="prevHandle"> 'Previous'/'Left' curve handle (used when this Node is end point) </param>
		/// <param name="nextHandle"> 'Next'/'Right' curve handle (used when this Node is start point) </param>
		inline BezierNode(const ValueType& value, const ValueType& prevHandle, const ValueType& nextHandle)
			: m_value(value) {
			IndependentHandles() = true;
			SetPrevHandle(prevHandle);
			SetNextHandle(nextHandle);
		}

		/// <summary>
		/// Constructor
		/// Note: Assumes zero handles if applicable
		/// </summary>
		/// <param name="value"> Node location </param>
		/// <param name="interpolateConstant"> Constant interpolation settings </param>
		inline BezierNode(const ValueType& value, const ConstantInterpolation& interpolateConstant) 
			: m_value(value) {
			InterpolateConstant() = interpolateConstant;
		}


		/// <summary> Node location </summary>
		inline ValueType& Value() { return m_value; }

		/// <summary> Node location </summary>
		inline const ValueType& Value()const { return m_value; }


		/// <summary>
		/// Sets 'Previous'/'Left' handle
		/// Note: For non-independent handles, updates 'Next' handle automatically
		/// </summary>
		/// <param name="handle"> New value </param>
		inline void SetPrevHandle(const ValueType& handle) {
			m_prevHandle = handle;
			if (!((const BezierNode*)this)->IndependentHandles())
				m_nextHandle = -handle;
		}

		/// <summary> Gives access to 'Previous'/'Left' handle </summary>
		inline Property<ValueType> PrevHandle() {
			ValueType(*get)(BezierNode*) = [](BezierNode* self) -> ValueType { return self->m_prevHandle; };
			void(*set)(BezierNode*, const ValueType&) = [](BezierNode* self, const ValueType& value) { self->SetPrevHandle(value); };
			return Property<ValueType>(get, set, this);
		}

		/// <summary> 'Previous'/'Left' handle </summary>
		inline const ValueType& PrevHandle()const { return m_prevHandle; }

		/// <summary>
		/// Sets 'Next'/'Right' handle
		/// Note: For non-independent handles, updates 'Previous' handle automatically
		/// </summary>
		/// <param name="handle"> New value </param>
		inline void SetNextHandle(const ValueType& handle) {
			m_nextHandle = handle;
			if (!((const BezierNode*)this)->IndependentHandles())
				m_prevHandle = -handle;
		}

		/// <summary> Gives access to 'Next'/'Right' handle </summary>
		inline Property<ValueType> NextHandle() {
			ValueType(*get)(BezierNode*) = [](BezierNode* self) -> ValueType { return self->m_nextHandle; };
			void(*set)(BezierNode*, const ValueType&) = [](BezierNode* self, const ValueType& value) { self->SetNextHandle(value); };
			return Property<ValueType>(get, set, this);
		}

		/// <summary> 'Next'/'Right' handle </summary>
		inline const ValueType& NextHandle()const { return m_nextHandle; }


		/// <summary> Gives access to 'Previous'/'Left' tangent (you can think of this as a timeline slope direction at the end point) </summary>
		inline Property<ValueType> PrevTangent() {
			ValueType(*get)(BezierNode*) = [](BezierNode* self) -> ValueType { return ((const BezierNode*)self)->PrevTangent(); };
			void(*set)(BezierNode*, const ValueType&) = [](BezierNode* self, const ValueType& value) { self->SetPrevHandle(value * (1.0f / 3.0f)); };
			return Property<ValueType>(get, set, this);
		}

		/// <summary> 'Previous'/'Left' tangent (you can think of this as a timeline slope direction at the end point) </summary>
		inline ValueType PrevTangent()const { return m_nextHandle * 3.0f; }

		/// <summary> Gives access to 'Next'/'Right' tangent (you can think of this as a timeline slope direction at the start point) </summary>
		inline Property<ValueType> NextTangent() {
			ValueType(*get)(BezierNode*) = [](BezierNode* self) -> ValueType { return ((const BezierNode*)self)->NextTangent(); };
			void(*set)(BezierNode*, const ValueType&) = [](BezierNode* self, const ValueType& value) { self->SetNextHandle(value * (1.0f / 3.0f)); };
			return Property<ValueType>(get, set, this);
		}

		/// <summary> 'Next'/'Right' tangent (you can think of this as a timeline slope direction at the start point) </summary>
		inline ValueType NextTangent()const { return m_nextHandle * 3.0f; }


		/// <summary> Gives access to 'Previous'/'Left' control point </summary>
		inline Property<ValueType> PrevControlPoint() {
			ValueType(*get)(BezierNode*) = [](BezierNode* self) -> ValueType { return ((const BezierNode*)self)->PrevControlPoint(); };
			void(*set)(BezierNode*, const ValueType&) = [](BezierNode* self, const ValueType& value) { self->SetPrevHandle(value - self->m_value); };
			return Property<ValueType>(get, set, this);
		}

		/// <summary> 'Previous'/'Left' control point </summary>
		inline ValueType PrevControlPoint()const { return m_value + m_prevHandle; }

		/// <summary> Gives access to 'Next'/'Right' control point </summary>
		inline Property<ValueType> NextControlPoint() {
			ValueType(*get)(BezierNode*) = [](BezierNode* self) -> ValueType { return ((const BezierNode*)self)->NextControlPoint(); };
			void(*set)(BezierNode*, const ValueType&) = [](BezierNode* self, const ValueType& value) { self->SetNextHandle(value - self->m_value); };
			return Property<ValueType>(get, set, this);
		}

		/// <summary> 'Next'/'Right' control point </summary>
		inline ValueType NextControlPoint()const { return m_value + m_nextHandle; }


		/// <summary> Gives access to handle independence flag </summary>
		inline Property<bool> IndependentHandles() {
			bool(*get)(BezierNode*) = [](BezierNode* self) -> bool { return ((const BezierNode*)self)->IndependentHandles(); };
			void(*set)(BezierNode*, const bool&) = [](BezierNode* self, const bool& independent) { 
				self->SetFlag(Flags::INDEPENDENT_HANDLES, independent);
				if (!independent) self->m_prevHandle = self->m_nextHandle;
			};
			return Property<bool>(get, set, this);
		}

		/// <summary> True, if the left/right handles are independent </summary>
		inline bool IndependentHandles()const {
			return (m_flags & static_cast<uint8_t>(Flags::INDEPENDENT_HANDLES)) != 0;
		}


		/// <summary> Gives access to constant interpolation settings </summary>
		inline Property<ConstantInterpolation> InterpolateConstant() {
			ConstantInterpolation(*get)(BezierNode*) = [](BezierNode* self) -> ConstantInterpolation { return ((const BezierNode*)self)->InterpolateConstant(); };
			void(*set)(BezierNode*, const ConstantInterpolation&) = [](BezierNode* self, const ConstantInterpolation& constant) { 
				self->SetFlag(Flags::INTERPOLATE_CONSTANT, constant.active);
				self->SetFlag(Flags::INTERPOLATE_CONSTANT_NEXT, constant.next);
			};
			return Property<ConstantInterpolation>(get, set, this);
		}

		/// <summary> Constant interpolation settings </summary>
		inline ConstantInterpolation InterpolateConstant()const {
			return ConstantInterpolation(
				(m_flags & static_cast<uint8_t>(Flags::INTERPOLATE_CONSTANT)) != 0,
				(m_flags & static_cast<uint8_t>(Flags::INTERPOLATE_CONSTANT_NEXT)) != 0);
		}


		/// <summary>
		/// Evaluates bezier curve value at some phase
		/// </summary>
		/// <param name="start"> 'Start'/'Left' node </param>
		/// <param name="end"> 'End'/'Right' node </param>
		/// <param name="phase"> Interpolation phase (valid range is [0.0f - 1.0f]) </param>
		/// <returns> Bezier curve value for given phase </returns>
		inline static ValueType Interpolate(const BezierNode& start, const BezierNode& end, float phase) {
			ConstantInterpolation constant = start.InterpolateConstant();
			if (constant.active)
				return constant.next ? end.m_value : start.m_value;
			else {
				float invPhase = (1.0f - phase);
				return
					(start.m_value * (invPhase * invPhase * invPhase)) +
					(start.NextControlPoint() * (3.0f * invPhase * invPhase * phase)) +
					(end.PrevControlPoint() * (3.0f * invPhase * phase * phase)) +
					(end.m_value * (phase * phase * phase));
			}
		}


	private:
		// Node location
		ValueType m_value = ValueType(0.0f);

		// 'Previous'/'Left' handle
		ValueType m_prevHandle = ValueType(0.0f);

		// 'Next'/'Right' handle
		ValueType m_nextHandle = ValueType(0.0f);

		// Node settings
		uint8_t m_flags = 0;

		// Node settings
		enum class Flags : uint8_t {
			NONE = 0,
			INDEPENDENT_HANDLES = 1,
			INTERPOLATE_CONSTANT = 1 << 1,
			INTERPOLATE_CONSTANT_NEXT = 1 << 2
		};

		// Sets a singular flag value
		void SetFlag(Flags flag, bool value) {
			uint8_t mask = static_cast<uint8_t>(flag);
			if (value) m_flags |= mask;
			else m_flags &= ~mask;
		}
	};


	/// <summary>
	/// Curve, describing a value evolving over time using keyframes
	/// </summary>
	/// <typeparam name="ValueType"> Curve value type </typeparam>
	/// <typeparam name="KeyFrameType"> Keyframe type on the timeline </typeparam>
	template<typename ValueType, typename KeyFrameType = BezierNode<ValueType>>
	class TimelineCurve: public virtual ParametricCurve<ValueType, float>, public virtual std::map<float, KeyFrameType> {
	public:
		/// <summary>
		/// Evaluates BezierCurve at the given time point
		/// </summary>
		/// <param name="curve"> Any sorted Keyframe to time mapping </param>
		/// <param name="time"> Time point to evaluate the curve at </param>
		/// <returns> Interpolated value </returns>
		inline static ValueType Value(const std::map<float, KeyFrameType>& curve, float time) {
			// If timeline is empty, we return zero by default:
			if (curve.size() <= 0) return ValueType(0.0f);

			// Let's get the lower bound (later interpreted as the bezier segment end):
			typename std::map<float, KeyFrameType>::const_iterator low = curve.lower_bound(time);

			// If the lower bound happens to go beyond timeline or be less than the minimal time value, we just return the boundary point:
			if (low == curve.end()) return KeyFrameType::Interpolate(curve.rbegin()->second, curve.rbegin()->second, 0.0f);
			else if (low == curve.begin() || time == low->first) return KeyFrameType::Interpolate(low->second, low->second, 0.0f);

			// If we have a valid range, we establish low to be start of the bezier segment and high to be the end:
			const typename std::map<float, KeyFrameType>::const_iterator high = low;
			low--;

			// For normal bezier segments we calculate the phase:
			const float segmentLength = (high->first - low->first);
			const float segmentTime = (time - low->first);
			const float phase = (segmentLength > std::numeric_limits<float>::epsilon() ? (segmentTime / segmentLength) : 0.0f);

			// Finally, we evaluate and return the bezier point:
			return KeyFrameType::Interpolate(low->second, high->second, phase);
		}

		/// <summary>
		/// Evaluates BezierCurve at the given time point
		/// </summary>
		/// <param name="time"> Time point to evaluate the curve at </param>
		/// <returns> Interpolated value </returns>
		inline virtual ValueType Value(float time)const override {
			return Value(*this, time);
		}
	};
}
