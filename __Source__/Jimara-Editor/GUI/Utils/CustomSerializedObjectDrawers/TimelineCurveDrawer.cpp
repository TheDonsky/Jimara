#include "TimelineCurveDrawer.h"
#include "../DrawTooltip.h"
#include "Math/Curves.h"


namespace Jimara {
	namespace Editor {
		namespace {
			inline static const TimelineCurveDrawer* MainTimelineCurveDrawer() {
				static const Reference<TimelineCurveDrawer> drawer = Object::Instantiate<TimelineCurveDrawer>();
				return drawer;
			}
		}

		bool TimelineCurveDrawer::DrawObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const {
			typedef std::map<float, BezierNode<float>> FloatingPointCurve;
			if (dynamic_cast<const Serialization::SerializerList::From<FloatingPointCurve>*>(object.Serializer()) == nullptr) {
				if (logger != nullptr) logger->Error("TimelineCurveDrawer::DrawObject - Unsupported Serializer Type!");
				return false;
			}
			FloatingPointCurve* curve = (FloatingPointCurve*)object.TargetAddr();
			if (curve == nullptr) {
				if (logger != nullptr) logger->Error("TimelineCurveDrawer::DrawObject - NULL curve provided!");
				return false;
			}

			const std::string plotName = DefaultGuiItemName(object, viewId);

			bool stuffChanged = false;
			if (ImPlot::BeginPlot(plotName.c_str(), ImVec2(-1.0f, 0.0f), ImPlotFlags_CanvasOnly)) {
				struct PointInfo {
					float time = 0.0f;
					BezierNode<float> node;
				};

				static thread_local std::vector<PointInfo> nodes;
				static thread_local int nodeIdSign = 1;
				static const constexpr int CREATE_OR_REMOVE_BUTTON = 2;

				nodes.clear();
				for (auto it = curve->begin(); it != curve->end(); ++it) {
					PointInfo info = { it->first, it->second };
					
					const constexpr float DRAG_POINT_SIZE = 4.0f;

					// Move point:
					ImPlotPoint nodePosition = { info.time, info.node.Value() };
					if (ImPlot::DragPoint(static_cast<int>(nodes.size()) * nodeIdSign, &nodePosition.x, &nodePosition.y, ImVec4(0.0f, 1.0f, 0.0f, 1.0f), DRAG_POINT_SIZE)) {
						info.time = static_cast<float>(nodePosition.x);
						info.node.Value() = static_cast<float>(nodePosition.y);
						stuffChanged = true;
					}

					// Delete point if we have corresponding input:
					else if (ImGui::IsMouseClicked(CREATE_OR_REMOVE_BUTTON) && ImGui::IsWindowFocused()) {
						const ImVec2 mousePos = ImPlot::PlotToPixels(ImPlot::GetPlotMousePos());
						const ImVec2 nodePos = ImPlot::PlotToPixels(nodePosition);
						float distance = Math::Magnitude(Vector2(mousePos.x - nodePos.x, mousePos.y - nodePos.y));
						if (distance <= DRAG_POINT_SIZE) {
							stuffChanged = true;
							continue;
						}
					}

					// __TODO__: Move 'left handle'
					
					// __TODO__: Move 'right handle'

					nodes.push_back(info);
				}

				bool nodeMoved = false;

				if (stuffChanged) {
					curve->clear();
					for (size_t i = 0; i < nodes.size(); i++) {
						const PointInfo& point = nodes[i];
						curve->operator[](point.time) = point.node;
					}
					if (curve->size() != nodes.size()) {
						nodeIdSign *= -1;
					}
				}
				else if (ImPlot::IsPlotHovered() && ImGui::IsItemClicked(CREATE_OR_REMOVE_BUTTON)) {
					ImPlotPoint point = ImPlot::GetPlotMousePos();
					curve->operator[](static_cast<float>(point.x)) = BezierNode<float>(static_cast<float>(point.y));
					nodeIdSign *= -1;
				}


				{
					static thread_local std::vector<ImPlotPoint> shape;
					shape.clear();

					for (size_t i = 1u; i < nodes.size(); i++) {
						const PointInfo a = nodes[i - 1u];
						const PointInfo b = nodes[i];
						static const constexpr float step = 1.0f / 32.0f;
						float t = 0.0f;
						while (true) {
							float time = Math::Lerp(a.time, b.time, t);
							float value = BezierNode<float>::Interpolate(a.node, b.node, t);
							shape.push_back({ time, value });
							if (t >= 1.0f) break;
							t += step;
							if (t > 1.0f) t = 1.0f;
						}
					}

					if (shape.size() > 0u) {
						ImPlot::SetNextLineStyle(ImVec4(0, 0.9f, 0, 1), 2);
						ImPlot::PlotLine("##shape", &shape[0].x, &shape[0].y, static_cast<int>(shape.size()), 0, 0, sizeof(ImPlotPoint));
					}

					shape.clear();
				}

				nodes.clear();
				ImPlot::EndPlot();
			}

			return stuffChanged;
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::TimelineCurveDrawer>() {
		Editor::MainTimelineCurveDrawer()->Register(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<TimelineCurve<float>::EditableTimelineCurveAttribute>());
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::TimelineCurveDrawer>() {
		Editor::MainTimelineCurveDrawer()->Unregister(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<TimelineCurve<float>::EditableTimelineCurveAttribute>());
	}
}
