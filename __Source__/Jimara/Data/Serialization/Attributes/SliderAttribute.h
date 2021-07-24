#pragma once
#include "../../../Core/Object.h"


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Attribute for ValueSerializer<ValueType>, telling the editor to display the value as a slider
		/// </summary>
		/// <typeparam name="ValueType"> Scalar type </typeparam>
		template<typename ValueType>
		class SliderAttribute : public virtual Object {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="minValue"> Minimal value the target can take </param>
			/// <param name="maxValue"> Maximal value the target can take </param>
			/// <param name="minStep"> In case you want to discretize the slider, feel free to make this non-zero </param>
			inline SliderAttribute(ValueType minValue, ValueType maxValue, ValueType minStep = static_cast<ValueType>(0))
				: m_min(minValue), m_max(maxValue), m_minStep(minStep) {}

			/// <summary> Minimal value the target can take </summary>
			inline const ValueType Min()const { return m_min; };

			/// <summary> Maximal value the target can take </summary>
			inline const ValueType Max()const { return m_max; };

			/// <summary> 
			/// Minimal stepping between the valid values 
			/// (if greater than 0, ValueSerializer can only take values that can be computed as (Min + n * MinStep) or Max, where n is an integer) 
			/// </summary>
			inline const ValueType MinStep()const { return m_minStep; }

		private:
			// Minimal value the target can take
			ValueType m_min;

			// Maximal value the target can take
			ValueType m_max;

			// Minimal stepping between the valid values 
			ValueType m_minStep;
		};

		/** Here are all the slider types engine is aware of: */

		/// <summary> Integer slider </summary>
		typedef SliderAttribute<int> IntSliderAttribute;

		/// <summary> Unsigned integer slider </summary>
		typedef SliderAttribute<unsigned int> UintSliderAttribute;

		/// <summary> 32 bit integer slider </summary>
		typedef SliderAttribute<int32_t> Int32SliderAttribute;

		/// <summary> 32 bit unsigned integer slider </summary>
		typedef SliderAttribute<uint32_t> Uint32SliderAttribute;

		/// <summary> 64 bit integer slider </summary>
		typedef SliderAttribute<int64_t> Int64SliderAttribute;

		/// <summary> 64 bit unsigned integer slider </summary>
		typedef SliderAttribute<uint64_t> Uint64SliderAttribute;

		/// <summary> 32 bit (single precision) floating point slider </summary>
		typedef SliderAttribute<float> FloatSliderAttribute;

		/// <summary> 64 bit (double precision) floating point slider </summary>
		typedef SliderAttribute<double> DoubleSliderAttribute;
	}
}
