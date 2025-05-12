#pragma once
#include "../../../Core/Object.h"
#include "../../../Core/Helpers.h"
#include "../ItemSerializers.h"
#include <string_view>
#include <string>


namespace Jimara {
	namespace Serialization {
		/// <summary>
		/// Generic enumeration choice provider
		/// </summary>
		/// <typeparam name="ValueType"> Primitive type </typeparam>
		template<typename ValueType>
		class EnumerableChoiceProviderAttribute : public virtual Object {
		public:
			using ChoiceValueType =
				std::conditional_t<std::is_same_v<ValueType, std::string_view>, std::string,
				std::conditional_t<std::is_same_v<ValueType, std::wstring_view>, std::wstring, ValueType>>;

			/// <summary>
			/// Enumeration/Bitmask bit value and name
			/// </summary>
			struct Choice {
				/// <summary> Enumeration/Bitmask name </summary>
				std::string name;

				/// <summary> Enumeration/Bitmask value (for bitmasks, has to be some arbitrary power of 2 for correct behaviour or a special value like "All<~0>" or "None<0>") </summary>
				ChoiceValueType value;

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
			/// Enumeration choice provider object
			/// </summary>
			struct ChoiceProvider : public virtual Object {
				/// <summary>
				/// Reports possible choices
				/// </summary>
				/// <param name="targetObject"> Target serialized object </param>
				/// <param name="reportChoice"> This callback can be used to report individual choices </param>
				inline virtual void GetChoices(const Serialization::SerializedObject& targetObject, const Callback<const Choice&> reportChoice)const = 0;
			};

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="isBitmask"> If true, the enumeration will be interpreted as "multiple choice bitmask" (not recommended for signed types) </param>
			/// <param name="choiceProvider"> Choice-provider object </param>
			inline EnumerableChoiceProviderAttribute(bool isBitmask, const Reference<const ChoiceProvider>& choiceProvider)
				: m_isBitmask(isBitmask), m_choiceProvider(choiceProvider) {}

			/// <summary> Virtual destructor </summary>
			inline virtual ~EnumerableChoiceProviderAttribute() {}

			/// <summary> If true, the enumeration should be interpreted as "multiple choice bitmask" </summary>
			inline bool IsBitmask()const { return m_isBitmask; }

			/// <summary>
			/// Reports possible choices
			/// </summary>
			/// <param name="targetObject"> Target serialized object </param>
			/// <param name="reportChoice"> This callback can be used to report individual choices </param>
			inline void GetChoices(const Serialization::SerializedObject& targetObject, const Callback<const Choice&> reportChoice)const {
				if (m_choiceProvider != nullptr)
					m_choiceProvider->GetChoices(targetObject, reportChoice);
			}

			/// <summary>
			/// Reports possible choices
			/// </summary>
			/// <typeparam name="ReportFn"> Any callable, that can receive an immutable Choice as an argument </typeparam>
			/// <param name="targetObject"> Target serialized object </param>
			/// <param name="reportChoice"> This callback can be used to report individual choices </param>
			template<typename ReportFn>
			inline void GetChoices(const Serialization::SerializedObject& targetObject, const ReportFn& reportChoice)const { 
				GetChoices(targetObject, Callback<const Choice&>::FromCall(&reportChoice));
			}

			/// <summary> Underlying Choice-provider </summary>
			inline const ChoiceProvider* GetChoiceProvider()const { return m_choiceProvider; }

		private:
			// Flag, marking a "multiple choice bitmask"
			const bool m_isBitmask;

			// Underlying choice provider
			const Reference<const ChoiceProvider> m_choiceProvider;
		};

		/// <summary>
		/// Attribute for ValueSerializer<ValueType>, telling the editor to display the value as a multiple choice enumeration or a bitmask
		/// </summary>
		/// <typeparam name="ValueType"> Primitive type </typeparam>
		template<typename ValueType> 
		class EnumAttribute : public virtual EnumerableChoiceProviderAttribute<ValueType> {
		public:
			using Choice = typename EnumerableChoiceProviderAttribute<ValueType>::Choice;

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
				: EnumerableChoiceProviderAttribute<ValueType>(bitmask, ([&]() -> Reference<const ChoiceProvider> {
				Reference<ChoiceList> choiceList = Object::Instantiate<ChoiceList>();
				CollectChoices(choiceList->choices, choices...);
				return choiceList;
					})()) {}

			/// <summary> Number of enumeration values </summary>
			inline size_t ChoiceCount()const { 
				return dynamic_cast<const ChoiceList*>(EnumerableChoiceProviderAttribute<ValueType>::GetChoiceProvider())->choices.size();
			}

			/// <summary>
			/// Enumeration/Bitmask bit value and name by index
			/// </summary>
			/// <param name="index"> Choice index </param>
			/// <returns> Enumeration value and name </returns>
			inline const Choice& operator[](size_t index)const { 
				return dynamic_cast<const ChoiceList*>(EnumerableChoiceProviderAttribute<ValueType>::GetChoiceProvider())->choices[index];
			}

		private:
			// Values and names
			using ChoiceProvider = typename EnumerableChoiceProviderAttribute<ValueType>::ChoiceProvider;
			struct ChoiceList : public virtual ChoiceProvider {
				std::vector<Choice> choices;
				inline virtual void GetChoices(const Serialization::SerializedObject& targetObject, const Callback<const Choice&> reportChoice)const override {
					Unused(targetObject);
					for (size_t i = 0u; i < choices.size(); i++)
						reportChoice(choices[i]);
				}
			};

			// 'Collects' choices:
			inline static void CollectChoices(std::vector<Choice>& choices) {}
			template<typename Name, typename Value, typename... Choices>
			inline static void CollectChoices(std::vector<Choice>& choices, const Name& name, const Value& value, const Choices&... rest) {
				choices.push_back(Choice(name, value));
				CollectChoices(choices, rest...);
			}

			// Internal constructor
			inline EnumAttribute(const Choice* choices, size_t choiceCount, bool bitmask) 
				: EnumerableChoiceProviderAttribute<ValueType>(bitmask, ([&]() -> Reference<const ChoiceProvider> {
					Reference<ChoiceList> choiceList = Object::Instantiate<ChoiceList>();
					for (size_t i = 0u; i < choiceCount; i++)
						choiceList->choices.push_back(choices[i]);
					return choiceList;
					})()) {
			}
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
