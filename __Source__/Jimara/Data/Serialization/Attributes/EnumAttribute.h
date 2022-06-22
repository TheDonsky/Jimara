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

				/// <summary>
				/// Constructor
				/// </summary>
				/// <typeparam name="Value"> Anything, that can be statically type-cast to ValueType (useful for enumerations) </typeparam>
				/// <param name="n"> Enumeration/Bitmask name </param>
				/// <param name="v"> Enumeration/Bitmask value </param>
				template<typename Value>
				inline Choice(const std::string_view& n = "", const Value& v = Value()) : name(n), value(static_cast<ValueType>(v)) {}
			};

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="choices"> Values and names </param>
			/// <param name="bitmask"> If true, the enumeration will be interpreted as "multiple choice bitmask" (not recommended for signed types) </param>
			inline EnumAttribute(const std::vector<Choice>& choices, bool bitmask) : EnumAttribute(choices.data(), choices.size(), bitmask) {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <typeparam name="ChoiceCount"> Number of choices </typeparam>
			/// <param name="choices"> Values and names </param>
			/// <param name="bitmask"> If true, the enumeration will be interpreted as "multiple choice bitmask" (not recommended for signed types) </param>
			template<size_t ChoiceCount>
			inline EnumAttribute(const Choice(&choices)[ChoiceCount], bool bitamsk) : EnumAttribute(choices, ChoiceCount, bitamsk) {}

			/// <summary>
			/// Constructor
			/// <para/> Use: EnumAttribute(true/false, "A", valA, "B", valB, ...);
			/// </summary>
			/// <typeparam name="...Choices"> Name, then value, then name, then value and so on </typeparam>
			/// <param name="bitmask"> If true, the enumeration will be interpreted as "multiple choice bitmask" (not recommended for signed types) </param>
			/// <param name="...choices"> Values and names </param>
			template<typename... Choices>
			inline EnumAttribute(bool bitmask, const Choices&... choices)
				: m_choices([&]() -> std::vector<Choice> {
				std::vector<Choice> rv;
				CollectChoices(rv, choices...);
				return rv;
					}())
				, m_isBitmask(bitmask) {}

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

			// 'Collects' choices:
			inline static void CollectChoices(std::vector<Choice>& choices) {}
			template<typename Name, typename Value, typename... Choices>
			inline static void CollectChoices(std::vector<Choice>& choices, const Name& name, const Value& value, const Choices&... rest) {
				choices.push_back(Choice(name, value));
				CollectChoices(choices, rest...);
			}

			// Internal constructor
			inline EnumAttribute(const Choice* choices, size_t choiceCount, bool bitmask) : m_choices(choices, choices + choiceCount), m_isBitmask(bitmask) {}
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
