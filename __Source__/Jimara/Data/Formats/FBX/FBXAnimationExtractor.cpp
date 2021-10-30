#include "FBXAnimationExtractor.h"
#include "../../../Components/Transform.h"
#include <math.h> 


namespace Jimara {
	namespace FBXHelpers {
		namespace {
			static const double FBX_TIME_SCALE = (1.0 / 46186158000.0);


			// [2 - 8] (1 << [1 - 3])
			enum class InterpolationType : int64_t {
				eInterpolationConstant = 0x00000002,
				eInterpolationLinear = 0x00000004,
				eInterpolationCubic = 0x00000008
			};

			// [256 - 16384] (1 << [8 - 14])
			enum class TangentMode : int64_t {
				eTangentAuto = 0x00000100,
				eTangentTCB = 0x00000200,
				eTangentUser = 0x00000400,
				eTangentGenericBreak = 0x00000800,
				eTangentBreak = eTangentGenericBreak | eTangentUser,
				eTangentAutoBreak = eTangentGenericBreak | eTangentAuto,
				eTangentGenericClamp = 0x00001000,
				eTangentGenericTimeIndependent = 0x00002000,
				eTangentGenericClampProgressive = 0x00004000 | eTangentGenericTimeIndependent
			};

			// [16777216 - 33554432] (1 << [24 - 25])
			enum class WeightedMode : int64_t {
				eWeightedNone = 0x00000000,
				eWeightedRight = 0x01000000,
				eWeightedNextLeft = 0x02000000,
				eWeightedAll = eWeightedRight | eWeightedNextLeft
			};

			// [268435456 - 536870912] (1 << [28 - 29])
			enum class VelocityMode : int64_t {
				eVelocityNone = 0x00000000,
				eVelocityRight = 0x10000000,
				eVelocityNextLeft = 0x20000000,
				eVelocityAll = eVelocityRight | eVelocityNextLeft
			};

			// [256] (1 << 8)
			enum class ConstantMode : int64_t {
				eConstantStandard = 0x00000000,
				eConstantNext = 0x00000100
			};

			// [1048576 - 2097152] (1 << [20 - 21])
			enum class TangentVisibility : int64_t {
				eTangentShowNone = 0x00000000,
				eTangentShowLeft = 0x00100000,
				eTangentShowRight = 0x00200000,
				eTangentShowBoth = eTangentShowLeft | eTangentShowRight
			};


			inline static bool IsAnimationLayer(const FBXObjectIndex::NodeWithConnections& node) {
				return node.node.NodeAttribute() == "AnimationLayer";
			}

			inline static bool IsAnimationCurveNode(const FBXObjectIndex::NodeWithConnections* node) {
				return node != nullptr && node->node.NodeAttribute() == "AnimationCurveNode";
			}

			inline static bool IsAnimationCurve(const FBXObjectIndex::ObjectPropertyId& node) {
				return node.connection != nullptr && node.connection->node.NodeAttribute() == "AnimationCurve" && node.propertyName.has_value();
			}

			enum class CurveNodeType : uint8_t {
				UNKNOWN,
				Lcl_Translation,
				Lcl_Rotation,
				Lcl_Scaling
			};

			inline static std::pair<FBXAnimationExtractor::TransformInfo, CurveNodeType> GetNodeTransform(
				const FBXObjectIndex::NodeWithConnections& curveNode, Function<FBXAnimationExtractor::TransformInfo, FBXUid> getNodeById) {
				for (size_t i = 0; i < curveNode.parentConnections.Size(); i++) {
					const FBXObjectIndex::ObjectPropertyId& transformNode = curveNode.parentConnections[i];
					FBXAnimationExtractor::TransformInfo transformInfo = getNodeById(transformNode.connection->node.Uid());
					if (transformInfo.first != nullptr) {
						if (transformNode.propertyName.value() == "Lcl Translation") return std::make_pair(transformInfo, CurveNodeType::Lcl_Translation);
						else if (transformNode.propertyName.value() == "Lcl Rotation") return std::make_pair(transformInfo, CurveNodeType::Lcl_Rotation);
						else if (transformNode.propertyName.value() == "Lcl Scaling") return std::make_pair(transformInfo, CurveNodeType::Lcl_Scaling);
					}
				}
				return std::make_pair(FBXAnimationExtractor::TransformInfo(nullptr, AnimationClip::TripleFloatCombine::EvaluationMode::STANDARD), CurveNodeType::UNKNOWN);
			}

			inline static float FBXTimeToSeconds(int64_t fbxTime) { return static_cast<float>(static_cast<double>(fbxTime) * FBX_TIME_SCALE); }

			inline static bool FixTrackScaleAndOrientation(
				const FBXNode* node, CurveNodeType nodeType,
				Function<const FBXNode*, const FBXNode*> getNodeParent,
				float rootScale, const Matrix4& rootAxisWrangle,
				Reference<ParametricCurve<float, float>>& x,
				Reference<ParametricCurve<float, float>>& y,
				Reference<ParametricCurve<float, float>>& z,
				OS::Logger* logger) {
				auto forAll = [&](ParametricCurve<float, float>* curve, auto action) {
					TimelineCurve<float, BezierNode<float>>* timeline = dynamic_cast<TimelineCurve<float, BezierNode<float>>*>(curve);
					if (timeline != nullptr)
						for (TimelineCurve<float, BezierNode<float>>::iterator it = timeline->begin(); it != timeline->end(); ++it)
							action(it->second);
				};
				auto invertCurve = [&](ParametricCurve<float, float>* curve) {
					forAll(curve, [](BezierNode<float>& node) {
						node.Value() *= -1.0f;
						node.SetNextHandle(-node.NextHandle());
						if (node.IndependentHandles())
							node.SetPrevHandle(-node.PrevHandle());
						});
				};
				if (nodeType == CurveNodeType::Lcl_Rotation) {
					invertCurve(x);
					invertCurve(y);
				}
				else if (node != nullptr && getNodeParent(node) == nullptr) {
					if (nodeType == CurveNodeType::Lcl_Translation) {
						// __TODO__: Test these in detail...
						ParametricCurve<float, float>* wrangledX = nullptr;
						ParametricCurve<float, float>* wrangledY = nullptr;
						ParametricCurve<float, float>* wrangledZ = nullptr;
						auto assignWrangled = [&](ParametricCurve<float, float>* curve, Vector3 engineAxis) -> bool {
							Vector3 wrangledAxis = rootAxisWrangle * Vector4(engineAxis, 1.0f);
							if (wrangledAxis.x == -1.0f || wrangledAxis.y == -1.0f || wrangledAxis.z == -1.0f) {
								invertCurve(curve);
								wrangledAxis = -wrangledAxis;
							}
							if (std::abs(wrangledAxis.x - 1.0f) < 0.000001f) wrangledX = curve;
							else if (std::abs(wrangledAxis.y - 1.0f) < 0.000001f) wrangledY = curve;
							else if (std::abs(wrangledAxis.z - 1.0f) < 0.000001f) wrangledZ = curve;
							else {
								if (logger != nullptr) logger->Error("FBXAnimationExtractor::FixTrackScaleAndOrientation - Invalid axis wrangle! [", __FILE__, ": ", __LINE__, "]");
								return false;
							}
							return true;
						};
						if (!assignWrangled(x, Vector3(1.0f, 0.0f, 0.0f))) return false;
						else if (!assignWrangled(y, Vector3(0.0f, 1.0f, 0.0f))) return false;
						else if (!assignWrangled(z, Vector3(0.0f, 0.0f, -1.0f))) return false;
						if (wrangledX == nullptr || wrangledY == nullptr || wrangledZ == nullptr) {
							if (logger != nullptr) logger->Error("FBXAnimationExtractor::FixTrackScaleAndOrientation - Invalid axis wrangle! [", __FILE__, ": ", __LINE__, "]");
							return false;
						}
						x = wrangledX;
						y = wrangledY;
						z = wrangledZ;
					}
					auto scaleCurve = [&](ParametricCurve<float, float>* curve) {
						forAll(curve, [&](BezierNode<float>& node) {
							node.Value() *= rootScale;
							node.SetNextHandle(node.NextHandle() * rootScale);
							if (node.IndependentHandles())
								node.SetPrevHandle(node.PrevHandle() * rootScale);
							});
					};
					scaleCurve(x);
					scaleCurve(y);
					scaleCurve(z);
				}
				else if (nodeType == CurveNodeType::Lcl_Translation) invertCurve(z);
				return true;
			}

			void SetBindingPath(
				AnimationClip::Writer& writer, size_t trackId, CurveNodeType nodeType, const FBXNode* parentNode, 
				const Function<const FBXNode*, const FBXNode*>& getNodeParent) {
				if (parentNode == nullptr) {
					if (nodeType == CurveNodeType::Lcl_Translation) writer.SetTrackTargetField(trackId, "Position");
					else if (nodeType == CurveNodeType::Lcl_Rotation) writer.SetTrackTargetField(trackId, "Rotation");
					else if (nodeType == CurveNodeType::Lcl_Scaling) writer.SetTrackTargetField(trackId, "Scale");
					writer.ClearTrackBindings(trackId);
				}
				else {
					SetBindingPath(writer, trackId, nodeType, getNodeParent(parentNode), getNodeParent);
					writer.AddTrackBinding<Transform>(trackId, parentNode->name);
				}
			}
		}

		bool FBXAnimationExtractor::Extract(const FBXObjectIndex& objectIndex, OS::Logger* logger
			, float rootScale, const Matrix4& rootAxisWrangle
			, Function<TransformInfo, FBXUid> getNodeById
			, Function<const FBXNode*, const FBXNode*> getNodeParent
			, Callback<FBXAnimation*> onAnimationFound) {
			for (size_t nodeId = 0; nodeId < objectIndex.ObjectCount(); nodeId++) {
				const FBXObjectIndex::NodeWithConnections& node = objectIndex.ObjectNode(nodeId);
				if (!IsAnimationLayer(node)) continue;
				Reference<FBXAnimation> animation = ExtractLayer(node, logger, rootScale, rootAxisWrangle, getNodeById, getNodeParent);
				if (animation == nullptr) return false;
				else onAnimationFound(animation);
			}
			return true;
		}

		Reference<FBXAnimation> FBXAnimationExtractor::ExtractLayer(const FBXObjectIndex::NodeWithConnections& node, OS::Logger* logger
			, float rootScale, const Matrix4& rootAxisWrangle
			, Function<TransformInfo, FBXUid> getNodeById
			, Function<const FBXNode*, const FBXNode*> getNodeParent) {
			Reference<AnimationClip> clip = Object::Instantiate<AnimationClip>(node.node.Name());
			float minFrameTime = std::numeric_limits<float>::infinity();
			//static_assert(std::numeric_limits<float>::is_iec559);
			//float maxFrameTime = -minFrameTime;
			AnimationClip::Writer writer(clip);
			for (size_t i = 0; i < node.childConnections.Size(); i++) {
				const FBXObjectIndex::NodeWithConnections* child = node.childConnections[i].connection;
				if (IsAnimationCurveNode(child))
					if (!ExtractCurveNode(*child, writer, logger, rootScale, rootAxisWrangle, getNodeById, getNodeParent, minFrameTime)) return nullptr;
			}
			FixAnimationTimes(writer, minFrameTime);
			Reference<FBXAnimation> result = Object::Instantiate<FBXAnimation>();
			result->uid = node.node.Uid();
			result->clip = clip;
			return result;
		}

		bool FBXAnimationExtractor::ExtractCurveNode(const FBXObjectIndex::NodeWithConnections& node, AnimationClip::Writer& writer, OS::Logger* logger
			, float rootScale, const Matrix4& rootAxisWrangle
			, Function<TransformInfo, FBXUid> getNodeById
			, Function<const FBXNode*, const FBXNode*> getNodeParent
			, float& minFrameTime) {
			const std::pair<FBXAnimationExtractor::TransformInfo, CurveNodeType> parentTransform = GetNodeTransform(node, getNodeById);
			if (parentTransform.first.first == nullptr) return true; // Not tied to anything; safe to ignore...

			auto propertySymbol = [](const std::string_view& propName) -> char {
				if (propName.length() < 2 || propName[propName.length() - 2] != '|') return '\0';
				else return std::toupper(propName[propName.length() - 1]);
			};

			const Vector3 defaultValues = [&]() -> Vector3 {
				Vector3 defaults = Vector3(parentTransform.second == CurveNodeType::Lcl_Scaling ? 1.0f : 0.0f);
				const FBXContent::Node* properties70Node = node.node.Node()->FindChildNodeByName("Properties70");
				if (properties70Node == nullptr) return defaults;
				for (size_t i = 0; i < properties70Node->NestedNodeCount(); i++) {
					const FBXContent::Node& defaultValueNode = properties70Node->NestedNode(i);
					if (defaultValueNode.PropertyCount() < 5) continue;
					
					const char propKey = [&]()-> char {
						std::string_view propName;
						if (!defaultValueNode.NodeProperty(0).Get(propName)) return '\0';
						else return propertySymbol(propName);
					}();

					float* value =
						(propKey == 'X') ? (&defaults.x) :
						(propKey == 'Y') ? (&defaults.y) :
						(propKey == 'Z') ? (&defaults.z) : nullptr;
					if (value == nullptr) continue;

					float newDefault = 0.0f;
					if (defaultValueNode.NodeProperty(4).Get(newDefault))
						(*value) = newDefault;
				}
				return defaults;
			}();

			const FBXObjectIndex::NodeWithConnections* xCurveNode = nullptr;
			const FBXObjectIndex::NodeWithConnections* yCurveNode = nullptr;
			const FBXObjectIndex::NodeWithConnections* zCurveNode = nullptr;

			for (size_t i = 0; i < node.childConnections.Size(); i++) {
				const FBXObjectIndex::ObjectPropertyId& child = node.childConnections[i];
				if (!IsAnimationCurve(child)) continue;
				const char propKey = propertySymbol(child.propertyName.value());
				if (propKey == 'X') xCurveNode = child.connection;
				else if (propKey == 'Y') yCurveNode = child.connection;
				else if (propKey == 'Z') zCurveNode = child.connection;
			}

			bool error = false;
			auto getTrack = [&](const FBXObjectIndex::NodeWithConnections* curveNode, float defaultValue) -> Reference<ParametricCurve<float, float>> {
				Reference<TimelineCurve<float, BezierNode<float>>> curve = Object::Instantiate<TimelineCurve<float, BezierNode<float>>>();
				if (curveNode == nullptr) curve->operator[](0.0f) = defaultValue;
				else if (!ExtractCurve(*curveNode, defaultValue, *curve, logger, minFrameTime)) {
					error = true;
					return nullptr;
				}
				return curve;
			};
			Reference<AnimationClip::TripleFloatCombine> track = writer.AddTrack<AnimationClip::TripleFloatCombine>();
			track->X() = getTrack(xCurveNode, defaultValues.x);
			track->Y() = getTrack(yCurveNode, defaultValues.y);
			track->Z() = getTrack(zCurveNode, defaultValues.z);
			track->Mode() = (parentTransform.second == CurveNodeType::Lcl_Rotation) ? parentTransform.first.second : AnimationClip::TripleFloatCombine::EvaluationMode::STANDARD;
			if (!FixTrackScaleAndOrientation(
				parentTransform.first.first, parentTransform.second, getNodeParent, rootScale, rootAxisWrangle, track->X(), track->Y(), track->Z(), logger)) return false;
			SetBindingPath(writer, track->Index(), parentTransform.second, parentTransform.first.first, getNodeParent);
			
			return !error;
		}

		bool FBXAnimationExtractor::ExtractCurve(
			const FBXObjectIndex::NodeWithConnections& node, float defaultValue, TimelineCurve<float, BezierNode<float>>& curve, OS::Logger* logger,
			float& minFrameTime) {
			auto error = [&](auto... msg) -> bool { if (logger != nullptr) logger->Error(msg...); return false; };
			
			// 'Default' Node:
			{
				const FBXContent::Node* defaultNode = node.node.Node()->FindChildNodeByName("Default");
				if (defaultNode != nullptr && defaultNode->PropertyCount() > 0) {
					float defVal = 0.0f;
					if (defaultNode->NodeProperty(0).Get(defVal))
						defaultValue = defVal;
				}
			}
			
			// 'KeyTime' Node:
			{
				auto defaultCurve = [&]() -> bool {
					curve[0.0f].Value() = defaultValue;
					return true;
				};
				const FBXContent::Node* keyTimeNode = node.node.Node()->FindChildNodeByName("KeyTime");
				if (keyTimeNode == nullptr || keyTimeNode->PropertyCount() <= 0) return defaultCurve();
				else if (!keyTimeNode->NodeProperty(0).Fill(m_timeBuffer, true))
					return error("FBXAnimationExtractor::ExtractCurve - 'KeyTime' node malformed!");
				else if (m_timeBuffer.size() <= 0) return defaultCurve();
			}

			auto fill = [&](const std::string_view& nodeName, auto* buffer) -> bool {
				const FBXContent::Node* subNode = node.node.Node()->FindChildNodeByName(nodeName);
				if (subNode != nullptr && subNode->PropertyCount() > 0 && subNode->NodeProperty(0).Fill(*buffer, true)) return true;
				else return error("FBXAnimationExtractor::ExtractCurve - '", nodeName, "' node ", (subNode == nullptr) ? "missing" : "malformed", '!');
			};

			// 'KeyValueFloat' Node:
			{
				if (!fill("KeyValueFloat", &m_valueBuffer)) return false;
				else if (m_valueBuffer.size() < m_timeBuffer.size())
					return error("FBXAnimationExtractor::ExtractCurve - 'KeyValueFloat' does not contain enough elements! ",
						"(needed: ", m_timeBuffer.size(), "; present: ", m_valueBuffer.size(), ")");
			}

			// 'KeyAttrFlags', 'KeyAttrDataFloat' and 'KeyAttrRefCount' Nodes:
			{
				if ((!fill("KeyAttrFlags", &m_attrFlagBuffer)) || (!fill("KeyAttrDataFloat", &m_dataBuffer)) || (!fill("KeyAttrRefCount", &m_refCountBuffer))) return false;
				else if ((m_attrFlagBuffer.size() * 4) > m_dataBuffer.size())
					return error("FBXAnimationExtractor::ExtractCurve - 'KeyAttrDataFloat' does not contain enough entries! ",
						"(needed: ", (m_attrFlagBuffer.size() * 4), "(KeyAttrFlags.size * 4); present: ", m_dataBuffer.size(), ")");
				else if (m_attrFlagBuffer.size() > m_refCountBuffer.size())
					return error("FBXAnimationExtractor::ExtractCurve - 'KeyAttrRefCount' does not contain enough entries! ",
						"(needed: ", m_attrFlagBuffer.size(), "(KeyAttrFlags.size); present: ", m_refCountBuffer.size(), ")");
				else {
					size_t totalRefCount = 0u;
					for (size_t i = 0; i < m_attrFlagBuffer.size(); i++)
						totalRefCount += m_refCountBuffer[i];
					if (totalRefCount < m_timeBuffer.size())
						return error("FBXAnimationExtractor::ExtractCurve - Sum of 'KeyAttrRefCount' less than the number of keyframes! ",
							"(needed: ", m_timeBuffer.size(), "; present: ", totalRefCount, ")");
				}
			}

			if (!FillCurve(logger, curve)) return false;
			if (curve.size() > 0) {
				const float first = curve.begin()->first;
				if (minFrameTime > first) minFrameTime = first;
			}
			return true;
		}


		bool FBXAnimationExtractor::FillCurve(OS::Logger* logger, TimelineCurve<float, BezierNode<float>>& curve)const {
			size_t flagsId;
			InterpolationType interpolationType;
			TangentMode tangentMode;
			WeightedMode weightedMode;
			VelocityMode velocityMode;
			ConstantMode constantMode;
			TangentVisibility tangentVisibility;
			const float* data;
			size_t refsLeft;

			auto updateFlags = [&](size_t newFlagId) {
				flagsId = newFlagId;
				int64_t flags = m_attrFlagBuffer[flagsId];
				auto hasFlag = [&](auto flag) { return (flags & static_cast<int64_t>(flag)) == static_cast<int64_t>(flag); };

				// InterpolationType:
				interpolationType =
					hasFlag(InterpolationType::eInterpolationConstant) ? InterpolationType::eInterpolationConstant :
					hasFlag(InterpolationType::eInterpolationLinear) ? InterpolationType::eInterpolationLinear : InterpolationType::eInterpolationCubic;

				// __TODO__: Set tangent mode...
				tangentMode = TangentMode::eTangentAuto;

				// WeightedMode: [Currently not supported]
				{
					weightedMode = hasFlag(WeightedMode::eWeightedRight) ? WeightedMode::eWeightedRight : WeightedMode::eWeightedNone;
					if (hasFlag(WeightedMode::eWeightedNextLeft))
						weightedMode = static_cast<WeightedMode>(static_cast<int64_t>(WeightedMode::eWeightedNextLeft) | static_cast<int64_t>(weightedMode));
				}

				// VelocityMode: [Currently not supported]
				{
					velocityMode = hasFlag(VelocityMode::eVelocityRight) ? VelocityMode::eVelocityRight : VelocityMode::eVelocityNone;
					if (hasFlag(VelocityMode::eVelocityNextLeft))
						velocityMode = static_cast<VelocityMode>(static_cast<int64_t>(VelocityMode::eVelocityNextLeft) | static_cast<int64_t>(velocityMode));
				}

				// ConstantMode:
				constantMode = hasFlag(ConstantMode::eConstantNext) ? ConstantMode::eConstantNext : ConstantMode::eConstantStandard;

				// TangentVisibility: [We don't care about this one]
				{
					tangentVisibility = hasFlag(TangentVisibility::eTangentShowRight) ? TangentVisibility::eTangentShowRight : TangentVisibility::eTangentShowNone;
					if (hasFlag(TangentVisibility::eTangentShowLeft))
						tangentVisibility = static_cast<TangentVisibility>(static_cast<int64_t>(TangentVisibility::eTangentShowLeft) | static_cast<int64_t>(tangentVisibility));
				}

				// Data & Refs:
				data = m_dataBuffer.data() + (flagsId << 2);
				refsLeft = m_refCountBuffer[flagsId];
			};

			updateFlags(0);
			for (size_t i = 0; i < m_timeBuffer.size(); i++) {
				while (refsLeft <= 0) updateFlags(flagsId + 1);
				float time = FBXTimeToSeconds(m_timeBuffer[i]);
				BezierNode<float>& node = curve[time];
				node.Value() = m_valueBuffer[i];
				if (interpolationType == InterpolationType::eInterpolationConstant)
					node.InterpolateConstant() = BezierNode<float>::ConstantInterpolation(true, constantMode == ConstantMode::eConstantNext);
				else if (interpolationType == InterpolationType::eInterpolationLinear) {
					node.IndependentHandles() = true;
					size_t nextT = (i + 1);
					if (nextT < m_timeBuffer.size()) {
						BezierNode<float>& nextNode = curve[FBXTimeToSeconds(m_timeBuffer[nextT])];
						nextNode.IndependentHandles() = true;
						float delta = m_valueBuffer[nextT] - node.Value();
						node.NextTangent() = delta;
						nextNode.PrevTangent() = -delta;
					}
					else node.NextHandle() = 0.0f;
				}
				else {
					// __TODO__: Maybe, pay attention to flags down the line?
					float rightTangent = data[0];
					if (std::abs(rightTangent - node.NextTangent()) > 0.0000001f) {
						node.IndependentHandles() = true;
						node.NextTangent() = rightTangent;
					}
					size_t nextT = (i + 1);
					if (nextT < m_timeBuffer.size())
						curve[FBXTimeToSeconds(m_timeBuffer[nextT])].PrevTangent() = -data[1];
				}
			}
			return true;
		}

		void FBXAnimationExtractor::FixAnimationTimes(AnimationClip::Writer& writer, float minFrameTime) {
			if (isinf(minFrameTime)) return;
			float maxFrameTime = 0.0f;
			auto fixTrack = [&](ParametricCurve<float, float>* track) -> bool {
				TimelineCurve<float, BezierNode<float>>* curve = dynamic_cast<TimelineCurve<float, BezierNode<float>>*>(track);
				if (curve == nullptr || curve->empty()) return false;
				if (minFrameTime != 0.0f) {
					m_bezierNodeBuffer.clear();
					for (TimelineCurve<float, BezierNode<float>>::const_iterator it = curve->begin(); it != curve->end(); ++it)
						m_bezierNodeBuffer.push_back(std::make_pair(it->first, it->second));
					curve->clear();
					for (size_t i = 0; i < m_bezierNodeBuffer.size(); i++) {
						const std::pair<float, BezierNode<float>>& node = m_bezierNodeBuffer[i];
						curve->insert(std::make_pair(node.first - minFrameTime, node.second));
					}
				}
				float end = curve->rbegin()->first;
				if (maxFrameTime < end) maxFrameTime = end;
				return true;
			};
			for (size_t i = 0; i < writer.TrackCount(); i++) {
				AnimationClip::Track* track = writer.GetTrack(i);
				AnimationClip::TripleFloatCombine* vec3Track = dynamic_cast<AnimationClip::TripleFloatCombine*>(track);
				if (vec3Track != nullptr) {
					fixTrack(vec3Track->X());
					fixTrack(vec3Track->Y());
					fixTrack(vec3Track->Z());
				}
				else fixTrack(dynamic_cast<AnimationClip::FloatTrack*>(track));
			}
			writer.SetDuration(maxFrameTime);
		}
	}
}
