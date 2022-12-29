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
				static const constexpr int CREATE_BUTTON = 0;
				static const constexpr int REMOVE_BUTTON = 2;

				nodes.clear();
				for (auto it = curve->begin(); it != curve->end(); ++it) {
					PointInfo info = { it->first, it->second };
					
					const constexpr float DRAG_POINT_SIZE = 4.0f;

					// Move point:
					ImPlotPoint nodePosition = { info.time, info.node.Value() };
					if (ImPlot::DragPoint(
						static_cast<int>(nodes.size()) * nodeIdSign, 
						&nodePosition.x, &nodePosition.y, ImVec4(0.0f, 1.0f, 0.0f, 1.0f), DRAG_POINT_SIZE)) {
						info.time = static_cast<float>(nodePosition.x);
						info.node.Value() = static_cast<float>(nodePosition.y);
						stuffChanged = true;
					}

					// Delete point if we have corresponding input:
					else if (ImGui::IsMouseClicked(REMOVE_BUTTON) && ImGui::IsWindowFocused()) {
						const ImVec2 mousePos = ImPlot::PlotToPixels(ImPlot::GetPlotMousePos());
						const ImVec2 nodePos = ImPlot::PlotToPixels(nodePosition);
						float distance = Math::Magnitude(Vector2(mousePos.x - nodePos.x, mousePos.y - nodePos.y));
						if (distance <= DRAG_POINT_SIZE) {
							stuffChanged = true;
							continue;
						}
					}

					nodes.push_back(info);
				}

				// Draw handles:

				const constexpr float HANDLE_SIZE = 64.0f;
				const Vector2 HANDLE_LENGTH = [&]() -> Vector2 {
					ImPlotPoint a = ImPlot::PixelsToPlot({ 0.0f, 0.0f });
					ImPlotPoint b = ImPlot::PixelsToPlot({ HANDLE_SIZE, HANDLE_SIZE });
					return {
						static_cast<float>(std::abs(b.x - a.x)),
						static_cast<float>(std::abs(b.y - a.y))
					};
				}();
				const constexpr float HANDLE_DRAG_SIZE = 3.0f;

				for (size_t i = 0u; i < nodes.size(); i++) {
					PointInfo& node = nodes[i];
					
					ImPlotPoint line[2];
					line[0] = { node.time, node.node.Value() };
					
					auto drawHandle = [&](Property<float> handle, size_t index, size_t indexMultiplier) {
						Vector2 direction;

						auto setDirection = [&](float dx, float dy) {
							direction =
								(std::isfinite(dx) && std::isfinite(dy)) ? Math::Normalize(Vector2(dx, dy)) :
								std::isfinite(dx) ? Vector2(0.0f, dy > 0.0f ? 1.0f : -1.0f) :
								Vector2(dx >= 0.0f ? 1.0f : -1.0f, 0.0f);
						};

						const Vector2 delta(
							(index == (size_t(0u) - 1u)) ? -1.0f :
							(index >= nodes.size()) ? 1.0f :
							((nodes[index].time - node.time) / 3.0f), handle);

						setDirection(delta.x, delta.y);

						ImPlotPoint& handleControl = line[1];
						handleControl = {
							static_cast<double>(direction.x) * HANDLE_LENGTH.x + node.time,
							static_cast<double>(direction.y) * HANDLE_LENGTH.y + node.node.Value()
						};
						
						
						if (ImPlot::DragPoint(
							static_cast<int>(curve->size() * indexMultiplier + i) * nodeIdSign,
							&handleControl.x, &handleControl.y,
							ImVec4(0.5f, 0.5f, 0.5f, 1.0f), HANDLE_DRAG_SIZE)) {
							setDirection(
								(static_cast<float>(handleControl.x) - node.time) / Math::Max(HANDLE_LENGTH.x, std::numeric_limits<float>::epsilon()),
								(static_cast<float>(handleControl.y) - node.node.Value()) / Math::Max(HANDLE_LENGTH.y, std::numeric_limits<float>::epsilon()));
							float multiplier = (std::abs(direction.x) > std::numeric_limits<float>::epsilon()) ? (1.0f / std::abs(direction.x)) : 1.0f;
							handle = std::abs(delta.x) * multiplier * direction.y;
							stuffChanged = true;
						}

						ImPlot::SetNextLineStyle(ImVec4(0.25f, 0.25f, 0.25f, 1.0f), 1);
						ImPlot::PlotLine("##shape", &line[0].x, &line[0].y, 2, 0, 0, sizeof(ImPlotPoint));
					};

					drawHandle(node.node.PrevHandle(), i - 1u, 1u);
					drawHandle(node.node.NextHandle(), i + 1u, 2u);
				}

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
				else if (ImPlot::IsPlotHovered() && ImGui::IsItemClicked(CREATE_BUTTON)) {
					ImPlotPoint point = ImPlot::GetPlotMousePos();
					curve->operator[](static_cast<float>(point.x)) = BezierNode<float>(static_cast<float>(point.y));
					nodeIdSign *= -1;
					stuffChanged = true;
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

			static thread_local const Serialization::ItemSerializer* lastSerializer = nullptr;
			static thread_local const void* lastTargetAddr = nullptr;

			const bool isSameObject = (object.Serializer() == lastSerializer && object.TargetAddr() == lastTargetAddr);
			if (stuffChanged) {
				lastSerializer = object.Serializer();
				lastTargetAddr = object.TargetAddr();
			}

			if (!ImGui::IsAnyItemActive()) {
				lastSerializer = nullptr;
				lastTargetAddr = nullptr;
			}

			return (isSameObject || stuffChanged) && (lastSerializer == nullptr);
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::TimelineCurveDrawer>() {
		Editor::MainTimelineCurveDrawer()->Register(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<TimelineCurve<float>::EditableTimelineCurveAttribute>());
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::TimelineCurveDrawer>() {
		Editor::MainTimelineCurveDrawer()->Unregister(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<TimelineCurve<float>::EditableTimelineCurveAttribute>());
	}
}
