#pragma once
#include "Curves.h"
#include "../Graphics/GraphicsDevice.h"


namespace Jimara {
	/// <summary>
	/// Timeline bezier spline that resides on a GPU
	/// </summary>
	/// <typeparam name="ValueType"> Curve value type </typeparam>
	template<typename ValueType>
	class GraphicsTimelineCurve : public virtual ParametricCurve<ValueType, float>, public virtual Serialization::Serializable {
	public:
		/// <summary> ValueType byte alignment within KeyFrame structure </summary>
		static const constexpr size_t ValueTypeAlignment() {
			return sizeof(ValueType) <= 4u ? 4u : sizeof(ValueType) <= 8u ? 8u : 16u;
		};

		/// <summary>
		/// Timeline Key Frame
		/// </summary>
		struct KeyFrame {
			/// <summary> Value at this key frame </summary>
			alignas(ValueTypeAlignment()) ValueType value;

			/// <summary> Evaluation 'Time' of this key frame </summary>
			alignas(4) float time;

			/// <summary> 'Previous'/'Left' handle </summary>
			alignas(ValueTypeAlignment()) ValueType prevHandle;

			/// <summary> 'Next'/'Right' handle </summary>
			alignas(ValueTypeAlignment()) ValueType nextHandle;

			/// <summary> Interpolation mode flags </summary>
			alignas(4) uint32_t flags;
		};

		/// <summary> Interpolation mode flags </summary>
		enum class Flags : uint32_t {
			/// <summary> Nothing </summary>
			NONE = 0u,

			/// <summary> Value will be constant till the next key frame </summary>
			INTERPOLATE_CONSTANT = 1u << 0u,

			/// <summary> If INTERPOLATE_CONSTANT flag is set, value till the next key frame will be equal to the next keyframe </summary>
			INTERPOLATE_CONSTANT_NEXT = 1u << 1u
		};

		
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="device"> Graphics device (NULL not allowed) </param>
		/// <param name="name"> Curve 'name' </param>
		/// <param name="name"> Curve description of sorts </param>
		inline GraphicsTimelineCurve(Graphics::GraphicsDevice* device, const std::string_view& name = "Curve", const std::string_view& hint = "")
			: m_device(device), m_serializer(name, hint) {
			assert(m_device != nullptr);
		}

		/// <summary> Virtual destructor </summary>
		inline virtual ~GraphicsTimelineCurve() {}

		/// <summary> Sets content of the curve </summary>
		/// <param name="curve"> Content </param>
		inline void SetContent(const std::map<float, BezierNode<ValueType>>& curve) {
			{
				std::unique_lock<std::shared_mutex> lock(m_curveLock);
				if (curve.size() == m_curve.size()) {
					if (curve.size() <= 0u) return;
					auto it0 = m_curve.begin();
					auto it1 = curve.begin();
					auto end = m_curve.end();
					while (it0 != end) {
						if (it0->first != it1->first || it0->second != it1->second) break;
						++it0;
						++end;
					}
					if (it0 == end) return;
				}
				m_curve = curve;
				std::unique_lock<SpinLock> bufferLock(m_bufferLock);
				m_curveBuffer = nullptr;
			}
			m_onDirty(this);
		}

		/// <summary> Invoked whenever the curve content gets altered </summary>
		inline Event<GraphicsTimelineCurve> OnDirty()const { return m_onDirty; }

		/// <summary> Retrieves GPU buffer, corresponding to this curve </summary>
		inline Graphics::ArrayBufferReference<KeyFrame> GetCurveBuffer()const {
			Graphics::ArrayBufferReference<KeyFrame> buffer;
			auto updateBufferRef = [&]() {
				std::unique_lock<SpinLock> lock(m_bufferLock);
				buffer = m_curveBuffer;
				return buffer != nullptr;
			};
			if (updateBufferRef()) return buffer;

			std::shared_lock<std::shared_mutex> curveLock(m_curveLock);
			if (updateBufferRef()) return buffer;
			buffer = m_device->CreateArrayBuffer<KeyFrame>(m_curve.size());
			if (buffer == nullptr) {
				m_device->Log()->Error(
					"GraphicsTimelineCurve<", TypeId::Of<ValueType>().Name(), ">::GetCurveBuffer - ",
					"Failed to allocate GPU buffer! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return nullptr;
			}
			KeyFrame* frame = buffer.Map();
			for (auto it = m_curve.begin(); it != m_curve.end(); ++it) {
				frame->value = it->second.Value();
				frame->time = it->first;
				frame->prevHandle = it->second.PrevHandle();
				frame->nextHandle = it->second.NextHandle();
				const auto interpolateConstant = it->second.InterpolateConstant();
				frame->flags = interpolateConstant.active ? (static_cast<uint32_t>(Flags::INTERPOLATE_CONSTANT) |
					(interpolateConstant.next ? static_cast<uint32_t>(Flags::INTERPOLATE_CONSTANT_NEXT) : 0u)) : 0u;
				frame++;
			}
			buffer->Unmap(true);
			{
				std::unique_lock<SpinLock> bufferLock(m_bufferLock);
				m_curveBuffer = buffer;
			}
			return buffer;
		}

		/// <summary>
		/// Evaluates GraphicsTimelineCurve at the given time point
		/// </summary>
		/// <param name="time"> Time point to evaluate the curve at </param>
		/// <returns> Interpolated value </returns>
		inline virtual ValueType Value(float time)const override {
			std::shared_lock<std::shared_mutex> curveLock(m_curveLock);
			return TimelineCurve<ValueType>::Value(m_curve, time);
		}

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer should be reported by invoking this callback with serializer & corresonding target as parameters </param>
		inline virtual void GetFields(Callback<Serialization::SerializedObject> recordElement) override {
			static thread_local std::vector<std::pair<float, BezierNode<ValueType>>> initialData;
			initialData.clear();
			{
				std::unique_lock<std::shared_mutex> lock(m_curveLock);
				for (auto it = m_curve.begin(); it != m_curve.end(); ++it)
					initialData.push_back(*it);
				
				recordElement(m_serializer.Serialize(m_curve));

				if (m_curve.size() == initialData.size()) {
					auto it = m_curve.begin();
					const auto* initial = initialData.data();
					while (it != m_curve.end()) {
						if (it->first != initial->first || it->second != initial->second) break;
						++it;
						++initial;
					}
					if (it == m_curve.end()) {
						initialData.clear();
						return;
					}
				}

				initialData.clear();
				std::unique_lock<SpinLock> bufferLock(m_bufferLock);
				m_curveBuffer = nullptr;
			}
			m_onDirty(this);
		}

	private:
		// Graphics device
		const Reference<Graphics::GraphicsDevice> m_device;

		// Curve serializer
		const typename TimelineCurve<ValueType>::Serializer m_serializer;

		// Curve shape
		std::map<float, BezierNode<ValueType>> m_curve;

		// Lock for the curve shape
		mutable std::shared_mutex m_curveLock;

		// Lock for the buffer
		mutable SpinLock m_bufferLock;

		// Curve buffer
		mutable Graphics::ArrayBufferReference<KeyFrame> m_curveBuffer;

		// Invoked whenever the data gets dirty
		mutable EventInstance<GraphicsTimelineCurve*> m_onDirty;
	};
}
