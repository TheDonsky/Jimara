#pragma once
#include "Math.h"
#include "../Core/Object.h"
#include "../Core/Property.h"
#include "../Data/Serialization/Serializable.h"
#include "../Data/Serialization/DefaultSerializer.h"
#include "../Data/Serialization/Attributes/EnumAttribute.h"
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
	class JIMARA_API ParametricCurve : public virtual Object {
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
				if (!independent) self->m_prevHandle = -self->m_nextHandle;
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

		/// <summary>
		/// Bezier node serializer
		/// </summary>
		struct Serializer;

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
	/// Bezier node serializer
	/// </summary>
	template<typename ValueType>
	struct BezierNode<ValueType>::Serializer : public virtual Serialization::SerializerList::From<BezierNode<ValueType>> {
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="name"> Name of the ItemSerializer </param>
		/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
		/// <param name="attributes"> Serializer attributes </param>
		inline Serializer(const std::string_view& name = "Bezier Node", const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {})
			: Serialization::ItemSerializer(name, hint, attributes) {}

		/// <summary>
		/// Serializes BezierNode
		/// </summary>
		/// <typeparam name="ValueType"> Bezier value type </typeparam>
		/// <param name="recordElement"> Fields will be reported through this callback </param>
		/// <param name="target"> BezierNode to serialize </param>
		inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, BezierNode<ValueType>* target)const override {
			{
				static const auto serializer = Serialization::DefaultSerializer<ValueType>::Create("Value", "Bezier node value");
				recordElement(serializer->Serialize(target->m_value));
			}
			{
				uint8_t flags = static_cast<uint8_t>(target->m_flags);
				static const auto serializer = Serialization::DefaultSerializer<uint8_t>::Create("Flags", "Bezier Node Flags",
					std::vector<Reference<const Object>> { Object::Instantiate<Serialization::EnumAttribute<uint8_t>>(true,
						"INDEPENDENT_HANDLES", static_cast<uint8_t>(Flags::INDEPENDENT_HANDLES),
						"INTERPOLATE_CONSTANT", static_cast<uint8_t>(Flags::INTERPOLATE_CONSTANT),
						"INTERPOLATE_CONSTANT_NEXT", static_cast<uint8_t>(Flags::INTERPOLATE_CONSTANT) | static_cast<uint8_t>(Flags::INTERPOLATE_CONSTANT_NEXT))
				});
				recordElement(serializer->Serialize(flags));
				target->IndependentHandles() = ((flags & static_cast<uint8_t>(Flags::INDEPENDENT_HANDLES)) != 0);
				target->InterpolateConstant() = ConstantInterpolation(
					(flags & static_cast<uint8_t>(Flags::INTERPOLATE_CONSTANT)) != 0,
					(flags & static_cast<uint8_t>(Flags::INTERPOLATE_CONSTANT_NEXT)) != 0);
			}
			{
				static const auto serializer = Serialization::DefaultSerializer<ValueType>::Create(
					"NextHandle", "Handle, connecting to the next segment of the spline");
				recordElement(serializer->Serialize(target->m_nextHandle));
			}
			if (target->IndependentHandles()) {
				static const auto serializer = Serialization::DefaultSerializer<ValueType>::Create(
					"PrevHandle", "Handle, connecting to the previous segment of the spline");
				recordElement(serializer->Serialize(target->m_prevHandle));
			}
			else target->m_prevHandle = -target->m_nextHandle;
		}
	};


	/// <summary>
	/// Curve, describing a value evolving over time using keyframes
	/// </summary>
	/// <typeparam name="ValueType"> Curve value type </typeparam>
	/// <typeparam name="KeyFrameType"> Keyframe type on the timeline </typeparam>
	template<typename ValueType, typename KeyFrameType = BezierNode<ValueType>>
	class TimelineCurve : public virtual ParametricCurve<ValueType, float>, public virtual std::map<float, KeyFrameType>, public virtual Serialization::Serializable {
	public:
		/// <summary>
		/// Evaluates TimelineCurve at the given time point
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

			// If we have a valid range, we establish low to be start of the bezier segment and hi gh to be the end:
			const typename std::map<float, KeyFrameType>::const_iterator high = low;
			low--;

			// For normal bezier segments we calculate the phase:
			const float segmentLength = (high->first - low->first);
			const float segmentTime = (time - low->first);
			const float phase = (segmentLength > std::numeric_limits<float>::epsilon() ? (segmentTime / segmentLength) : 0.0f);

			// Finally, we evaluate and return the point:
			return KeyFrameType::Interpolate(low->second, high->second, phase);
		}

		/// <summary>
		/// Evaluates TimelineCurve at the given time point
		/// </summary>
		/// <param name="time"> Time point to evaluate the curve at </param>
		/// <returns> Interpolated value </returns>
		inline virtual ValueType Value(float time)const override {
			return Value(*this, time);
		}

		/// <summary>
		/// Attribute, shared among all TimelineCurve::Serializer instances (lets the editor know to plot an editable curve)
		/// </summary>
		class EditableTimelineCurveAttribute : public virtual Object {};

		/// <summary>
		/// Timeline curve serializer
		/// </summary>
		struct Serializer : public virtual Serialization::SerializerList::From<std::map<float, KeyFrameType>> {
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="name"> Name of the ItemSerializer </param>
			/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
			/// <param name="attributes"> Serializer attributes </param>
			inline Serializer(const std::string_view& name = "Timeline Curve", const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {})
				: Serialization::ItemSerializer(name, hint, [&]() -> std::vector<Reference<const Object>> {
				std::vector<Reference<const Object>> attrs = attributes;
				attrs.push_back(Object::Instantiate<EditableTimelineCurveAttribute>());
				return attrs;
					}()) {}

			/// <summary>
			/// Serializes TimelineCurve
			/// </summary>
			/// <typeparam name="ValueType"> Bezier value type </typeparam>
			/// <param name="recordElement"> Fields will be reported through this callback </param>
			/// <param name="target"> TimelineCurve to serialize </param>
			inline virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, std::map<float, KeyFrameType>* target)const override {
				size_t elemCount = target->size();
				{
					static const auto serializer = Serialization::DefaultSerializer<size_t>::Create("KeyFrame Count", "Number of Keyframes");
					recordElement(serializer->Serialize(elemCount));
				}

				static thread_local std::vector<std::pair<float, KeyFrameType>> keyFrames;
				const size_t startIndex = keyFrames.size();
				keyFrames.resize(startIndex + elemCount);

				{
					size_t index = startIndex;
					float lastT = 0.0f;
					KeyFrameType lastKeyFrame;
					for (auto it = target->begin(); it != target->end(); ++it) {
						if (index >= elemCount) break;
						keyFrames[index] = *it;
						lastT = it->first;
						lastKeyFrame = it->second;
						index++;
					}
					for (index = index; index < elemCount; index++)
						keyFrames[index] = std::make_pair(lastT, lastKeyFrame);
					target->clear();
				}

				for (size_t i = 0u; i < elemCount; i++) {
					struct KeyFrameSerializer : public virtual Serialization::SerializerList::From<std::pair<float, KeyFrameType>> {
						inline KeyFrameSerializer() : Serialization::ItemSerializer("KeyFrame", "Timeline curev Key Frame") {}
						inline virtual void GetFields(const Callback<Serialization::SerializedObject>& record, std::pair<float, KeyFrameType>* target)const final override {
							static const auto timeSerializer = Serialization::DefaultSerializer<float>::Create("Time", "KeyFrame time");
							record(timeSerializer->Serialize(target->first));
							static const auto keyFrameSerializer = Serialization::DefaultSerializer<KeyFrameType>::Create("Data", "KeyFrame data");
							record(keyFrameSerializer->Serialize(target->second));
						}
					};
					static const KeyFrameSerializer serializer;
					std::pair<float, KeyFrameType> entry = keyFrames[i + startIndex];
					recordElement(serializer.Serialize(entry));
					target->operator[](entry.first) = entry.second;
				}

				keyFrames.resize(startIndex);
			}
		};

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer should be reported by invoking this callback with serializer & corresonding target as parameters </param>
		inline virtual void GetFields(Callback<Serialization::SerializedObject> recordElement) override {
			static const Serializer serializer;
			serializer.GetFields(recordElement, this);
		}
	};


	/// <summary>
	/// Curve, describing a value evolving over time using a descrete evenly-spaced set of keyframes
	/// </summary>
	/// <typeparam name="ValueType"> Curve value type </typeparam>
	/// <typeparam name="KeyFrameType"> Keyframe type on the timeline </typeparam>
	template<typename ValueType, typename KeyFrameType = BezierNode<ValueType>>
	class DiscreteCurve : public virtual ParametricCurve<ValueType, float>, public virtual std::vector<KeyFrameType> {
	public:
		/// <summary>
		/// Evaluates DiscreteCurve at the given time point
		/// </summary>
		/// <param name="keyframes"> List of keyframes </param>
		/// <param name="keyframeCount"> Number of keyframes </param>
		/// <param name="time"> Time point to evaluate the curve at (integer part means keyframe index, fraction corresponds to the phase) </param>
		/// <returns> Interpolated value </returns>
		inline static ValueType Value(const KeyFrameType* keyframes, const size_t keyframeCount, float time) {
			// If timeline is empty, we return zero by default:
			if (keyframeCount <= 0 || keyframes == nullptr) return ValueType(0.0f);

			// Start index and phase:
			const size_t startIndex = static_cast<size_t>(time);
			const float phase = (time - static_cast<float>(startIndex));

			// If time is out of bounds, we just pick the first or the last value:
			if (time < 0.0f) return KeyFrameType::Interpolate(keyframes[0], keyframes[0], 0.0f);
			else {
				const size_t lastIndex = (keyframeCount - 1);
				if (startIndex >= lastIndex) return KeyFrameType::Interpolate(keyframes[lastIndex], keyframes[lastIndex], 0.0f);
			}
			
			// Finally, if all is good, we evaluate and return the point:
			return KeyFrameType::Interpolate(keyframes[startIndex], keyframes[startIndex + 1], phase);
		}

		/// <summary>
		/// Evaluates BezierCurve at the given time point
		/// </summary>
		/// <param name="curve"> Any Keyframe list </param>
		/// <param name="time"> Time point to evaluate the curve at (integer part means keyframe index, fraction corresponds to the phase) </param>
		/// <returns> Interpolated value </returns>
		inline static ValueType Value(const std::vector<KeyFrameType>& curve, float time) {
			return Value(curve.data(), curve.size(), time);
		}

		/// <summary>
		/// Evaluates DiscreteCurve at the given time point
		/// </summary>
		/// <param name="time"> Time point to evaluate the curve at (integer part means keyframe index, fraction corresponds to the phase) </param>
		/// <returns> Interpolated value </returns>
		inline virtual ValueType Value(float time)const override {
			return Value(*this, time);
		}
	};
}
