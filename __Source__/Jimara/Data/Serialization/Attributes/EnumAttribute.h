#pragma once
#include "../../../Core/Object.h"
#include <string_view>
#include <string>


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Attribute for ValueSerializer<ValueType>, telling the editor to display the value as a multiple choice enumeration or a bitmask
		/// </summary>
		/// <typeparam name="ValueType"> Integer type </typeparam>
		template<typename ValueType> 
		class EnumAttribute : public virtual Object {
		public:
			/// <summary>
			/// Enumeration/Bitmask bit value and name
			/// </summary>
			struct Choice {
				/// <summary> Enumeration/Bitmask name </summary>
				std::string name;

				/// <summary> Enumeration/Bitmask value (for bitmasks, has to be some arbitrary power of 2 for correct behaviour or a special value like "All<~0>" or "None<0>") </summary>
				ValueType value;

				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="n"> Enumeration/Bitmask name </param>
				/// <param name="v"> Enumeration/Bitmask value </param>
				inline Choice(const std::string_view& n = "", const ValueType& v = ValueType()) : name(n), value(v) {}
			};

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="choices"> Values and names </param>
			/// <param name="bitmask"> If true, the enumeration will be interpreted as "multiple choice bitmask" (not recommended for signed types) </param>
			inline EnumAttribute(const std::vector<Choice>& choices, bool bitmask) : m_choices(choices), m_isBitmask(bitmask) {}

			/// <summary> If true, the enumeration should be interpreted as "multiple choice bitmask" </summary>
			inline bool IsBitmask()const { return m_isBitmask; }

			/// <summary> Number of enumeration values </summary>
			inline size_t ChoiceCount()const { return m_choices.size(); }

			/// <summary>
			/// Enumeration/Bitmask bit value and name by index
			/// </summary>
			/// <param name="index"> Choice index </param>
			/// <returns> Enumeration value and name </returns>
			inline const Choice& operator[](size_t index)const { return m_choices[index]; }

		private:
			// Values and names
			const std::vector<Choice> m_choices;

			// Flag, marking a "multiple choice bitmask"
			const bool m_isBitmask;
		};


		/** Here are all the enum types engine is aware of: */

		/// <summary> Integer enumeration </summary>
		typedef EnumAttribute<int> IntEnumAttribute;

		/// <summary> Unsigned integer enumeration </summary>
		typedef EnumAttribute<unsigned int> UintEnumAttribute;

		/// <summary> 32 bit integer enumeration </summary>
		typedef EnumAttribute<int32_t> Int32EnumAttribute;

		/// <summary> 32 bit unsigned integer enumeration </summary>
		typedef EnumAttribute<uint32_t> Uint32EnumAttribute;

		/// <summary> 64 bit integer enumeration </summary>
		typedef EnumAttribute<int64_t> Int64EnumAttribute;

		/// <summary> 64 bit unsigned integer enumeration </summary>
		typedef EnumAttribute<uint64_t> Uint64EnumAttribute;
	}
}
