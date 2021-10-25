#include "FBXAnimationExtractor.h"


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

			inline static bool IsAnimationCurve(const FBXObjectIndex::NodeWithConnections* node) {
				return node != nullptr && node->node.NodeAttribute() == "AnimationCurve";
			}

			inline static const FBXObjectIndex::NodeWithConnections* GetNodeTransform(const FBXObjectIndex::NodeWithConnections& curveNode) {
				for (size_t i = 0; i < curveNode.childConnections.Size(); i++) {
					const FBXObjectIndex::NodeWithConnections* transformNode = curveNode.childConnections[i].connection;
					if (transformNode->node.NodeAttribute() == "Model") return transformNode;
				}
				return 0;
			}

			inline static constexpr float FBXTimeToSeconds(int64_t fbxTime) { return static_cast<float>(static_cast<double>(fbxTime) * FBX_TIME_SCALE); }
		}

		bool FBXAnimationExtractor::Extract(const FBXObjectIndex& objectIndex, OS::Logger* logger, Callback<FBXAnimation*> onAnimationFound) {
			for (size_t nodeId = 0; nodeId < objectIndex.ObjectCount(); nodeId++) {
				const FBXObjectIndex::NodeWithConnections& node = objectIndex.ObjectNode(nodeId);
				if (!IsAnimationLayer(node)) continue;
				Reference<FBXAnimation> animation = ExtractLayer(node, logger);
				if (animation == nullptr) return false;
				else onAnimationFound(animation);
			}
			return true;
		}

		Reference<FBXAnimation> FBXAnimationExtractor::ExtractLayer(const FBXObjectIndex::NodeWithConnections& node, OS::Logger* logger) {
			Reference<AnimationClip> clip = Object::Instantiate<AnimationClip>(node.node.Name());
			AnimationClip::Writer writer(clip);
			for (size_t i = 0; i < node.childConnections.Size(); i++) {
				const FBXObjectIndex::NodeWithConnections* child = node.childConnections[i].connection;
				if (IsAnimationCurveNode(child))
					if (!ExtractCurveNode(*child, writer, logger)) return nullptr;
			}
			Reference<FBXAnimation> result = Object::Instantiate<FBXAnimation>();
			result->uid = node.node.Uid();
			result->clip = clip;
			return result;
		}

		bool FBXAnimationExtractor::ExtractCurveNode(const FBXObjectIndex::NodeWithConnections& node, AnimationClip::Writer& writer, OS::Logger* logger) {
			const FBXObjectIndex::NodeWithConnections* parentTransform = GetNodeTransform(node);
			if (parentTransform == nullptr) return true; // Not tied to anything; safe to ignore...
			for (size_t i = 0; i < node.childConnections.Size(); i++) {
				const FBXObjectIndex::ObjectPropertyId& child = node.childConnections[i];
				if (!IsAnimationCurve(child.connection)) continue;
				// __TODO__: Maybe? change default value?
				Reference<ParametricCurve<float, float>> curve = ExtractCurve(*child.connection, 0.0f, logger);
				if (curve == nullptr) return false;
				// __TODO__: Create tracks and link them to corresponding fields
			}
			return true;
		}

		Reference<ParametricCurve<float, float>> FBXAnimationExtractor::ExtractCurve(const FBXObjectIndex::NodeWithConnections& node, float defaultValue, OS::Logger* logger) {
			auto error = [&](auto... msg) -> Reference<ParametricCurve<float, float>> { if (logger != nullptr) logger->Error(msg...); return nullptr; };
			
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
				auto defaultCurve = [&]() -> Reference<ParametricCurve<float, float>> {
					Reference<TimelineCurve<float, BezierNode<float>>> curve = Object::Instantiate<TimelineCurve<float, BezierNode<float>>>();
					curve->operator[](0.0f).Value() = defaultValue;
					return curve;
				};
				const FBXContent::Node* keyTimeNode = node.node.Node()->FindChildNodeByName("KeyTime");
				if (keyTimeNode == nullptr || keyTimeNode->PropertyCount() <= 0) return defaultCurve();
				else if (!keyTimeNode->NodeProperty(0).Fill(m_timeBuffer, true))
					return error("FBXAnimationExtractor::ExtractCurve - 'KeyTime' node malformed!");
				else if (m_timeBuffer.size() <= 0) return defaultCurve();
			}

			auto fill = [&](const std::string_view& nodeName, auto buffer) -> bool {
				const FBXContent::Node* subNode = node.node.Node()->FindChildNodeByName(nodeName);
				if (subNode != nullptr && subNode->PropertyCount() > 0 && subNode->NodeProperty(0).Fill(buffer, true)) return true;
				error("FBXAnimationExtractor::ExtractCurve - '", nodeName, "' node ", (subNode == nullptr) ? "missing" : "malformed", '!');
				return false;
			};

			// 'KeyValueFloat' Node:
			{
				if (!fill("KeyValueFloat", m_valueBuffer)) return nullptr;
				else if (m_valueBuffer.size() < m_timeBuffer.size())
					return error("FBXAnimationExtractor::ExtractCurve - 'KeyValueFloat' does not contain enough elements! ",
						"(needed", m_timeBuffer.size(), "; present: ", m_valueBuffer.size(), ")");
			}

			// 'KeyAttrFlags', 'KeyAttrDataFloat' and 'KeyAttrRefCount' Nodes:
			{
				if ((!fill("KeyAttrFlags", m_attrFlagBuffer)) || (!fill("KeyAttrDataFloat", m_dataBuffer)) || (!fill("KeyAttrRefCount", m_refCountBuffer))) return nullptr;
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

			return CreateCurve(logger);
		}


		Reference<ParametricCurve<float, float>> FBXAnimationExtractor::CreateCurve(OS::Logger* logger)const {
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

			Reference<TimelineCurve<float, BezierNode<float>>> curve = Object::Instantiate<TimelineCurve<float, BezierNode<float>>>();
			updateFlags(0);
			for (size_t i = 0; i < m_timeBuffer.size(); i++) {
				while (refsLeft <= 0) updateFlags(flagsId + 1);
				float time = FBXTimeToSeconds(m_timeBuffer[i]);
				BezierNode<float>& node = curve->operator[](time);
				node.Value() = m_valueBuffer[i];
				if (interpolationType == InterpolationType::eInterpolationConstant)
					node.InterpolateConstant() = BezierNode<float>::ConstantInterpolation(true, constantMode == ConstantMode::eConstantNext);
				else if (interpolationType == InterpolationType::eInterpolationLinear) {
					// __TODO__: Handle linear to next...
				}
				else {
					// __TODO__: Handle cubic to next...
				}
			}
			return curve;
		}
	}
}
