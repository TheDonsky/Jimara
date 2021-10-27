#include "FBXData.h"
#include "FBXObjectIndex.h"
#include "FBXPropertyParser.h"
#include "FBXMeshExtractor.h"
#include "FBXAnimationExtractor.h"
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
				if (propertyNode.PropertyCount() < 5) return Error(logger, false, "FBXData::Extract::ReadGlobalSettings - ", FBXHelpers::FBXPropertyParser::PropertyName(propertyNode), " has no value!");
				else if (!propertyNode.NodeProperty(4).Get(sign)) 
					return Error(logger, false, "FBXData::Extract::ReadGlobalSettings - ", FBXHelpers::FBXPropertyParser::PropertyName(propertyNode), " is not an integer/bool!");
				reinterpret_cast<FBXGlobalSettings*>(settings)->axisSign[axisIndex] = (sign > 0) ? 1.0f : (-1.0f);
				return true;
			};

			static const FBXHelpers::FBXPropertyParser PROPERTY_EXTRACTOR({
				FBXHelpers::FBXPropertyParser::ParserForPropertyName(AXIS_NAMES[UP_INDEX], [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseEnumProperty(reinterpret_cast<FBXGlobalSettings*>(target)->axisIndex[UP_INDEX], propertyNode, logger);
					}),
				FBXHelpers::FBXPropertyParser::ParserForPropertyName("UpAxisSign", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return getAxisSign(target, UP_INDEX, propertyNode, logger);
					}),
				FBXHelpers::FBXPropertyParser::ParserForPropertyName(AXIS_NAMES[FRONT_INDEX], [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseEnumProperty(reinterpret_cast<FBXGlobalSettings*>(target)->axisIndex[FRONT_INDEX], propertyNode, logger);
					}),
				FBXHelpers::FBXPropertyParser::ParserForPropertyName("FrontAxisSign", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return getAxisSign(target, FRONT_INDEX, propertyNode, logger);
					}),
				FBXHelpers::FBXPropertyParser::ParserForPropertyName(AXIS_NAMES[COORD_INDEX], [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseEnumProperty(reinterpret_cast<FBXGlobalSettings*>(target)->axisIndex[COORD_INDEX], propertyNode, logger);
					}),
				FBXHelpers::FBXPropertyParser::ParserForPropertyName("CoordAxisSign", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return getAxisSign(target, COORD_INDEX, propertyNode, logger);
					}),
				FBXHelpers::FBXPropertyParser::ParserForPropertyName(AXIS_NAMES[ORIGINAL_UP_INDEX], [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseEnumProperty<FBXGlobalSettings::AxisIndex, FBXHelpers::FBXPropertyParser::IgnoreIfNegative<int64_t>>(
							reinterpret_cast<FBXGlobalSettings*>(target)->axisIndex[ORIGINAL_UP_INDEX], propertyNode, logger);
					}),
				FBXHelpers::FBXPropertyParser::ParserForPropertyName("OriginalUpAxisSign", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return getAxisSign(target, ORIGINAL_UP_INDEX, propertyNode, logger);
					}),
				FBXHelpers::FBXPropertyParser::ParserForPropertyName("UnitScaleFactor", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FBXGlobalSettings*>(target)->unitScale, propertyNode, logger);
					}),
				FBXHelpers::FBXPropertyParser::ParserForPropertyName("OriginalUnitScaleFactor", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FBXGlobalSettings*>(target)->originalUnitScaleFactor, propertyNode, logger);
					})
				});

			FBXGlobalSettings settings;
			if (!PROPERTY_EXTRACTOR.ParseProperties(&settings, properties70Node, logger)) return false;

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
				static const FBXHelpers::FBXPropertyParser PROPERTY_EXTRACTOR({
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("Lcl Translation", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->lclTranslation, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("Lcl Rotation", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->lclRotation, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("Lcl Scaling", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->lclScaling, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("Visibility", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->visibility, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("Visibility Inheritance", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->visibilityInheritance, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("QuaternionInterpolate", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseEnumProperty(reinterpret_cast<FbxNodeSettings*>(target)->quaternionInterpolate, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationOffset", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationOffset, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationPivot", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationPivot, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("ScalingOffset", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingOffset, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("ScalingPivot", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingPivot, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("TranslationActive", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationActive, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("TranslationMin", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMin, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("TranslationMax", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMax, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("TranslationMinX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMinX, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("TranslationMinY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMinY, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("TranslationMinZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMinZ, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("TranslationMaxX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMaxX, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("TranslationMaxY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMaxY, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("TranslationMaxZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->translationMaxZ, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationOrder", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseEnumProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationOrder, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationSpaceForLimitOnly", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationSpaceForLimitOnly, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationStiffnessX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationStiffnessX, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationStiffnessY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationStiffnessY, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationStiffnessZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationStiffnessZ, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("AxisLen", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->axisLen, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("PreRotation", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->preRotation, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("PostRotation", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->postRotation, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationActive", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationActive, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationMin", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMin, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationMax", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMax, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationMinX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMinX, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationMinY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMinY, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationMinZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMinZ, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationMaxX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMaxX, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationMaxY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMaxY, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("RotationMaxZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->rotationMaxZ, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("InheritType", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseEnumProperty(reinterpret_cast<FbxNodeSettings*>(target)->inheritType, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("ScalingActive", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingActive, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("ScalingMin", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMin, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("ScalingMax", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMax, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("ScalingMinX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMinX, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("ScalingMinY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMinY, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("ScalingMinZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMinZ, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("ScalingMaxX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMaxX, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("ScalingMaxY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMaxY, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("ScalingMaxZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->scalingMaxZ, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("GeometricTranslation", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->geometricTranslation, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("GeometricRotation", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->geometricRotation, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("GeometricScaling", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->geometricScaling, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("MinDampRangeX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->minDampRangeX, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("MinDampRangeY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->minDampRangeY, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("MinDampRangeZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->minDampRangeZ, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("MaxDampRangeX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->maxDampRangeX, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("MaxDampRangeY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->maxDampRangeY, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("MaxDampRangeZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->maxDampRangeZ, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("MinDampStrengthX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->minDampStrengthX, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("MinDampStrengthY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->minDampStrengthY, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("MinDampStrengthZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->minDampStrengthZ, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("MaxDampStrengthX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->maxDampStrengthX, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("MaxDampStrengthY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->maxDampStrengthY, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("MaxDampStrengthZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->maxDampStrengthZ, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("PreferedAngleX", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->preferedAngleX, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("PreferedAngleY", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->preferedAngleY, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("PreferedAngleZ", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->preferedAngleZ, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("LookAtProperty", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->lookAtProperty, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("UpVectorProperty", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->upVectorProperty, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("Show", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->show, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("NegativePercentShapeSupport", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->negativePercentShapeSupport, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("DefaultAttributeIndex", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->defaultAttributeIndex, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("Freeze", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->freeze, propertyNode, logger);
						}),
					FBXHelpers::FBXPropertyParser::ParserForPropertyName("LODBox", [](void* target, const FBXContent::Node& propertyNode, OS::Logger* logger) -> bool {
						return FBXHelpers::FBXPropertyParser::ParseProperty(reinterpret_cast<FbxNodeSettings*>(target)->lodBox, propertyNode, logger);
						})
					});

				const FBXContent::Node* properties70Node = parsedNode.FindChildNodeByName("Properties70");
				if (properties70Node == nullptr) {
					if (logger != nullptr) logger->Warning("FBXData::Extract::FbxNodeSettings::Extract - 'Properties70' missing in 'FbxNode' node!");
					return true;
				}
				if (!PROPERTY_EXTRACTOR.ParseProperties(reinterpret_cast<void*>(this), properties70Node, logger)) return false;

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
		const FBXContent::Node* takesNode = sourceContent->RootNode().FindChildNodeByName("Takes", 10);

		// Notes:
		// 0. We ignore the contents of FBXHeaderExtension for performance and also, we don't really care for them; invalid FBX files may pass here;
		// 1. We also don't care about FileId, CreationTime Creator, Documents, References and Takes for similar reasons;
		// 2. I've followed this link to make sense of the content: 
		//    https://web.archive.org/web/20160605023014/https://wiki.blender.org/index.php/User:Mont29/Foundation/FBX_File_Structure#Spaces_.26_Parenting
		Reference<FBXData> result = Object::Instantiate<FBXData>();

		// Parse GlobalSettings:
		if (!ReadGlobalSettings(&result->m_globalSettings, globalSettingsNode, logger)) return nullptr;
		const float ROOT_POSE_SCALE = 0.01f;
		const Matrix4 axisWrangle = Math::Transpose(Matrix4(
			Vector4(result->m_globalSettings.coordAxis.x, result->m_globalSettings.coordAxis.y, -result->m_globalSettings.coordAxis.z, 0.0f),
			Vector4(result->m_globalSettings.upAxis.x, result->m_globalSettings.upAxis.y, -result->m_globalSettings.upAxis.z, 0.0f),
			Vector4(result->m_globalSettings.forwardAxis.x, result->m_globalSettings.forwardAxis.y, -result->m_globalSettings.forwardAxis.z, 0.0f),
			Vector4(0.0f, 0.0f, 0.0f, 1.0f)));
		const bool engineIsLeftHanded = (Math::Dot(Math::Cross(Math::Right(), Math::Up()), Math::Forward()) > 0.0f);
		if (!engineIsLeftHanded) return error("FBXData::Extract - Internal error: engine is supposed to have a left handed coordinate system!");
		const bool isLeftHanded = Math::Dot(Math::Cross(result->m_globalSettings.coordAxis, result->m_globalSettings.upAxis), result->m_globalSettings.forwardAxis) > 0.0f;
		if (isLeftHanded) return error("FBXData::Extract - FBX files are expected to have right handed coordinate systems!");

		// Parse Definitions:
		FBXTemplates templates;
		if (!templates.Extract(definitionsNode, logger)) return nullptr;

		// Build Node index:
		FBXHelpers::FBXObjectIndex objectIndex;
		if (!objectIndex.Build(sourceContent->RootNode(), logger)) return nullptr;

		// Parse Objects (Incomplete...):
		FBXHelpers::FBXMeshExtractor meshExtractor;
		std::unordered_map<int64_t, size_t> transformIndex;
		std::vector<std::pair<Reference<FBXNode>, size_t>> transforms;

		for (size_t i = 0; i < objectIndex.ObjectCount(); i++) {
			const FBXHelpers::FBXObjectIndex::NodeWithConnections& objectNode = objectIndex.ObjectNode(i);

			// Fallback for reading types that are not yet implemented:
			auto readNotImplemented = [&]() -> bool {
				warning("FBXData::Extract - Object[", i, "].Name() = '", objectNode.node.NodeAttribute(), "'; [__TODO__]: Parser not yet implemented! Object entry will be ignored...");
				return true;
			};

			// Reads a Model:
			auto readModel = [&]() -> bool {
				FbxNodeSettings nodeSettings = templates.nodeSettings;
				if (!nodeSettings.Extract(*objectNode.node.Node(), logger)) return false;
				// __TODO__: Implement this crap properly!
				const Reference<FBXNode> node = Object::Instantiate<FBXNode>();
				node->uid = objectNode.node.Uid();
				node->name = objectNode.node.Name();

				node->position = Vector3(nodeSettings.lclTranslation.x, nodeSettings.lclTranslation.y, -nodeSettings.lclTranslation.z);
				const float
					eulerX = Math::Radians(-nodeSettings.lclRotation.x),
					eulerY = Math::Radians(-nodeSettings.lclRotation.y),
					eulerZ = Math::Radians(nodeSettings.lclRotation.z);
				node->rotation = Math::EulerAnglesFromMatrix(
					(nodeSettings.rotationOrder == FbxNodeSettings::FBXEulerOrder::eOrderXYZ) ? glm::eulerAngleZYX(eulerZ, eulerY, eulerX) :
					(nodeSettings.rotationOrder == FbxNodeSettings::FBXEulerOrder::eOrderXZY) ? glm::eulerAngleYZX(eulerY, eulerZ, eulerX) :
					(nodeSettings.rotationOrder == FbxNodeSettings::FBXEulerOrder::eOrderYZX) ? glm::eulerAngleXZY(eulerX, eulerZ, eulerY) :
					(nodeSettings.rotationOrder == FbxNodeSettings::FBXEulerOrder::eOrderYXZ) ? glm::eulerAngleZXY(eulerZ, eulerX, eulerY) :
					(nodeSettings.rotationOrder == FbxNodeSettings::FBXEulerOrder::eOrderZXY) ? glm::eulerAngleYXZ(eulerY, eulerX, eulerZ) :
					(nodeSettings.rotationOrder == FbxNodeSettings::FBXEulerOrder::eOrderZYX) ? glm::eulerAngleXYZ(eulerX, eulerY, eulerZ) : Math::Identity());

				node->scale = nodeSettings.lclScaling;

				transformIndex[objectNode.node.Uid()] = transforms.size();
				transforms.push_back(std::make_pair(node, i));
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
				if (objectNode.node.SubClass() != "Mesh") {
					warning("FBXData::Extract::readMesh - subClassProperty<'", objectNode.node.SubClass(), "'> is not 'Mesh'!; Ignoring the node...");
					return true;
				}
				const Reference<FBXMesh> mesh = meshExtractor.ExtractMesh(objectNode, logger);
				if (mesh == nullptr) return false;
				result->m_meshes.push_back(mesh);
				return true;
			};

			// Distinguish object types:
			bool success;
			if (objectNode.node.NodeAttribute() == "Model") success = readModel();
			else if (objectNode.node.NodeAttribute() == "Light") success = readLight();
			else if (objectNode.node.NodeAttribute() == "Camera") success = readCamera();
			else if (objectNode.node.NodeAttribute() == "Geometry") success = readMesh();
			else success = true; // It's ok to ignore things we don't understand. Just like life, right?
			if (!success) return nullptr;
		}


		// Finds parent transform indices for object with given index:
		auto findParentTransforms = [&](size_t nodeId, auto onFound) {
			const FBXHelpers::FBXObjectIndex::NodeWithConnections& node = objectIndex.ObjectNode(nodeId);
			for (size_t i = 0; i < node.parentConnections.Size(); i++) {
				const std::unordered_map<FBXUid, size_t>::const_iterator it = transformIndex.find(node.parentConnections[i].connection->node.Uid());
				if (it == transformIndex.end()) continue;
				else onFound(it->second);
			}
		};

		// Connect transforms in parent-child heirarchy:
		for (size_t i = 0; i < transforms.size(); i++) {
			bool foundMultiple = false;
			auto findParentTransform = [&](size_t nodeId)->const std::pair<Reference<FBXNode>, size_t>* {
				const std::pair<Reference<FBXNode>, size_t>* parent = nullptr;
				findParentTransforms(nodeId, [&](size_t parentIndex) {
					if (parent != nullptr) foundMultiple = true;
					else parent = transforms.data() + parentIndex;
					});
				return parent;
			};
			const std::pair<Reference<FBXNode>, size_t>& node = transforms[i];
			const std::pair<Reference<FBXNode>, size_t>* const parentNode = findParentTransform(node.second);
			if (parentNode != nullptr) {
				for (const std::pair<Reference<FBXNode>, size_t>* parent = parentNode; parent != nullptr; parent = findParentTransform(parent->second)) {
					if (foundMultiple) return error("FBXData::Extract - Transform has more than one parent!");
					if (parent == &node) return error("FBXData::Extract - Found circular dependency!");
				}
				parentNode->first->children.push_back(node.first);
			}
			else {
				node.first->position = axisWrangle * Vector4(node.first->position * ROOT_POSE_SCALE, 0.0f);
				node.first->rotation = Math::EulerAnglesFromMatrix(axisWrangle * Math::MatrixFromEulerAngles(node.first->rotation));
				node.first->scale *= ROOT_POSE_SCALE;
				result->m_rootNode->children.push_back(node.first);
			}
		}

		// Attach meshes to transforms:
		for (size_t i = 0; i < result->m_meshes.size(); i++) {
			const FBXMesh* mesh = result->m_meshes[i];
			std::optional<size_t> objectNodeId = objectIndex.ObjectNodeByUID(mesh->uid);
			if (!objectNodeId.has_value()) return error("BXData::Extract - Internal error: Mesh node not found in index!");
			findParentTransforms(objectNodeId.value(), [&](size_t parentIndex) { transforms[parentIndex].first->meshes.Push(mesh); });
		}

		// Extract animation:
		FBXHelpers::FBXAnimationExtractor animationExtractor;
		void(*onAnimationFound)(FBXData*, FBXAnimation*) = [](FBXData* data, FBXAnimation* animation) { 
			data->m_animations.push_back(animation);
		};
		if (!animationExtractor.Extract(objectIndex, logger, Callback<FBXAnimation*>(onAnimationFound, result.operator->())))
			return nullptr;

		return result;
	}


	const FBXData::FBXGlobalSettings& FBXData::Settings()const { return m_globalSettings; }

	size_t FBXData::MeshCount()const { return m_meshes.size(); }

	const FBXMesh* FBXData::GetMesh(size_t index)const { return m_meshes[index]; }

	size_t FBXData::AnimationCount()const { return m_animations.size(); }

	const FBXAnimation* FBXData::GetAnimation(size_t index)const { return m_animations[index]; }

	const FBXNode* FBXData::RootNode()const { return m_rootNode; }
}
