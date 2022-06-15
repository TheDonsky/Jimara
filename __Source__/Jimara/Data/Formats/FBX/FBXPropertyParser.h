#pragma once
#include "FBXContent.h"
#include <unordered_set>


namespace Jimara {
	namespace FBXHelpers {
		/// <summary>
		/// A helper for extracting properties from Properties70 node
		/// </summary>
		class JIMARA_API FBXPropertyParser {
		public:
			/// <summary> Parse function (params: userData, propertyNode, logger) </summary>
			typedef bool(*ParseFn)(void*, const FBXContent::Node&, OS::Logger*);

			/// <summary> Parse function, paired with a name of a property the function is supposed to parse </summary>
			typedef std::pair<std::string_view, ParseFn> ParserForPropertyName;

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parsersPerPropertyName"> ParserForPropertyName pairs to create logical mappings </param>
			/// <param name="count"> Number of elements within parsersPerPropertyName </param>
			inline FBXPropertyParser(const ParserForPropertyName* parsersPerPropertyName, size_t count) : m_internals(parsersPerPropertyName, count) {}

			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="parsersPerPropertyName"> ParserForPropertyName pairs to create logical mappings </param>
			template<size_t Count>
			inline FBXPropertyParser(const ParserForPropertyName(&parsersPerPropertyName)[Count]) : FBXPropertyParser(parsersPerPropertyName, Count) {}

			/// <summary>
			/// Attempts to parse properties from Properties70 node
			/// </summary>
			/// <param name="userData"> User data that will be passed to ParseFn functions when they get invoked </param>
			/// <param name="properties70Node"> Properties70 node or any other node containing similar data </param>
			/// <param name="logger"> Logger for error and warning reporting (passed to ParseFn functions) </param>
			/// <returns> True, if no error occures </returns>
			inline bool ParseProperties(void* userData, const FBXContent::Node* properties70Node, OS::Logger* logger)const {
				for (size_t propertyId = 0; propertyId < properties70Node->NestedNodeCount(); propertyId++) {
					const FBXContent::Node& propertyNode = properties70Node->NestedNode(propertyId);
					if (propertyNode.PropertyCount() < 4) {
						if (logger != nullptr) logger->Warning("FBXData::Extract::PropertyExtractor::ExtractProperties - Properties70 node contains a non-property entry...");
						continue;
					}
					std::string_view propName, propType, propLabel, propFlags;
					auto warning = [&](auto... message) { if (logger != nullptr) logger->Warning(message...); };
					if (!propertyNode.NodeProperty(0).Get(propName)) { warning("FBXPropertyParser::ParseProperties - Properties70 node contains a property with no PropName..."); continue; }
					if (!propertyNode.NodeProperty(1).Get(propType)) { warning("FBXPropertyParser::ParseProperties - Properties70 node contains a property with no PropType..."); continue; }
					if (!propertyNode.NodeProperty(2).Get(propLabel)) { warning("FBXPropertyParser::ParseProperties - Properties70 node contains a property with no Label..."); continue; }
					if (!propertyNode.NodeProperty(3).Get(propFlags)) { warning("FBXPropertyParser::ParseProperties - Properties70 node contains a property with no Flags..."); continue; }
					std::unordered_map<std::string_view, ParseFn>::const_iterator it = m_internals.parsersPerName.find(propName);
					if (it != m_internals.parsersPerName.end())
						if (!it->second(userData, propertyNode, logger)) return false;
				}
				return true;
			}

			/// <summary>
			/// Gets the name of a property
			/// Note: safe to use from ParseFn callbacks; otherwise, just make sure the node has at least one property and it's of a STRING type.
			/// </summary>
			/// <param name="propertyNode"> Property node from an FBX tree (has to be valid) </param>
			/// <returns> Property name </returns>
			static std::string_view PropertyName(const FBXContent::Node& propertyNode) { return propertyNode.NodeProperty(0); }


			/// <summary>
			/// Filter result for "Parse" type functions below
			/// </summary>
			enum class FilterResult : uint8_t {
				/// <summary> No error; use extracted value </summary>
				PASS,

				/// <summary> Extracted value should be ignored, but this is not an error and parse status should not be false </summary>
				IGNORE_VALUE,

				/// <summary> Parsing should report error </summary>
				FAIL
			};

			/// <summary>
			/// Filter for "Parse" type functions that always approves the value
			/// </summary>
			/// <typeparam name="ValueType"> Any value type </typeparam>
			template<typename ValueType>
			struct NoFilter { 
				/// <summary>
				/// Returns true
				/// </summary>
				/// <param name=""> ignored </param>
				/// <param name=""> ignored </param>
				/// <param name=""> ignored </param>
				/// <returns> True </returns>
				inline static FilterResult Filter(const ValueType&, const FBXContent::Node&, OS::Logger*) { return FilterResult::PASS; } 
			};

			/// <summary>
			/// Filter for "Parse" type functions that ignores negative values and approves zeroes and positive numbers
			/// </summary>
			/// <typeparam name="ValueType"> Any numeric type </typeparam>
			template<typename ValueType>
			struct IgnoreIfNegative {
				/// <summary>
				/// Filters value
				/// </summary>
				/// <param name="value"> Value to examine </param>
				/// <param name=""> ignored </param>
				/// <param name=""> ignored </param>
				/// <returns> IGNORE_VALUE if value < 0; PASS otherwise </returns>
				inline static FilterResult Filter(const ValueType& value, const FBXContent::Node&, OS::Logger*) { return value < 0 ? FilterResult::IGNORE_VALUE : FilterResult::PASS; } 
			};

			/// <summary>
			/// Default CastToEnum parameter for ParseEnumProperty
			/// (can cast int64_t to an enumeration)
			/// </summary>
			struct DefaultCastToEnum {
				/// <summary>
				/// Type casts int64_t value to a simple 'enum class'
				/// Notes:
				///		0. EnumType has to be an 'enum class' or a namespace name;
				///		1. Enumeration values should cover all unsigned integers from 0 to EnumType::ENUM_SIZE;
				///		2. EnumType::ENUM_SIZE has to be an actual thing inside the enumeration
				/// </summary>
				/// <typeparam name="EnumType"> Enumeration type </typeparam>
				/// <param name="value"> Value to cast </param>
				/// <param name="enumValue"> Result will be stored here if successful </param>
				/// <param name=""> ignored </param>
				/// <param name=""> ignored </param>
				/// <returns> True, if type cast from int64_t to EnumType was valid and, therefore, successful </returns>
				template<typename EnumType>
				inline static bool Cast(int64_t value, EnumType& enumValue, const FBXContent::Node&, OS::Logger*) {
					if (value < 0 || value > static_cast<int64_t>(EnumType::ENUM_SIZE)) return false;
					enumValue = static_cast<EnumType>(value);
					return true;
				}
			};

			/// <summary>
			/// Interprets property node as a storage for an enumeration value and attempts to parse it
			/// </summary>
			/// <typeparam name="EnumType"> Enumeration type </typeparam>
			/// <typeparam name="PreFilterType"> Filter for int64_t value before attempting to type cast it to the enumeration </typeparam>
			/// <typeparam name="CastToEnum"> Tool for casting int64_t to EnumType </typeparam>
			/// <typeparam name="FilterType"> Filter for the final enumeration value </typeparam>
			/// <typeparam name="...PropertyPath"> In case an error occures, some message with property name will be logged. In case you want to add a preffix to the name, add arbitary things here </typeparam>
			/// <param name="value"> Reference to the output value </param>
			/// <param name="propertyNode"> Node to extract value from </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="...propertyPath"> In case an error occures, some message with property name will be logged. In case you want to add a preffix to the name, add arbitary things here </param>
			/// <returns> True, if the value gets extracted successfully </returns>
			template<typename EnumType, typename PreFilterType = NoFilter<int64_t>, typename CastToEnum = DefaultCastToEnum, typename FilterType = NoFilter<EnumType>, typename... PropertyPath>
			inline static bool ParseEnumProperty(EnumType& value, const FBXContent::Node& propertyNode, OS::Logger* logger, const PropertyPath&... propertyPath) {
				if (propertyNode.PropertyCount() < 5) return true; // We automatically ignore this if value is missing
				int64_t tmp;
				if (!propertyNode.NodeProperty(4).Get(tmp)) {
					if (logger != nullptr) logger->Error("FBXPropertyParser::ParseProperties - ", propertyPath..., PropertyName(propertyNode), " is not an integer type!");
					return false;
				}
				else {
					FilterResult result = PreFilterType::Filter(tmp, propertyNode, logger);
					if (result == FilterResult::IGNORE_VALUE) return true;
					else if (result == FilterResult::FAIL) return false;
				}
				EnumType enumValue;
				if (!CastToEnum::Cast(tmp, enumValue, propertyNode, logger)) {
					if (logger != nullptr) logger->Error("FBXPropertyParser::ParseProperties - ", propertyPath..., PropertyName(propertyNode), "<", tmp, "> not a valid enumeration value!");
					return false;
				}
				else {
					FilterResult result = FilterType::Filter(enumValue, propertyNode, logger);
					if (result == FilterResult::PASS) {
						value = enumValue;
						return true;
					}
					else return result != FilterResult::FAIL;
				}
			}

			/// <summary>
			/// Interprets property node as a storage for a signed integer value and attempts to parse it
			/// </summary>
			/// <typeparam name="FilterType"> Filter for the value </typeparam>
			/// <typeparam name="...PropertyPath"> In case an error occures, some message with property name will be logged. In case you want to add a preffix to the name, add arbitary things here </typeparam>
			/// <param name="value"> Reference to the output value </param>
			/// <param name="propertyNode"> Node to extract value from </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="...propertyPath"> In case an error occures, some message with property name will be logged. In case you want to add a preffix to the name, add arbitary things here </param>
			/// <returns> True, if the value gets extracted successfully </returns>
			template<typename FilterType = NoFilter<int64_t>, typename... PropertyPath>
			inline static bool ParseProperty(int64_t& value, const FBXContent::Node& propertyNode, OS::Logger* logger, const PropertyPath&... propertyPath) {
				if (propertyNode.PropertyCount() < 5) return true; // We automatically ignore this if value is missing
				int64_t tmp;
				if (!propertyNode.NodeProperty(4).Get(tmp)) {
					if (logger != nullptr) logger->Error("FBXPropertyParser::ParseProperties - ", propertyPath..., PropertyName(propertyNode), " is not an integer type!");
					return false;
				}
				FilterResult result = FilterType::Filter(tmp, propertyNode, logger);
				if (result == FilterResult::PASS) {
					value = tmp;
					return true;
				}
				else return result != FilterResult::FAIL;
			}

			/// <summary>
			/// Interprets property node as a storage for a boolean value and attempts to parse it
			/// </summary>
			/// <typeparam name="FilterType"> Filter for the value </typeparam>
			/// <typeparam name="...PropertyPath"> In case an error occures, some message with property name will be logged. In case you want to add a preffix to the name, add arbitary things here </typeparam>
			/// <param name="value"> Reference to the output value </param>
			/// <param name="propertyNode"> Node to extract value from </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="...propertyPath"> In case an error occures, some message with property name will be logged. In case you want to add a preffix to the name, add arbitary things here </param>
			/// <returns> True, if the value gets extracted successfully </returns>
			template<typename FilterType = NoFilter<bool>, typename... PropertyPath>
			inline static bool ParseProperty(bool& value, const FBXContent::Node& propertyNode, OS::Logger* logger, const PropertyPath&... propertyPath) {
				if (propertyNode.PropertyCount() < 5) return true; // We automatically ignore this if value is missing
				int64_t tmp;
				if (!propertyNode.NodeProperty(4).Get(tmp)) {
					if (logger != nullptr) logger->Error("FBXPropertyParser::ParseProperties - ", propertyPath..., PropertyName(propertyNode), " is not an boolean or an integer type!");
					return false;
				}
				bool booleanValue = (tmp != 0);
				FilterResult result = FilterType::Filter(booleanValue, propertyNode, logger);
				if (result == FilterResult::PASS) {
					value = booleanValue;
					return true;
				}
				else return result != FilterResult::FAIL;
			}

			/// <summary>
			/// Interprets property node as a storage for a floating point value and attempts to parse it
			/// </summary>
			/// <typeparam name="FilterType"> Filter for the value </typeparam>
			/// <typeparam name="...PropertyPath"> In case an error occures, some message with property name will be logged. In case you want to add a preffix to the name, add arbitary things here </typeparam>
			/// <param name="value"> Reference to the output value </param>
			/// <param name="propertyNode"> Node to extract value from </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="...propertyPath"> In case an error occures, some message with property name will be logged. In case you want to add a preffix to the name, add arbitary things here </param>
			/// <returns> True, if the value gets extracted successfully </returns>
			template<typename FilterType = NoFilter<float>, typename... PropertyPath>
			inline static bool ParseProperty(float& value, const FBXContent::Node& propertyNode, OS::Logger* logger, const PropertyPath&... propertyPath) {
				if (propertyNode.PropertyCount() < 5) return true; // We automatically ignore this if value is missing
				float tmp;
				if (!propertyNode.NodeProperty(4).Get(tmp)) {
					if (logger != nullptr) logger->Error(logger, false, "FBXPropertyParser::ParseProperties - ", propertyPath..., PropertyName(propertyNode), " is not a floating point!");
					return false;
				}
				FilterResult result = FilterType::Filter(tmp, propertyNode, logger);
				if (result == FilterResult::PASS) {
					value = tmp;
					return true;
				}
				else return result != FilterResult::FAIL;
			}

			/// <summary>
			/// Interprets property node as a storage for a 3d vector value and attempts to parse it
			/// </summary>
			/// <typeparam name="FilterType"> Filter for the value </typeparam>
			/// <typeparam name="...PropertyPath"> In case an error occures, some message with property name will be logged. In case you want to add a preffix to the name, add arbitary things here </typeparam>
			/// <param name="value"> Reference to the output value </param>
			/// <param name="propertyNode"> Node to extract value from </param>
			/// <param name="logger"> Logger for error reporting </param>
			/// <param name="...propertyPath"> In case an error occures, some message with property name will be logged. In case you want to add a preffix to the name, add arbitary things here </param>
			/// <returns> True, if the value gets extracted successfully </returns>
			template<typename FilterType = NoFilter<Vector3>, typename... PropertyPath>
			inline static bool ParseProperty(Vector3& value, const FBXContent::Node& propertyNode, OS::Logger* logger, const PropertyPath&... propertyPath) {
				if (propertyNode.PropertyCount() < 7) {
					if (propertyNode.PropertyCount() > 4) return true; // We automatically ignore this if value is fully missing
					else {
						if (logger != nullptr) logger->Error("FBXPropertyParser::ParseProperties - ", propertyPath..., PropertyName(propertyNode), " does not hold 3d vector value!");
						return false;
					}
				}
				Vector3 tmp;
				if (!propertyNode.NodeProperty(4).Get(tmp.x)) {
					if (logger != nullptr) logger->Error("FBXPropertyParser::ParseProperties - ", propertyPath..., PropertyName(propertyNode), ".x is not a floating point!");
					return false;
				}
				else if (!propertyNode.NodeProperty(5).Get(tmp.y)) {
					if (logger != nullptr) logger->Error("FBXPropertyParser::ParseProperties - ", propertyPath..., PropertyName(propertyNode), ".y is not a floating point!");
					return false;
				}
				else if (!propertyNode.NodeProperty(6).Get(tmp.z)) {
					if (logger != nullptr) logger->Error("FBXData::Extract::PropertyParser::ParseProperty - ", propertyPath..., PropertyName(propertyNode), ".z is not a floating point!");
					return false;
				}
				FilterResult result = FilterType::Filter(tmp, propertyNode, logger);
				if (result == FilterResult::PASS) {
					value = tmp;
					return true;
				}
				else return result != FilterResult::FAIL;
			}

		private:
			// ParseFn to name translation unit
			struct Internals {
				// Allocation for string_view objects
				std::vector<char> propertyNames;

				// Property name to ParseFn mapping
				std::unordered_map<std::string_view, ParseFn> parsersPerName;

				// Constructor
				inline Internals(const ParserForPropertyName* parsersPerPropertyName, size_t count) {
					for (size_t i = 0; i < count; i++) {
						const std::string_view& name = parsersPerPropertyName[i].first;
						for (size_t j = 0; j < name.size(); j++) propertyNames.push_back(name[j]);
						propertyNames.push_back('\0');
					}
					size_t ptr = 0;
					for (size_t i = 0; i < count; i++) {
						const std::pair<std::string_view, ParseFn>& entry = parsersPerPropertyName[i];
						parsersPerName[std::string_view(propertyNames.data() + ptr, entry.first.size())] = entry.second;
						ptr += entry.first.size() + 1;
					}
				}
			};

			// ParseFn to name translation unit
			const Internals m_internals;
		};
	}
}
