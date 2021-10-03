#include "FBXData.h"
#include <stddef.h>
#include <unordered_map>
#include <map>
#include <stdint.h>
#include <algorithm>


namespace Jimara {
	namespace {
		// Some meaningless helper for 
		template<typename ReturnType, typename... MessageTypes>
		inline static ReturnType Error(OS::Logger* logger, const ReturnType& returnValue, const MessageTypes&... message) {
			if (logger != nullptr) logger->Error(message...);
			return returnValue;
		}


		// Generic property parser and extractor:
		class PropertyParser {
		public:
			typedef bool(*ParseFn)(void*, const FBXContent::Node&, OS::Logger*);
			inline PropertyParser() : m_propertyName(""), m_parseFn([](void*, const FBXContent::Node&, OS::Logger*)->bool { return true; }) {}
			inline PropertyParser(const std::string_view& propertyName, const ParseFn& parseFn) : m_propertyName(propertyName), m_parseFn(parseFn) {}
			inline const std::string_view& PropertyName()const { return m_propertyName; }
			inline bool Parse(void* target, const FBXContent::Node& propertyNode, OS::Logger* logger)const { return m_parseFn(target, propertyNode, logger); }
			
			static const std::string_view NameOf(const FBXContent::Node& propertyNode) { return propertyNode.NodeProperty(0); }

			enum class FilterResult : uint8_t {
				PASS,
				IGNORE_VALUE,
				FAIL
			};

			template<typename ValueType>
			struct NoFilter { inline static FilterResult Filter(const ValueType&, const FBXContent::Node&, OS::Logger*) { return FilterResult::PASS; } };

			template<typename ValueType>
			struct IgnoreIfNegative { inline static FilterResult Filter(const ValueType& value, const FBXContent::Node&, OS::Logger*) { return value < 0 ? FilterResult::IGNORE_VALUE : FilterResult::PASS; } };

			struct DefaultCastToEnum {
				template<typename EnumType>
				inline static bool Cast(int64_t value, EnumType& enumValue, const FBXContent::Node&, OS::Logger*) {
					if (value < 0 || value > static_cast<int64_t>(EnumType::ENUM_SIZE)) return false;
					enumValue = static_cast<EnumType>(value);
					return true;
				}
			};

			template<typename EnumType, typename PreFilterType = NoFilter<int64_t>, typename CastToEnum = DefaultCastToEnum, typename FilterType = NoFilter<EnumType>, typename... PropertyPath>
			inline static bool ParseEnumProperty(EnumType& value, const FBXContent::Node& propertyNode, OS::Logger* logger, const PropertyPath&... propertyPath) {
				if (propertyNode.PropertyCount() < 5) return true; // We automatically ignore this if value is missing
				int64_t tmp;
				if (!propertyNode.NodeProperty(4).Get(tmp))
					return Error(logger, false, "FBXData::Extract::PropertyParser::ParseEnumProperty - ", propertyPath..., PropertyParser::NameOf(propertyNode), " is not an integer type!");
				else {
					FilterResult result = PreFilterType::Filter(tmp, propertyNode, logger);
					if (result == FilterResult::IGNORE_VALUE) return true;
					else if (result == FilterResult::FAIL) return false;
				}
				EnumType enumValue;
				if (!CastToEnum::Cast(tmp, enumValue, propertyNode, logger))
					return Error(logger, false, "FBXData::Extract::PropertyParser::ParseEnumProperty - ", propertyPath..., PropertyParser::NameOf(propertyNode), "<", tmp, "> not a valid enumeration value!");
				else {
					FilterResult result = FilterType::Filter(enumValue, propertyNode, logger);
					if (result == FilterResult::PASS) {
						value = enumValue;
						return true;
					}
					else return result != FilterResult::FAIL;
				}
			}

			template<typename FilterType = NoFilter<int64_t>, typename... PropertyPath>
			inline static bool ParseProperty(int64_t& value, const FBXContent::Node& propertyNode, OS::Logger* logger, const PropertyPath&... propertyPath) {
				if (propertyNode.PropertyCount() < 5) return true; // We automatically ignore this if value is missing
				int64_t tmp;
				if (!propertyNode.NodeProperty(4).Get(tmp))
					return Error(logger, false, "FBXData::Extract::PropertyParser::ParseProperty - ", propertyPath..., PropertyParser::NameOf(propertyNode), " is not an integer type!");
				FilterResult result = FilterType::Filter(tmp, propertyNode, logger);
				if (result == FilterResult::PASS) {
					value = tmp;
					return true;
				}
				else return result != FilterResult::FAIL;
			}

			template<typename FilterType = NoFilter<bool>, typename... PropertyPath>
			inline static bool ParseProperty(bool& value, const FBXContent::Node& propertyNode, OS::Logger* logger, const PropertyPath&... propertyPath) {
				if (propertyNode.PropertyCount() < 5) return true; // We automatically ignore this if value is missing
				int64_t tmp;
				if (!propertyNode.NodeProperty(4).Get(tmp))
					return Error(logger, false, "FBXData::Extract::PropertyParser::ParseProperty - ", propertyPath..., PropertyParser::NameOf(propertyNode), " is not an boolean or an integer type!");
				bool booleanValue = (tmp != 0);
				FilterResult result = FilterType::Filter(booleanValue, propertyNode, logger);
				if (result == FilterResult::PASS) {
					value = booleanValue;
					return true;
				}
				else return result != FilterResult::FAIL;
			}

			template<typename FilterType = NoFilter<float>, typename... PropertyPath>
			inline static bool ParseProperty(float& value, const FBXContent::Node& propertyNode, OS::Logger* logger, const PropertyPath&... propertyPath) {
				if (propertyNode.PropertyCount() < 5) return true; // We automatically ignore this if value is missing
				float tmp;
				if (!propertyNode.NodeProperty(4).Get(tmp))
					return Error(logger, false, "FBXData::Extract::PropertyParser::ParseProperty - ", propertyPath..., PropertyParser::NameOf(propertyNode), " is not a floating point!");
				FilterResult result = FilterType::Filter(tmp, propertyNode, logger);
				if (result == FilterResult::PASS) {
					value = tmp;
					return true;
				}
				else return result != FilterResult::FAIL;
			}

			template<typename FilterType = NoFilter<Vector3>, typename... PropertyPath>
			inline static bool ParseProperty(Vector3& value, const FBXContent::Node& propertyNode, OS::Logger* logger, const PropertyPath&... propertyPath) {
				if (propertyNode.PropertyCount() < 7) {
					if (propertyNode.PropertyCount() > 4) return true; // We automatically ignore this if value is fully missing
					else return Error(logger, false, "FBXData::Extract::PropertyParser::ParseProperty - ", propertyPath..., PropertyParser::NameOf(propertyNode), " does not hold 3d vector value!");
				}
				Vector3 tmp;
				if (!propertyNode.NodeProperty(4).Get(tmp.x))
					return Error(logger, false, "FBXData::Extract::PropertyParser::ParseProperty - ", propertyPath..., PropertyParser::NameOf(propertyNode), ".x is not a floating point!");
				else if (!propertyNode.NodeProperty(5).Get(tmp.y))
					return Error(logger, false, "FBXData::Extract::PropertyParser::ParseProperty - ", propertyPath..., PropertyParser::NameOf(propertyNode), ".y is not a floating point!");
				else if (!propertyNode.NodeProperty(6).Get(tmp.z))
					return Error(logger, false, "FBXData::Extract::PropertyParser::ParseProperty - ", propertyPath..., PropertyParser::NameOf(propertyNode), ".z is not a floating point!");
				FilterResult result = FilterType::Filter(tmp, propertyNode, logger);
				if (result == FilterResult::PASS) {
					value = tmp;
					return true;
				}
				else return result != FilterResult::FAIL;
			}

		private:
			std::string_view m_propertyName;
			ParseFn m_parseFn;
		};

		class PropertyExtractor {
		private:
			typedef std::unordered_map<std::string_view, PropertyParser> ParserMap;
			const ParserMap m_parsers;

		public:
			template<size_t Count>
			inline PropertyExtractor(const PropertyParser(&parsers)[Count]) 
				: m_parsers([&]() ->ParserMap {
				ParserMap parserMap;
				for (size_t i = 0; i < Count; i++)
					parserMap[parsers[i].PropertyName()] = parsers[i];
				return parserMap;
					}()) {
			}

			inline bool ExtractProperties(void* target, const FBXContent::Node* properties70Node, OS::Logger* logger)const {
				for (size_t propertyId = 0; propertyId < properties70Node->NestedNodeCount(); propertyId++) {
					const FBXContent::Node& propertyNode = properties70Node->NestedNode(propertyId);
					if (propertyNode.PropertyCount() < 4) {
						if (logger != nullptr) logger->Warning("FBXData::Extract::PropertyExtractor::ExtractProperties - Properties70 node contains a non-property entry...");
						continue;
					}
					std::string_view propName, propType, propLabel, propFlags;
					auto warning = [&](auto... message) { if (logger != nullptr) logger->Warning(message...); };
					if (!propertyNode.NodeProperty(0).Get(propName)) { warning("FBXData::Extract::PropertyExtractor::ExtractProperties - Properties70 node contains a property with no PropName..."); continue; }
					if (!propertyNode.NodeProperty(1).Get(propType)) { warning("FBXData::Extract::PropertyExtractor::ExtractProperties - Properties70 node contains a property with no PropType..."); continue; }
					if (!propertyNode.NodeProperty(2).Get(propLabel)) { warning("FBXData::Extract::PropertyExtractor::ExtractProperties - Properties70 node contains a property with no Label..."); continue; }
					if (!propertyNode.NodeProperty(3).Get(propFlags)) { warning("FBXData::Extract::PropertyExtractor::ExtractProperties - Properties70 node contains a property with no Flags..."); continue; }
					ParserMap::const_iterator it = m_parsers.find(propName);
					if (it != m_parsers.end())
						if (!it->second.Parse(target, propertyNode, logger)) return false;
				}
				return true;
			}
		};


		// Reads GlobalSettings from FBXContent::Node
		struct FBXGlobalSettings {
			enum class AxisIndex : uint8_t {
				X_INDEX = 0,
				Y_INDEX = 1,
				Z_INDEX = 2,
				ENUM_SIZE = 3
			} axisIndex[4] = { AxisIndex::Y_INDEX, AxisIndex::Z_INDEX, AxisIndex::X_INDEX, AxisIndex::Y_INDEX };
			float axisSign[4] = { 1.0f, -1.0f, 1.0f, 1.0f };
			float unitScale = 1.0f;
			float originalUnitScaleFactor = 1.0f;
		};
		inline static bool ReadGlobalSettings(FBXData::FBXGlobalSettings* result, const FBXContent::Node* globalSettingsNode, OS::Logger* logger) {
			if (globalSettingsNode == nullptr) return true;
			const FBXContent::Node* properties70Node = globalSettingsNode->FindChildNodeByName("Properties70");
			if (properties70Node == nullptr) {
				if (logger != nullptr) logger->Warning("FBXData::Extract::ReadGlobalSettings - 'Properties70' missing in 'GlobalSettings' node!");
				return true;
			}

			// Index to direction:
			static const Vector3 INDEX_TO_DIRECTION[] = {
				Math::Right(),
				Math::Up(),
				Math::Forward()
			};
			static const size_t INDEX_TO_DIRECTION_COUNT = (sizeof(INDEX_TO_DIRECTION) / sizeof(Vector3));

			// Indexes and AXIS_NAMES:
			static const char* AXIS_NAMES[] = {
				"UpAxis",
				"FrontAxis",
				"CoordAxis",
				"OriginalUpAxis"
			};
			static const size_t UP_INDEX = 0, FRONT_INDEX = 1, COORD_INDEX = 2, ORIGINAL_UP_INDEX = 3;

			static const auto getAxisSign = [](void* settings, size_t axisIndex, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
				int64_t sign;
				if (propertyNode.PropertyCount() < 5) return Error(logger, false, "FBXData::Extract::ReadGlobalSettings - ", PropertyParser::NameOf(propertyNode), " has no value!");
				else if (!propertyNode.NodeProperty(4).Get(sign)) 
					return Error(logger, false, "FBXData::Extract::ReadGlobalSettings - ", PropertyParser::NameOf(propertyNode), " is not an integer/bool!");
				reinterpret_cast<FBXGlobalSettings*>(settings)->axisSign[axisIndex] = (sign > 0) ? 1.0f : (-1.0f);
				return true;
			};

			static const PropertyExtractor PROPERTY_EXTRACTOR({
				PropertyParser(AXIS_NAMES[UP_INDEX], [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseEnumProperty(reinterpret_cast<FBXGlobalSettings*>(target)->axisIndex[UP_INDEX], propertyNode, logger);
					}),
				PropertyParser("UpAxisSign", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return getAxisSign(target, UP_INDEX, propertyNode, logger);
					}),
				PropertyParser(AXIS_NAMES[FRONT_INDEX], [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseEnumProperty(reinterpret_cast<FBXGlobalSettings*>(target)->axisIndex[FRONT_INDEX], propertyNode, logger);
					}),
				PropertyParser("FrontAxisSign", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return getAxisSign(target, FRONT_INDEX, propertyNode, logger);
					}),
				PropertyParser(AXIS_NAMES[COORD_INDEX], [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseEnumProperty(reinterpret_cast<FBXGlobalSettings*>(target)->axisIndex[COORD_INDEX], propertyNode, logger);
					}),
				PropertyParser("CoordAxisSign", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return getAxisSign(target, COORD_INDEX, propertyNode, logger);
					}),
				PropertyParser(AXIS_NAMES[ORIGINAL_UP_INDEX], [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseEnumProperty<FBXGlobalSettings::AxisIndex, PropertyParser::IgnoreIfNegative<int64_t>>(
							reinterpret_cast<FBXGlobalSettings*>(target)->axisIndex[ORIGINAL_UP_INDEX], propertyNode, logger);
					}),
				PropertyParser("OriginalUpAxisSign", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return getAxisSign(target, ORIGINAL_UP_INDEX, propertyNode, logger);
					}),
				PropertyParser("UnitScaleFactor", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FBXGlobalSettings*>(target)->unitScale, propertyNode, logger);
					}),
				PropertyParser("OriginalUnitScaleFactor", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FBXGlobalSettings*>(target)->originalUnitScaleFactor, propertyNode, logger);
					})
				});

			FBXGlobalSettings settings;
			if (!PROPERTY_EXTRACTOR.ExtractProperties(&settings, properties70Node, logger)) return false;

			for (size_t i = 0; i < INDEX_TO_DIRECTION_COUNT; i++)
				for (size_t j = 0; j < INDEX_TO_DIRECTION_COUNT; j++)
					if (i != j && settings.axisIndex[i] == settings.axisIndex[j])
						return Error(logger, false, "FBXData::Extract::ReadGlobalSettings - ", AXIS_NAMES[i], " and ", AXIS_NAMES[j], "Are the same!");

			settings.axisSign[FRONT_INDEX] *= -1.0f;
			auto axisValue = [&](size_t axis) ->Vector3 { return INDEX_TO_DIRECTION[static_cast<size_t>(settings.axisIndex[axis])] * settings.axisSign[axis]; };
			result->upAxis = axisValue(UP_INDEX);
			result->forwardAxis = axisValue(FRONT_INDEX);
			result->coordAxis = axisValue(COORD_INDEX);
			return true;
		}


		// Settings for 'FbxNode':
		// https://help.autodesk.com/view/FBX/2017/ENU/?guid=__cpp_ref_class_fbx_node_html
		struct FbxNodeSettings {
			Vector3 lclTranslation = Vector3(0.0f);
			Vector3 lclRotation = Vector3(0.0f);
			Vector3 lclScaling = Vector3(1.0f);
			
			float visibility = 1.0f;
			bool visibilityInheritance = true;
			
			enum class EFbxQuatInterpMode : uint8_t {
				eQuatInterpOff,
				eQuatInterpClassic,
				eQuatInterpSlerp,
				eQuatInterpCubic,
				eQuatInterpTangentDependent,
				eQuatInterpCount,
				ENUM_SIZE
			} quaternionInterpolate = EFbxQuatInterpMode::eQuatInterpOff;
			
			Vector3 rotationOffset = Vector3(0.0f);
			Vector3 rotationPivot = Vector3(0.0f);
			
			Vector3 scalingOffset = Vector3(0.0f);
			Vector3 scalingPivot = Vector3(0.0f);
			
			bool translationActive = false;
			Vector3 translationMin = Vector3(0.0f);
			Vector3 translationMax = Vector3(0.0f);
			bool translationMinX = false;
			bool translationMinY = false;
			bool translationMinZ = false;
			bool translationMaxX = false;
			bool translationMaxY = false;
			bool translationMaxZ = false;
			
			enum class FBXEulerOrder : uint8_t {
				eOrderXYZ,
				eOrderXZY,
				eOrderYZX,
				eOrderYXZ,
				eOrderZXY,
				eOrderZYX,
				eOrderSphericXYZ,
				ENUM_SIZE
			} rotationOrder = FBXEulerOrder::eOrderXYZ;
			
			bool rotationSpaceForLimitOnly = false;
			float rotationStiffnessX = 0.0f;
			float rotationStiffnessY = 0.0f;
			float rotationStiffnessZ = 0.0f;
			
			float axisLen = 10.0f;
			
			Vector3 preRotation = Vector3(0.0f);
			Vector3 postRotation = Vector3(0.0f);
			
			bool rotationActive = false;
			Vector3 rotationMin = Vector3(0.0f);
			Vector3 rotationMax = Vector3(0.0f);
			bool rotationMinX = false;
			bool rotationMinY = false;
			bool rotationMinZ = false;
			bool rotationMaxX = false;
			bool rotationMaxY = false;
			bool rotationMaxZ = false;
			
			enum class EInheritType : uint8_t {
				eInheritRrSs,
				eInheritRSrs,
				eInheritRrs,
				ENUM_SIZE
			} inheritType = EInheritType::eInheritRrSs;

			bool scalingActive = false;
			Vector3 scalingMin = Vector3(1.0f);
			Vector3 scalingMax = Vector3(1.0f);
			bool scalingMinX = false;
			bool scalingMinY = false;
			bool scalingMinZ = false;
			bool scalingMaxX = false;
			bool scalingMaxY = false;
			bool scalingMaxZ = false;

			Vector3 geometricTranslation = Vector3(0.0f);
			Vector3 geometricRotation = Vector3(0.0f);
			Vector3 geometricScaling = Vector3(1.0f);

			float minDampRangeX = 0.0f;
			float minDampRangeY = 0.0f;
			float minDampRangeZ = 0.0f;
			float maxDampRangeX = 0.0f;
			float maxDampRangeY = 0.0f;
			float maxDampRangeZ = 0.0f;

			float minDampStrengthX = 0.0f;
			float minDampStrengthY = 0.0f;
			float minDampStrengthZ = 0.0f;
			float maxDampStrengthX = 0.0f;
			float maxDampStrengthY = 0.0f;
			float maxDampStrengthZ = 0.0f;

			float preferedAngleX = 0.0f;
			float preferedAngleY = 0.0f;
			float preferedAngleZ = 0.0f;

			int64_t lookAtProperty = 0;
			int64_t upVectorProperty = 0;

			bool show = true;
			bool negativePercentShapeSupport = true;
			int64_t defaultAttributeIndex = -1;
			bool freeze = false;
			bool lodBox = false;

			inline bool Extract(const FBXContent::Node& parsedNode, OS::Logger* logger) {
				static const PropertyExtractor PROPERTY_EXTRACTOR({
					PropertyParser("Lcl Translation", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->lclTranslation, propertyNode, logger);
						}),
					PropertyParser("Lcl Rotation", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->lclRotation, propertyNode, logger);
						}),
					PropertyParser("Lcl Scaling", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->lclScaling, propertyNode, logger);
						}),
					PropertyParser("Visibility", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->visibility, propertyNode, logger);
						}),
					PropertyParser("Visibility Inheritance", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->visibilityInheritance, propertyNode, logger);
						}),
					PropertyParser("QuaternionInterpolate", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseEnumProperty(reinterpret_cast<FbxNodeSettings*>(target)->quaternionInterpolate, propertyNode, logger);
						}),
					PropertyParser("RotationOffset", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationOffset, propertyNode, logger);
						}),
					PropertyParser("RotationPivot", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationPivot, propertyNode, logger);
						}),
					PropertyParser("ScalingOffset", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingOffset, propertyNode, logger);
						}),
					PropertyParser("ScalingPivot", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingPivot, propertyNode, logger);
						}),
					PropertyParser("TranslationActive", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationActive, propertyNode, logger);
						}),
					PropertyParser("TranslationMin", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMin, propertyNode, logger);
						}),
					PropertyParser("TranslationMax", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMax, propertyNode, logger);
						}),
					PropertyParser("TranslationMinX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMinX, propertyNode, logger);
						}),
					PropertyParser("TranslationMinY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMinY, propertyNode, logger);
						}),
					PropertyParser("TranslationMinZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMinZ, propertyNode, logger);
						}),
					PropertyParser("TranslationMaxX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMaxX, propertyNode, logger);
						}),
					PropertyParser("TranslationMaxY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMaxY, propertyNode, logger);
						}),
					PropertyParser("TranslationMaxZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMaxZ, propertyNode, logger);
						}),
					PropertyParser("RotationOrder", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseEnumProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationOrder, propertyNode, logger);
						}),
					PropertyParser("RotationSpaceForLimitOnly", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationSpaceForLimitOnly, propertyNode, logger);
						}),
					PropertyParser("RotationStiffnessX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationStiffnessX, propertyNode, logger);
						}),
					PropertyParser("RotationStiffnessY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationStiffnessY, propertyNode, logger);
						}),
					PropertyParser("RotationStiffnessZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationStiffnessZ, propertyNode, logger);
						}),
					PropertyParser("AxisLen", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->axisLen, propertyNode, logger);
						}),
					PropertyParser("PreRotation", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->preRotation, propertyNode, logger);
						}),
					PropertyParser("PostRotation", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->postRotation, propertyNode, logger);
						}),
					PropertyParser("RotationActive", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationActive, propertyNode, logger);
						}),
					PropertyParser("RotationMin", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMin, propertyNode, logger);
						}),
					PropertyParser("RotationMax", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMax, propertyNode, logger);
						}),
					PropertyParser("RotationMinX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMinX, propertyNode, logger);
						}),
					PropertyParser("RotationMinY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMinY, propertyNode, logger);
						}),
					PropertyParser("RotationMinZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMinZ, propertyNode, logger);
						}),
					PropertyParser("RotationMaxX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMaxX, propertyNode, logger);
						}),
					PropertyParser("RotationMaxY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMaxY, propertyNode, logger);
						}),
					PropertyParser("RotationMaxZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMaxZ, propertyNode, logger);
						}),
					PropertyParser("InheritType", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseEnumProperty(reinterpret_cast<FbxNodeSettings*>(target)->inheritType, propertyNode, logger);
						}),
					PropertyParser("ScalingActive", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingActive, propertyNode, logger);
						}),
					PropertyParser("ScalingMin", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMin, propertyNode, logger);
						}),
					PropertyParser("ScalingMax", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMax, propertyNode, logger);
						}),
					PropertyParser("ScalingMinX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMinX, propertyNode, logger);
						}),
					PropertyParser("ScalingMinY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMinY, propertyNode, logger);
						}),
					PropertyParser("ScalingMinZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMinZ, propertyNode, logger);
						}),
					PropertyParser("ScalingMaxX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMaxX, propertyNode, logger);
						}),
					PropertyParser("ScalingMaxY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMaxY, propertyNode, logger);
						}),
					PropertyParser("ScalingMaxZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMaxZ, propertyNode, logger);
						}),
					PropertyParser("GeometricTranslation", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->geometricTranslation, propertyNode, logger);
						}),
					PropertyParser("GeometricRotation", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->geometricRotation, propertyNode, logger);
						}),
					PropertyParser("GeometricScaling", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->geometricScaling, propertyNode, logger);
						}),
					PropertyParser("MinDampRangeX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->minDampRangeX, propertyNode, logger);
						}),
					PropertyParser("MinDampRangeY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->minDampRangeY, propertyNode, logger);
						}),
					PropertyParser("MinDampRangeZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->minDampRangeZ, propertyNode, logger);
						}),
					PropertyParser("MaxDampRangeX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->maxDampRangeX, propertyNode, logger);
						}),
					PropertyParser("MaxDampRangeY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->maxDampRangeY, propertyNode, logger);
						}),
					PropertyParser("MaxDampRangeZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->maxDampRangeZ, propertyNode, logger);
						}),
					PropertyParser("MinDampStrengthX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->minDampStrengthX, propertyNode, logger);
						}),
					PropertyParser("MinDampStrengthY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->minDampStrengthY, propertyNode, logger);
						}),
					PropertyParser("MinDampStrengthZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->minDampStrengthZ, propertyNode, logger);
						}),
					PropertyParser("MaxDampStrengthX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->maxDampStrengthX, propertyNode, logger);
						}),
					PropertyParser("MaxDampStrengthY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->maxDampStrengthY, propertyNode, logger);
						}),
					PropertyParser("MaxDampStrengthZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->maxDampStrengthZ, propertyNode, logger);
						}),
					PropertyParser("PreferedAngleX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->preferedAngleX, propertyNode, logger);
						}),
					PropertyParser("PreferedAngleY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->preferedAngleY, propertyNode, logger);
						}),
					PropertyParser("PreferedAngleZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->preferedAngleZ, propertyNode, logger);
						}),
					PropertyParser("LookAtProperty", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->lookAtProperty, propertyNode, logger);
						}),
					PropertyParser("UpVectorProperty", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->upVectorProperty, propertyNode, logger);
						}),
					PropertyParser("Show", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->show, propertyNode, logger);
						}),
					PropertyParser("NegativePercentShapeSupport", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->negativePercentShapeSupport, propertyNode, logger);
						}),
					PropertyParser("DefaultAttributeIndex", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->defaultAttributeIndex, propertyNode, logger);
						}),
					PropertyParser("Freeze", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->freeze, propertyNode, logger);
						}),
					PropertyParser("LODBox", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return PropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->lodBox, propertyNode, logger);
						})
					});

				const FBXContent::Node* properties70Node = parsedNode.FindChildNodeByName("Properties70");
				if (properties70Node == nullptr) {
					if (logger != nullptr) logger->Warning("FBXData::Extract::FbxNodeSettings::Extract - 'Properties70' missing in 'FbxNode' node!");
					return true;
				}
				if (!PROPERTY_EXTRACTOR.ExtractProperties(reinterpret_cast<void*>(this), properties70Node, logger)) return false;

				// __TODO__: Maybe? make sure nothing is being violated
				return true;
			}
		};


		// Collection of important templates:
		struct FBXTemplates {
			FbxNodeSettings nodeSettings;

			inline bool Extract(const FBXContent::Node* templatesNode, OS::Logger* logger) {
				typedef bool(*TemplateParseFn)(FBXTemplates*, const FBXContent::Node*, OS::Logger*);
				typedef std::unordered_map<std::string_view, std::pair<std::string_view, TemplateParseFn>> ParserCollections;
				static const ParserCollections PARSERS = {
					std::make_pair(std::string_view("Model"), std::make_pair(std::string_view("FbxNode"), [](FBXTemplates* templates, const FBXContent::Node* templatesNode, OS::Logger* logger) -> bool {
						return templates->nodeSettings.Extract(*templatesNode, logger);
						}))
				};
				if (templatesNode == nullptr) return true;
				else for (size_t i = 0; i < templatesNode->NestedNodeCount(); i++) {
					const FBXContent::Node& objectTypeNode = templatesNode->NestedNode(i);
					if (objectTypeNode.Name() != "ObjectType") continue;
					else if (objectTypeNode.PropertyCount() <= 0) {
						if (logger != nullptr) logger->Warning("FBXData::Extract::FBXTemplates::Extract - ObjectType has no value...");
						continue;
					}
					std::string_view objectTypeName;
					if (!objectTypeNode.NodeProperty(0).Get(objectTypeName))
						return Error(logger, false, "FBXData::Extract::FBXTemplates::Extract - ObjectType property was expected to be a string!");
					ParserCollections::const_iterator it = PARSERS.find(objectTypeName);
					if (it == PARSERS.end()) continue;
					const FBXContent::Node* propertyTemplateNode = objectTypeNode.FindChildNodeByName("PropertyTemplate");
					if (propertyTemplateNode == nullptr) {
						if (logger != nullptr) logger->Warning("FBXData::Extract::FBXTemplates::Extract - PropertyTemplate not found within ObjectType node for '", objectTypeName, "'...");
						continue;
					}
					else if (propertyTemplateNode->PropertyCount() <= 0) {
						if (logger != nullptr) logger->Warning("FBXData::Extract::FBXTemplates::Extract - PropertyTemplate has no value...");
						continue;
					}
					std::string_view propertyTemplateClassName;
					if (!propertyTemplateNode->NodeProperty(0).Get(propertyTemplateClassName))
						return Error(logger, false, "FBXData::Extract::FBXTemplates::Extract - PropertyTemplate property was expected to be a string!");
					if (propertyTemplateClassName != it->second.first) {
						if (logger != nullptr)
							logger->Warning(
								"FBXData::Extract::FBXTemplates::Extract - PropertyTemplate class name for '", objectTypeName,
								"' was expected to be '", it->second.first, "'; encountered: '", propertyTemplateClassName, "'...");
						continue;
					}
					if (!it->second.second(this, propertyTemplateNode, logger))
						return Error(logger, false, "FBXData::Extract::FBXTemplates::Extract - Failed to read PropertyTemplate for '", objectTypeName, "'!");
				}
				return true;
			}
		};


		// Reads mesh from FBXContent::Node
		class FBXMeshExtractor {
		private:
			struct VNUIndex {
				uint32_t vertexId = 0;
				uint32_t normalId = 0;
				uint32_t uvId = 0;

				inline bool operator<(const VNUIndex& other)const {
					return vertexId < other.vertexId || (vertexId == other.vertexId && (normalId < other.normalId || (normalId == other.normalId && uvId < other.uvId)));
				}
			};

			struct Index : public VNUIndex {
				uint32_t smoothId = 0;
				uint32_t nextIndexOnPoly = 0;

				inline static constexpr size_t NormalIdOffset() { Index index; return ((size_t)(&(index.normalId))) - ((size_t)(&index)); }
				inline static constexpr size_t SmoothIdOffset() { Index index; return ((size_t)(&(index.smoothId))) - ((size_t)(&index)); }
				inline static constexpr size_t UvIdOffset() { Index index; return ((size_t)(&(index.uvId))) - ((size_t)(&index)); }
			};

			std::vector<Vector3> m_nodeVertices;
			std::vector<Vector3> m_normals;
			std::vector<Vector2> m_uvs;
			std::vector<bool> m_smooth;
			std::vector<size_t> m_faceEnds;
			std::vector<Index> m_indices;
			std::vector<Stacktor<uint32_t, 4>> m_nodeEdges;
			std::vector<uint32_t> m_layerIndexBuffer;

			inline void Clear() {
				m_nodeVertices.clear();
				m_normals.clear();
				m_uvs.clear();
				m_smooth.clear();
				m_indices.clear();
				m_faceEnds.clear();
				m_nodeEdges.clear();
			}

			inline bool ExtractVertices(const FBXContent::Node* objectNode, OS::Logger* logger) {
				const FBXContent::Node* verticesNode = objectNode->FindChildNodeByName("Vertices", 0);
				if (verticesNode == nullptr) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractVertices - Vertices node missing!");
				else if (verticesNode->PropertyCount() >= 1)
					if (!verticesNode->NodeProperty(0).Fill(m_nodeVertices, true))
						return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractVertices - Vertices node invalid!");
				return true;
			}

			inline bool ExtractFaces(const FBXContent::Node* objectNode, OS::Logger* logger) {
				const FBXContent::Node* indicesNode = objectNode->FindChildNodeByName("PolygonVertexIndex", 0);
				if (indicesNode == nullptr) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractFaces - Indices node missing!");
				else if (indicesNode->PropertyCount() >= 1) {
					const FBXContent::Property& prop = indicesNode->NodeProperty(0);
					const size_t vertexCount = m_nodeVertices.size();
					auto addNodeIndex = [&](int32_t value) -> bool {
						auto pushIndex = [&](uint32_t index) -> bool {
							if (index >= vertexCount) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractFaces - Vertex index overflow!");
							Index id;
							id.vertexId = index;
							id.nextIndexOnPoly = static_cast<uint32_t>(m_indices.size()) + 1u;
							m_indices.push_back(id);
							return true;
						};
						if (value < 0) {
							if (!pushIndex(static_cast<uint32_t>(value ^ int32_t(-1)))) return false;
							m_indices.back().nextIndexOnPoly = static_cast<uint32_t>((m_faceEnds.size() <= 0) ? 0 : m_faceEnds.back());
							m_faceEnds.push_back(m_indices.size());
							return true;
						}
						else return pushIndex(static_cast<uint32_t>(value));
					};
					if (prop.Type() == FBXContent::PropertyType::INT_32_ARR) {
						for (size_t i = 0; i < prop.Count(); i++)
							if (!addNodeIndex(prop.Int32Elem(i))) return false;
					}
					else if (prop.Type() == FBXContent::PropertyType::INT_64_ARR) {
						for (size_t i = 0; i < prop.Count(); i++) {
							int64_t value = prop.Int64Elem(i);
							int32_t truncatedValue = static_cast<int32_t>(value);
							if (value != truncatedValue) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractFaces - Face index can not fit into an int32_t value!");
							else if (!addNodeIndex(truncatedValue)) return false;
						}
					}
					else return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractFaces - Indices does not have an integer array property where expected!");
					if ((m_indices.size() > 0) && ((m_faceEnds.size() <= 0) || (m_faceEnds.back() != m_indices.size()))) {
						if (logger != nullptr) logger->Warning("FBXData::FBXMeshExtractor::ExtractFaces - Last index not negative... Pretending as if it was xor-ed with -1...");
						m_indices.back().nextIndexOnPoly = static_cast<uint32_t>((m_faceEnds.size() <= 0) ? 0 : m_faceEnds.back());
						m_faceEnds.push_back(m_indices.size());
					}
				}
				return true;
			}

			inline bool ExtractEdges(const FBXContent::Node* objectNode, OS::Logger* logger) {
				const FBXContent::Node* verticesNode = objectNode->FindChildNodeByName("Edges", 0);
				if (verticesNode == nullptr) return true;
				m_layerIndexBuffer.clear();
				if (verticesNode->PropertyCount() >= 1)
					if (!verticesNode->NodeProperty(0).Fill(m_layerIndexBuffer, true))
						return Error(logger, false, "BXData::FBXMeshExtractor::ExtractEdges - Edges buffer invalid!");
				if (m_indices.size() > 1) {
					for (size_t i = 0; i < m_layerIndexBuffer.size(); i++) {
						if (m_layerIndexBuffer[i] >= m_indices.size())
							return Error(logger, false, "BXData::FBXMeshExtractor::ExtractEdges - Edge value exceeds maximal valid edge index!");
					}
					auto getEdgeKey = [&](uint32_t polyVertId) -> uint64_t {
						const Index& index = m_indices[polyVertId];
						uint64_t vertexA = static_cast<uint64_t>(index.vertexId);
						uint64_t vertexB = static_cast<uint64_t>(m_indices[index.nextIndexOnPoly].vertexId);
						if (vertexA > vertexB) std::swap(vertexA, vertexB);
						return ((vertexB << 32) | vertexA);
					};
					m_nodeEdges.resize(m_layerIndexBuffer.size());
					std::unordered_map<uint64_t, size_t> edgeIndex;
					for (size_t i = 0; i < m_layerIndexBuffer.size(); i++)
						edgeIndex[getEdgeKey(m_layerIndexBuffer[i])] = i;
					bool edgeSetIncomplete = false;
					for (uint32_t i = 0; i < m_indices.size(); i++) {
						std::unordered_map<uint64_t, size_t>::const_iterator it = edgeIndex.find(getEdgeKey(i));
						if (it == edgeIndex.end()) {
							edgeSetIncomplete = true;
							continue;
						}
						Stacktor<uint32_t, 4>& edgeIndices = m_nodeEdges[it->second];
						edgeIndices.Push(i);
						edgeIndices.Push(m_indices[i].nextIndexOnPoly);
					}
					if (edgeSetIncomplete && logger != nullptr)
						logger->Warning("BXData::FBXMeshExtractor::ExtractEdges - Edge set incomplete!");
				}
				else if (m_layerIndexBuffer.size() > 0)
					return Error(logger, false, "BXData::FBXMeshExtractor::ExtractEdges - We have less than 2 indices and therefore, can not have any edges!");
				return true;
			}

			template<size_t IndexOffset>
			inline bool ExtractLayerIndexInformation(
				const FBXContent::Node* layerElement, size_t layerElemCount, 
				const std::string_view& layerElementName, const std::string_view& indexSubElementName, OS::Logger* logger) {
				auto findInformationType = [&](const std::string_view& name) -> const FBXContent::Property* {
					const FBXContent::Node* informationTypeNode = layerElement->FindChildNodeByName(name);
					if (informationTypeNode == nullptr)
						return Error(logger, nullptr, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ", name, " node missing for ", layerElementName, "!");
					else if (informationTypeNode->PropertyCount() <= 0)
						return Error(logger, nullptr, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ", name, " node has no value for ", layerElementName, "!");
					const FBXContent::Property* informationTypeProp = &informationTypeNode->NodeProperty(0);
					if (informationTypeProp->Type() != FBXContent::PropertyType::STRING)
						return Error(logger, nullptr, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ", name, " not string for ", layerElementName, "!");
					return informationTypeProp;
				};

				// ReferenceInformationType:
				const FBXContent::Property* referenceInformationTypeProp = findInformationType("ReferenceInformationType");
				if (referenceInformationTypeProp == nullptr) return false;
				const std::string_view referenceInformationType = *referenceInformationTypeProp;
				
				// Index:
				m_layerIndexBuffer.clear();
				if (referenceInformationType == "Direct") {
					for (uint32_t i = 0; i < layerElemCount; i++) m_layerIndexBuffer.push_back(i);
				}
				else if (referenceInformationType == "IndexToDirect" || referenceInformationType == "Index") {
					const FBXContent::Node* indexNode = layerElement->FindChildNodeByName(indexSubElementName);
					if (indexNode == nullptr) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ", indexSubElementName, " node missing!");
					else if (indexNode->PropertyCount() <= 0) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ", indexSubElementName, " has no values!");
					else if (!indexNode->NodeProperty(0).Fill(m_layerIndexBuffer, true)) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ", indexSubElementName, " node invalid!");
					else for (size_t i = 0; i < m_layerIndexBuffer.size(); i++)
						if (m_layerIndexBuffer[i] >= layerElemCount)
							return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ",
								indexSubElementName, " contains indices greater than or equal to ", indexSubElementName, ".size()<", layerElemCount, ">!");
				}
				else return Error(logger, false,
					"FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ReferenceInformationType<", referenceInformationType, "> not supported for ", layerElementName, "!");

				// MappingInformationType:
				const FBXContent::Property* mappingInformationTypeProp = findInformationType("MappingInformationType");
				if (mappingInformationTypeProp == nullptr) return false;
				const std::string_view mappingInformationType = *mappingInformationTypeProp;

				// Fill m_indices:
				const size_t indexCount = m_layerIndexBuffer.size();
				auto indexValue = [&](size_t index) -> uint32_t& {
					return (*reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(m_indices.data() + index) + IndexOffset));
				};
				if (mappingInformationType == "ByVertex" || mappingInformationType == "ByVertice") {
					if (indexCount != m_nodeVertices.size()) 
						return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - Index count for ", layerElementName, " does not match the vertex count!");
					for (size_t i = 0; i < m_indices.size(); i++)
						indexValue(i) = m_layerIndexBuffer[m_indices[i].vertexId];
				}
				else if (mappingInformationType == "ByPolygonVertex") {
					if (indexCount != m_indices.size())
						return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - Index count for ", layerElementName, " does not match the polygon vertex count!");
					for (size_t i = 0; i < m_indices.size(); i++)
						indexValue(i) = m_layerIndexBuffer[i];
				}
				else if (mappingInformationType == "ByPolygon") {
					if (indexCount != m_faceEnds.size())
						return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - Index count for ", layerElementName, " does not match the polygon count!");
					size_t faceId = 0;
					for (size_t i = 0; i < m_indices.size(); i++) {
						indexValue(i) = m_layerIndexBuffer[faceId];
						if (i == m_faceEnds[faceId]) faceId++;
					}
				}
				else if (mappingInformationType == "ByEdge") {
					if (indexCount != m_nodeEdges.size())
						return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - Index count for ", layerElementName, " does not match the edge count!");
					for (size_t i = 0; i < m_indices.size(); i++)
						indexValue(i) = static_cast<uint32_t>(layerElemCount);
					for (size_t i = 0; i < m_nodeEdges.size(); i++) {
						const Stacktor<uint32_t, 4>& edgeIndices = m_nodeEdges[i];
						uint32_t value = m_layerIndexBuffer[i];
						for (size_t j = 0; j < edgeIndices.Size(); j++)
							indexValue(edgeIndices[j]) = value;
					}
					for (size_t i = 0; i < m_indices.size(); i++)
						if (indexValue(i) >= layerElemCount)
							return Error(logger, false,
								"FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - Edges do not cover all indices and can not be used for layer elements for ", layerElementName, "!");
					if (logger != nullptr)
						logger->Warning("FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - ", layerElementName, " layer was set 'ByEdge'; not sure if the interpretation is correct...");
				}
				else if (mappingInformationType == "AllSame") {
					if (layerElemCount <= 0)
						return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - Index count for ", layerElementName, " is zero!");
					for (size_t i = 0; i < m_indices.size(); i++)
						indexValue(i) = 0;
				}
				else return Error(logger, false,
					"FBXData::FBXMeshExtractor::ExtractLayerIndexInformation - MappingInformationType<", mappingInformationType, "> not supported for ", layerElementName, "!");
				return true;
			}

			// One of the compilers will hate me if I don't do this...
#ifdef _WIN32
#define NORMAL_ID_OFFSET offsetof(Index, normalId)
#define SMOOTH_ID_OFFSET offsetof(Index, smoothId)
#define UV_ID_OFFSET offsetof(Index, uvId)
#else
#define NORMAL_ID_OFFSET Index::NormalIdOffset()
#define SMOOTH_ID_OFFSET Index::SmoothIdOffset()
#define UV_ID_OFFSET Index::UvIdOffset()
#endif

			inline bool ExtractNormals(const FBXContent::Node* layerElement, OS::Logger* logger) {
				if (layerElement == nullptr) return true;
				const FBXContent::Node* normalsNode = layerElement->FindChildNodeByName("Normals");
				if (normalsNode == nullptr) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractNormals - Normals node missing!");
				else if (normalsNode->PropertyCount() >= 1)
					if (!normalsNode->NodeProperty(0).Fill(m_normals, true))
						return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractNormals - Normals node invalid!");
				return ExtractLayerIndexInformation<NORMAL_ID_OFFSET>(layerElement, m_normals.size(), "Normals", "NormalsIndex", logger);
			}

			inline bool ExtractSmoothing(const FBXContent::Node* layerElement, OS::Logger* logger) {
				if (layerElement == nullptr) return true;
				const FBXContent::Node* smoothingNode = layerElement->FindChildNodeByName("Smoothing");
				if (smoothingNode == nullptr) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractSmoothing - Smoothing node missing!");
				else if (smoothingNode->PropertyCount() >= 1)
					if (!smoothingNode->NodeProperty(0).Fill(m_smooth, true))
						return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractSmoothing - Smoothing node invalid!");
				return ExtractLayerIndexInformation<SMOOTH_ID_OFFSET>(layerElement, m_smooth.size(), "Smoothing", "SmoothingIndex", logger);
			}

			inline bool ExtractUVs(const FBXContent::Node* layerElement, OS::Logger* logger) {
				if (layerElement == nullptr) {
					m_uvs = { Vector2(0.0f) };
					return true;
				}
				const FBXContent::Node* uvNode = layerElement->FindChildNodeByName("UV");
				if (uvNode == nullptr) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractUVs - UV node missing!");
				else if (uvNode->PropertyCount() >= 1)
					if (!uvNode->NodeProperty(0).Fill(m_uvs, true))
						return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractUVs - UV node invalid!");
				if (!ExtractLayerIndexInformation<UV_ID_OFFSET>(layerElement, m_uvs.size(), "UV", "UVIndex", logger)) return false;
				else if (m_uvs.size() <= 0) m_uvs.push_back(Vector2(0.0f));
				return true;
			}

#undef NORMAL_ID_OFFSET
#undef SMOOTH_ID_OFFSET
#undef UV_ID_OFFSET

			inline bool FixNormals(OS::Logger* logger) {
				// Make sure there are normals for each face vertex:
				bool noNormals = (m_normals.size() <= 0);
				if (noNormals && m_faceEnds.size() > 0 && m_faceEnds[0] > 0) {
					size_t faceId = 0;
					size_t prev = m_faceEnds[faceId] - 1;
					for (size_t i = 0; i < m_indices.size(); i++) {
						if (i >= m_faceEnds[faceId]) {
							faceId++;
							prev = m_faceEnds[faceId] - 1;
						}
						Index& index = m_indices[i];
						const Vector3 o = m_nodeVertices[index.vertexId];
						const Vector3 a = m_nodeVertices[m_indices[index.nextIndexOnPoly].vertexId];
						const Vector3 b = m_nodeVertices[m_indices[prev].vertexId];
						const Vector3 cross = Math::Cross(a - o, b - o);
						const float crossSqrMagn = Math::Dot(cross, cross);
						index.normalId = static_cast<uint32_t>(m_normals.size());
						m_normals.push_back(crossSqrMagn <= 0.0f ? Vector3(0.0f) : (cross / std::sqrt(crossSqrMagn)));
						prev = i;
					}
				}

				// Make sure each face vertex has a smoothing value:
				bool hasSmoothing = false;
				if (m_smooth.size() <= 0) {
					m_smooth.push_back(noNormals);
					for (size_t i = 0; i < m_indices.size(); i++)
						m_indices[i].smoothId = 0;
					hasSmoothing = noNormals;
				}
				else for (size_t i = 0; i < m_indices.size(); i++)
					if (m_smooth[m_indices[i].smoothId]) {
						hasSmoothing = true;
						break;
					}

				// If there are smooth vertices, their normals should be averaged and merged:
				if (!hasSmoothing) return true;
				size_t desiredNormalCount = m_normals.size() + m_nodeVertices.size();
				if (desiredNormalCount > UINT32_MAX) return Error(logger, false, "FBXData::FBXMeshExtractor::ExtractSmoothing - Too many normals!");
				const uint32_t baseSmoothNormal = static_cast<uint32_t>(m_normals.size());
				for (size_t i = 0; i < m_nodeVertices.size(); i++)
					m_normals.push_back(Vector3(0.0f));
				for (size_t i = 0; i < m_indices.size(); i++) {
					Index& index = m_indices[i];
					if (!m_smooth[index.smoothId]) continue;
					uint32_t vertexId = index.vertexId;
					uint32_t normalIndex = baseSmoothNormal + vertexId;
					m_normals[normalIndex] += m_normals[index.normalId];
					index.normalId = normalIndex;
				}
				for (size_t i = 0; i < m_nodeVertices.size(); i++) {
					Vector3& normal = m_normals[static_cast<size_t>(baseSmoothNormal) + m_indices[i].vertexId];
					const float sqrMagn = Math::Dot(normal, normal);
					if (sqrMagn > 0.0f) normal /= std::sqrt(sqrMagn);
				}
				return true;
			}

			inline Reference<PolyMesh> CreateMesh(const std::string_view& name) {
				std::map<VNUIndex, uint32_t> vertexIndexMap;
				Reference<PolyMesh> mesh = Object::Instantiate<PolyMesh>(name);
				PolyMesh::Writer writer(mesh);
				for (size_t faceId = 0; faceId < m_faceEnds.size(); faceId++) {
					const size_t faceEnd = m_faceEnds[faceId];
					if (faceEnd <= 0 || (faceId > 0 && m_faceEnds[faceId] <= m_faceEnds[faceId - 1])) continue;
					writer.Faces().push_back(PolygonFace());
					PolygonFace& face = writer.Faces().back();
					for (size_t i = m_indices[faceEnd - 1].nextIndexOnPoly; i < faceEnd; i++) {
						const VNUIndex& index = m_indices[i];
						uint32_t vertexIndex;
						{
							std::map<VNUIndex, uint32_t>::const_iterator it = vertexIndexMap.find(index);
							if (it == vertexIndexMap.end()) {
								vertexIndex = static_cast<uint32_t>(writer.Verts().size());
								vertexIndexMap[index] = vertexIndex;
								const Vector3 vertex = m_nodeVertices[index.vertexId];
								const Vector3 normal = m_normals[index.normalId];
								const Vector2 uv = m_uvs[index.uvId];
								writer.Verts().push_back(MeshVertex(Vector3(vertex.x, vertex.y, -vertex.z), Vector3(normal.x, normal.y, -normal.z), Vector2(uv.x, 1.0f - uv.y)));
							}
							else vertexIndex = it->second;
						}
						face.Push(vertexIndex);
					}
				}
				return mesh;
			}

		public:
			inline Reference<PolyMesh> ExtractData(const FBXContent::Node* objectNode, const std::string_view& name, OS::Logger* logger) {
				auto error = [&](auto... message) ->Reference<PolyMesh> { return Error<Reference<PolyMesh>>(logger, nullptr, message...); };
				Clear();
				if (!ExtractVertices(objectNode, logger)) return nullptr;
				else if (!ExtractFaces(objectNode, logger)) return nullptr;
				else if (!ExtractEdges(objectNode, logger)) return nullptr;
				std::pair<const FBXContent::Node*, int64_t> normalLayerElement = std::pair<const FBXContent::Node*, int64_t>(nullptr, 0);
				std::pair<const FBXContent::Node*, int64_t> smoothingLayerElement = std::pair<const FBXContent::Node*, int64_t>(nullptr, 0);
				std::pair<const FBXContent::Node*, int64_t> uvLayerElement = std::pair<const FBXContent::Node*, int64_t>(nullptr, 0);
				for (size_t i = 0; i < objectNode->NestedNodeCount(); i++) {
					const FBXContent::Node* layerNode = &objectNode->NestedNode(i);
					const std::string_view elementName = layerNode->Name();
					std::pair<const FBXContent::Node*, int64_t>* layerElementToReplace = (
						(elementName == "LayerElementNormal") ? (&normalLayerElement) :
						(elementName == "LayerElementSmoothing") ? (&normalLayerElement) :
						(elementName == "LayerElementUV") ? (&uvLayerElement) : nullptr);
					if (layerElementToReplace == nullptr) continue;
					else if (layerNode->PropertyCount() <= 0) return error("FBXData::FBXMeshExtractor::ExtractData - Layer element does not have layer id!");
					const FBXContent::Property& layerIdProp = layerNode->NodeProperty(0);
					int64_t layerIndex;
					if (layerIdProp.Type() == FBXContent::PropertyType::INT_64) layerIndex = static_cast<int64_t>(layerIdProp.operator int64_t());
					else if (layerIdProp.Type() == FBXContent::PropertyType::INT_32) layerIndex = static_cast<int64_t>(layerIdProp.operator int32_t());
					else if (layerIdProp.Type() == FBXContent::PropertyType::INT_16) layerIndex = static_cast<int64_t>(layerIdProp.operator int16_t());
					else return error("FBXData::FBXMeshExtractor::ExtractData - Layer element does not have an integer layer id!");
					bool hasAnotherLayer = (layerElementToReplace->first != nullptr);
					if ((!hasAnotherLayer) || layerElementToReplace->second > layerIndex)
						(*layerElementToReplace) = std::make_pair(layerNode, layerIndex);
					else hasAnotherLayer = true;
					if (hasAnotherLayer && logger != nullptr) 
						logger->Warning("FBXData::FBXMeshExtractor::ExtractData - Multiple layer elements<", elementName, "> not [currently] supported...");
				}
				if (!ExtractNormals(normalLayerElement.first, logger)) return nullptr;
				else if (!ExtractSmoothing(smoothingLayerElement.first, logger)) return nullptr;
				else if (!ExtractUVs(uvLayerElement.first, logger)) return nullptr;
				else if (!FixNormals(logger)) return nullptr;
				else return CreateMesh(name);
			}
		};
	}


	Reference<FBXData> FBXData::Extract(const FBXContent* sourceContent, OS::Logger* logger) {
		auto error = [&](auto... message) { return Error<FBXData*>(logger, nullptr, message...); };
		auto warning = [&](auto... message) {  if (logger != nullptr) logger->Warning(message...); };

		if (sourceContent == nullptr) return error("FBXData::Extract - NULL sourceContent provided!");

		// Root level nodes:
		const FBXContent::Node* fbxHeaderExtensionNode = sourceContent->RootNode().FindChildNodeByName("FBXHeaderExtension", 0);
		if (fbxHeaderExtensionNode == nullptr) warning("FBXData::Extract - FBXHeaderExtension missing!");
		const FBXContent::Node* fileIdNode = sourceContent->RootNode().FindChildNodeByName("FileId", 1);
		if (fileIdNode == nullptr) warning("FBXData::Extract - FileId missing!");
		const FBXContent::Node* creationTimeNode = sourceContent->RootNode().FindChildNodeByName("CreationTime", 2);
		if (creationTimeNode == nullptr) warning("FBXData::Extract - CreationTime missing!");
		const FBXContent::Node* creatorNode = sourceContent->RootNode().FindChildNodeByName("Creator", 3);
		if (creatorNode == nullptr) warning("FBXData::Extract - Creator missing!");
		const FBXContent::Node* globalSettingsNode = sourceContent->RootNode().FindChildNodeByName("GlobalSettings", 4);
		if (globalSettingsNode == nullptr) warning("FBXData::Extract - GlobalSettings missing!");
		const FBXContent::Node* documentsNode = sourceContent->RootNode().FindChildNodeByName("Documents", 5);
		const FBXContent::Node* referencesNode = sourceContent->RootNode().FindChildNodeByName("References", 6);
		const FBXContent::Node* definitionsNode = sourceContent->RootNode().FindChildNodeByName("Definitions", 7);
		const FBXContent::Node* objectsNode = sourceContent->RootNode().FindChildNodeByName("Objects", 8);
		const FBXContent::Node* connectionsNode = sourceContent->RootNode().FindChildNodeByName("Connections", 9);
		const FBXContent::Node* takesNode = sourceContent->RootNode().FindChildNodeByName("Takes", 10);

		// Notes:
		// 0. We ignore the contents of FBXHeaderExtension for performance and also, we don't really care for them; invalid FBX files may pass here;
		// 1. We also don't care about FileId, CreationTime Creator, Documents, References and Takes for similar reasons;
		// 2. I've followed this link to make sense of the content: 
		//    https://web.archive.org/web/20160605023014/https://wiki.blender.org/index.php/User:Mont29/Foundation/FBX_File_Structure#Spaces_.26_Parenting
		Reference<FBXData> result = Object::Instantiate<FBXData>();

		// Parse GlobalSettings:
		if (!ReadGlobalSettings(&result->m_globalSettings, globalSettingsNode, logger)) return nullptr;
		const Matrix4 axisWrangle = Math::Transpose(Matrix4(
			Vector4(result->m_globalSettings.coordAxis, 0.0f),
			Vector4(result->m_globalSettings.upAxis, 0.0f),
			Vector4(-result->m_globalSettings.forwardAxis, 0.0f),
			Vector4(0.0f, 0.0f, 0.0f, 1.0f)));
		static_assert(Math::Dot(Math::Cross(Math::Right(), Math::Up()), Math::Forward()) > 0.0f);
		static bool isLeftHanded = Math::Dot(Math::Cross(result->m_globalSettings.coordAxis, result->m_globalSettings.upAxis), result->m_globalSettings.forwardAxis) > 0.0f;
		if (isLeftHanded) return error("FBXData::Extract - FBX files are expected to have right handed coordinate systems!");

		// Parse Definitions:
		FBXTemplates templates;
		if (!templates.Extract(definitionsNode, logger)) return nullptr;

		// Connections:
		std::unordered_map<int64_t, std::vector<int64_t>> parentNodes;
		if (connectionsNode != nullptr)
			for (size_t i = 0; i < connectionsNode->NestedNodeCount(); i++) {
				const FBXContent::Node& connectionNode = connectionsNode->NestedNode(i);
				if (connectionNode.Name() != "C") continue;
				else if (connectionNode.PropertyCount() < 3) {
					warning("FBXData::Extract - Connection node incomplete!");
					continue;
				}
				std::string_view connectionType;
				if (!connectionNode.NodeProperty(0).Get(connectionType)) return error("FBXData::Extract - Connection node does not have a valid connection type string!");
				int64_t childUid, parentUid;
				if (!connectionNode.NodeProperty(1).Get(childUid)) return error("FBXData::Extract - Connection node child UID invalid!");
				if (!connectionNode.NodeProperty(2).Get(parentUid)) return error("FBXData::Extract - Connection node parent UID invalid!");
				if (parentUid != 0) parentNodes[childUid].push_back(parentUid);
			}

		// Parse Objects:
		if (objectsNode != nullptr) {
			FBXMeshExtractor meshExtractor;
			std::unordered_map<int64_t, size_t> transformIndex;
			std::vector<Reference<FBXNode>> transforms;

			for (size_t i = 0; i < objectsNode->NestedNodeCount(); i++) {
				const FBXContent::Node* objectNode = &objectsNode->NestedNode(i);

				// NodeAttribute:
				const std::string_view& nodeAttribute = objectNode->Name();

				// Property count:
				if (objectNode->PropertyCount() < 3) {
					warning("FBXData::Extract - Object[", i, "] has less than 3 properties. Expected [UID, Name::Class, Sub-Class]; Object entry will be ignored...");
					continue;
				}

				// UID:
				const FBXContent::Property& objectUidProperty = objectNode->NodeProperty(0);
				int64_t objectUid;
				if (!objectNode->NodeProperty(0).Get(objectUid)) {
					warning("FBXData::Extract - Object[", i, "].NodeProperty[0]<UID> is not an integer type; Object entry will be ignored...");
					continue;
				}

				// Name::Class
				std::string_view objectNameClass;
				if (!objectNode->NodeProperty(1).Get(objectNameClass)) {
					warning("FBXData::Extract - Object[", i, "].NodeProperty[1]<Name::Class> is not a string; Object entry will be ignored...");
					continue;
				}
				const std::string_view objectName = objectNameClass.data();
				if (objectNameClass.size() != (nodeAttribute.size() + objectName.size() + 2u)) {
					// __TODO__: Warning needed, but this check has to change a bit for animation curves...
					//warning("FBXData::Extract - Object[", i, "].NodeProperty[1]<Name::Class> not formatted correctly(name=", nodeAttribute, "); Object entry will be ignored...");
					continue;
				}
				else if (objectNameClass.data()[objectName.size()] != 0x00 || objectNameClass.data()[objectName.size() + 1] != 0x01) {
					warning("FBXData::Extract - Object[", i, "].NodeProperty[1]<Name::Class> Expected '0x00,0x01' between Name and Class, got something else; Object entry will be ignored...");
					continue;
				}
				else if (std::string_view(objectName.data() + objectName.size() + 2) != nodeAttribute) {
					warning("FBXData::Extract - Object[", i, "].NodeProperty[1]<Name::Class> 'Name::' not followed by nodeAttribute<", nodeAttribute, ">; Object entry will be ignored...");
					continue;
				}

				// Sub-class:
				std::string_view objectSubClass;
				if (!objectNode->NodeProperty(2).Get(objectSubClass)) {
					warning("FBXData::Extract - Object[", i, "].NodeProperty[2]<Sub-class> is not a string; Object entry will be ignored...");
					continue;
				}


				// Fallback for reading types that are not yet implemented:
				auto readNotImplemented = [&]() -> bool {
					warning("FBXData::Extract - Object[", i, "].Name() = '", nodeAttribute, "'; [__TODO__]: Parser not yet implemented! Object entry will be ignored...");
					return true;
				};

				// Reads a Model:
				auto readModel = [&]() -> bool {
					FbxNodeSettings nodeSettings = templates.nodeSettings;
					if (!nodeSettings.Extract(*objectNode, logger)) return false;
					// __TODO__: Implement this crap properly!
					const Reference<FBXNode> node = Object::Instantiate<FBXNode>();
					node->objectId = objectUid;
					node->name = objectName;

					const bool noParent = (parentNodes.find(objectUid) == parentNodes.end());
					const float ROOT_POSE_SCALE = 0.01f;

					node->position = Vector3(nodeSettings.lclTranslation.x, nodeSettings.lclTranslation.y, -nodeSettings.lclTranslation.z);
					if (noParent) node->position = axisWrangle * Vector4(node->position * ROOT_POSE_SCALE, 0.0f);
					
					const float
						eulerX = Math::Radians(-nodeSettings.lclRotation.x),
						eulerY = Math::Radians(-nodeSettings.lclRotation.y),
						eulerZ = Math::Radians(nodeSettings.lclRotation.z);
					node->rotation = Math::EulerAnglesFromMatrix((noParent ? axisWrangle : Math::Identity()) * (
						(nodeSettings.rotationOrder == FbxNodeSettings::FBXEulerOrder::eOrderXYZ) ? glm::eulerAngleZYX(eulerZ, eulerY, eulerX) :
						(nodeSettings.rotationOrder == FbxNodeSettings::FBXEulerOrder::eOrderXZY) ? glm::eulerAngleYZX(eulerY, eulerZ, eulerX) :
						(nodeSettings.rotationOrder == FbxNodeSettings::FBXEulerOrder::eOrderYZX) ? glm::eulerAngleXZY(eulerX, eulerZ, eulerY) :
						(nodeSettings.rotationOrder == FbxNodeSettings::FBXEulerOrder::eOrderYXZ) ? glm::eulerAngleZXY(eulerZ, eulerX, eulerY) :
						(nodeSettings.rotationOrder == FbxNodeSettings::FBXEulerOrder::eOrderZXY) ? glm::eulerAngleYXZ(eulerY, eulerX, eulerZ) :
						(nodeSettings.rotationOrder == FbxNodeSettings::FBXEulerOrder::eOrderZYX) ? glm::eulerAngleXYZ(eulerX, eulerY, eulerZ) : Math::Identity()));

					node->scale = nodeSettings.lclScaling;
					if (noParent) node->scale = node->scale * ROOT_POSE_SCALE;
					
					transformIndex[objectUid] = transforms.size();
					transforms.push_back(node);
					return true;
				};

				// Reads Light:
				auto readLight = [&]() -> bool {
					// __TODO__: Implement this crap!
					return readNotImplemented();
				};

				// Reads Camera:
				auto readCamera = [&]() -> bool {
					// __TODO__: Implement this crap!
					return readNotImplemented();
				};

				// Reads a Mesh:
				auto readMesh = [&]() -> bool {
					if (objectSubClass != "Mesh") {
						warning("FBXData::Extract::readMesh - subClassProperty<'", objectSubClass, "'> is not 'Mesh'!; Ignoring the node...");
						return true;
					}
					const Reference<PolyMesh> mesh = meshExtractor.ExtractData(objectNode, objectName, logger);
					if (mesh == nullptr) return false;
					result->m_meshIndex[objectUid] = static_cast<int64_t>(result->m_meshes.size());
					const Reference<FBXMesh> fbxMesh = Object::Instantiate<FBXMesh>();
					fbxMesh->objectId = objectUid;
					fbxMesh->mesh = mesh;
					result->m_meshes.push_back(fbxMesh);
					return true;
				};

				// Ignores unknown data type with a message:
				auto readUnknownType = [&]() -> bool {
					warning("FBXData::Extract - Object[", i, "].Name() = '", nodeAttribute, "': Unknown type! Object entry will be ignored...");
					return true;
				};

				// Distinguish object types:
				bool success;
				if (nodeAttribute == "Model") success = readModel();
				else if (nodeAttribute == "Light") success = readLight();
				else if (nodeAttribute == "Camera") success = readCamera();
				else if (nodeAttribute == "Geometry") success = readMesh();
				else success = readUnknownType();
				if (!success) return nullptr;
			}

			for (size_t i = 0; i < transforms.size(); i++) {
				const FBXNode* node = transforms[i];
				const std::unordered_map<int64_t, std::vector<int64_t>>::const_iterator parentIt = parentNodes.find(node->objectId);
				if (parentIt == parentNodes.end()) result->m_rootNode->children.push_back(node);
				else for (size_t j = 0; j < parentIt->second.size(); j++) {
					const std::unordered_map<int64_t, size_t>::const_iterator transformIt = transformIndex.find(parentIt->second[j]);
					if (transformIt == transformIndex.end()) continue;
					transforms[transformIt->second]->children.push_back(node);
				}
			}

			for (size_t i = 0; i < result->m_meshes.size(); i++) {
				const FBXMesh* mesh = result->m_meshes[i];
				const std::unordered_map<int64_t, std::vector<int64_t>>::const_iterator parentIt = parentNodes.find(mesh->objectId);
				if (parentIt == parentNodes.end()) continue;
				for (size_t j = 0; j < parentIt->second.size(); j++) {
					const std::unordered_map<int64_t, size_t>::const_iterator transformIt = transformIndex.find(parentIt->second[j]);
					if (transformIt == transformIndex.end()) continue;
					transforms[transformIt->second]->meshIndices.Push(i);
				}
			}
		}

		// __TODO__: Parse Connections...

		return result;
	}


	const FBXData::FBXGlobalSettings& FBXData::Settings()const { return m_globalSettings; }

	size_t FBXData::MeshCount()const { return m_meshes.size(); }

	const FBXData::FBXMesh* FBXData::GetMesh(size_t index)const { return m_meshes[index]; }

	const FBXData::FBXNode* FBXData::RootNode()const { return m_rootNode; }
}
