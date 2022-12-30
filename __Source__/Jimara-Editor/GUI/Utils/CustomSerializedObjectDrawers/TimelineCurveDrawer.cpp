#include "TimelineCurveDrawer.h"
#include "../DrawTooltip.h"
#include "Math/Curves.h"


namespace Jimara {
	namespace Editor {
		namespace {
			typedef std::map<float, BezierNode<float>> FloatingPointCurve;

			inline static const TimelineCurveDrawer* MainTimelineCurveDrawer() {
				static const Reference<TimelineCurveDrawer> drawer = Object::Instantiate<TimelineCurveDrawer>();
				return drawer;
			}

			struct FloatCurvePointInfo {
				float time = 0.0f;
				BezierNode<float> node;
			};


			static const constexpr int CREATE_CURVE_NODE_BUTTON = 0;
			static const constexpr int REMOVE_CURVE_NODE_BUTTON = 2;
			static const constexpr float CURVE_HANDLE_SIZE = 64.0f;
			static const constexpr float CURVE_HANDLE_DRAG_SIZE = 3.0f;

			inline static bool MoveCurveVerts(FloatingPointCurve* curve, size_t& nodeIndex) {
				static thread_local std::vector<FloatCurvePointInfo> nodes;
				static thread_local int nodeIdSign = 1;

				bool stuffChanged = false;
				nodes.clear();

				for (auto it = curve->begin(); it != curve->end(); ++it) {
					FloatCurvePointInfo info = { it->first, it->second };

					const constexpr float DRAG_POINT_SIZE = 4.0f;

					// Move point:
					ImPlotPoint nodePosition = { info.time, info.node.Value() };
					if (ImPlot::DragPoint(
						static_cast<int>(nodeIndex) * nodeIdSign,
						&nodePosition.x, &nodePosition.y, ImVec4(0.0f, 1.0f, 0.0f, 1.0f), DRAG_POINT_SIZE)) {
						info.time = static_cast<float>(nodePosition.x);
						info.node.Value() = static_cast<float>(nodePosition.y);
						stuffChanged = true;
					}
					nodeIndex++;

					// Delete point if we have corresponding input:
					if (ImGui::IsMouseClicked(REMOVE_CURVE_NODE_BUTTON) && ImGui::IsWindowFocused()) {
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

				// Update original curve if dirty:
				if (stuffChanged) {
					curve->clear();
					for (size_t i = 0; i < nodes.size(); i++) {
						const FloatCurvePointInfo& point = nodes[i];
						curve->operator[](point.time) = point.node;
					}
					if (curve->size() != nodes.size()) {
						nodeIdSign *= -1;
					}
				}

				nodes.clear();
				return stuffChanged;
			}

			inline static bool MoveCurveHandles(FloatingPointCurve* curve, size_t& nodeIndex) {
				// Calculate scaled handle size:
				const Vector2 HANDLE_LENGTH = [&]() -> Vector2 {
					ImPlotPoint a = ImPlot::PixelsToPlot({ 0.0f, 0.0f });
					ImPlotPoint b = ImPlot::PixelsToPlot({ CURVE_HANDLE_SIZE, CURVE_HANDLE_SIZE });
					return {
						static_cast<float>(std::abs(b.x - a.x)),
						static_cast<float>(std::abs(b.y - a.y))
					};
				}();

				bool stuffChanged = false;
				auto prev = curve->end();
				auto next = curve->begin();
				while (next != curve->end()) {
					auto cur = next;
					++next;

					// Line that will be drawn:
					ImPlotPoint line[2];
					line[0] = { cur->first, cur->second.Value() };

					auto drawHandle = [&](Property<float> handle, float deltaX) {
						const Vector2 delta(deltaX, handle);

						// Normalized handle 'directon':
						Vector2 direction;
						auto setDirection = [&](float dx, float dy) {
							direction =
								(std::isfinite(dx) && std::isfinite(dy)) ? Math::Normalize(Vector2(dx, dy)) :
								std::isfinite(dx) ? Vector2(0.0f, dy > 0.0f ? 1.0f : -1.0f) :
								Vector2(dx >= 0.0f ? 1.0f : -1.0f, 0.0f);
						};
						setDirection(delta.x, delta.y);

						// Handle endpoint position on graph:
						ImPlotPoint& handleControl = line[1];
						handleControl = {
							static_cast<double>(direction.x) * HANDLE_LENGTH.x + cur->first,
							static_cast<double>(direction.y) * HANDLE_LENGTH.y + cur->second.Value()
						};

						// Drag handle:
						if (ImPlot::DragPoint(
							static_cast<int>(nodeIndex),
							&handleControl.x, &handleControl.y,
							ImVec4(0.5f, 0.5f, 0.5f, 1.0f), CURVE_HANDLE_DRAG_SIZE)) {
							setDirection(
								(static_cast<float>(handleControl.x) - cur->first) / Math::Max(HANDLE_LENGTH.x, std::numeric_limits<float>::epsilon()),
								(static_cast<float>(handleControl.y) - cur->second.Value()) / Math::Max(HANDLE_LENGTH.y, std::numeric_limits<float>::epsilon()));
							float multiplier = (std::abs(direction.x) > std::numeric_limits<float>::epsilon()) ? (1.0f / std::abs(direction.x)) : 1.0f;
							handle = std::abs(delta.x) * multiplier * direction.y;
							stuffChanged = true;
						}
						nodeIndex++;

						// Draw handle:
						ImPlot::SetNextLineStyle(ImVec4(0.25f, 0.25f, 0.25f, 1.0f), 1);
						ImPlot::PlotLine("##handle", &line[0].x, &line[0].y, 2, 0, 0, sizeof(ImPlotPoint));
					};

					// Calculate mean value of 'time' differences between the previous and next nodes:
					float avgDeltaX = [&]() {
						float deltaX = 1.0f;
						if (prev != curve->end()) deltaX = (cur->first - prev->first);
						if (next != curve->end()) deltaX = (deltaX + (next->first - cur->first)) * 0.5f;
						return deltaX;
					}();

					// Draw handle controls:
					drawHandle(cur->second.PrevHandle(), -avgDeltaX);
					drawHandle(cur->second.NextHandle(), avgDeltaX);

					prev = cur;
				}

				return stuffChanged;
			}

			inline static bool AddNewNode(FloatingPointCurve* curve) {
				// Add node if we have a 'free click' on the plot:
				if (ImPlot::IsPlotHovered() && ImGui::IsItemClicked(CREATE_CURVE_NODE_BUTTON)) {
					ImPlotPoint point = ImPlot::GetPlotMousePos();
					curve->operator[](static_cast<float>(point.x)) = BezierNode<float>(static_cast<float>(point.y));
					return true;
				}
				else return false;
			}

			inline static void DrawCurve(FloatingPointCurve* curve) {
				if (curve->size() <= 0u) return;
				static thread_local std::vector<ImPlotPoint> shape;
				shape.clear();

				// Construct shape:
				auto next = curve->begin();
				while (true) {
					auto cur = next;
					++next;
					if (next == curve->end()) break;
					static const constexpr float step = 1.0f / 32.0f;
					float t = 0.0f;
					while (true) {
						float time = Math::Lerp(cur->first, next->first, t);
						float value = BezierNode<float>::Interpolate(cur->second, next->second, t);
						shape.push_back({ time, value });
						if (t >= 1.0f) break;
						t += step;
						if (t > 1.0f) t = 1.0f;
					}
				}

				// Draw shape:
				if (shape.size() > 0u) {
					ImPlot::SetNextLineStyle(ImVec4(0, 0.9f, 0, 1), 2);
					ImPlot::PlotLine("##shape", &shape[0].x, &shape[0].y, static_cast<int>(shape.size()), 0, 0, sizeof(ImPlotPoint));
				}

				shape.clear();
			}

			inline static bool EditCurve(FloatingPointCurve* curve) {
				size_t nodeIndex = 0u;
				bool stuffChanged = MoveCurveVerts(curve, nodeIndex);
				if (MoveCurveHandles(curve, nodeIndex)) stuffChanged = true;
				if (!stuffChanged) stuffChanged = AddNewNode(curve);
				DrawCurve(curve);

				static thread_local const FloatingPointCurve* lastTargetAddr = nullptr;
				const bool isSameObject = (lastTargetAddr == curve);

				if (stuffChanged) 
					lastTargetAddr = curve;

				if (!ImGui::IsAnyItemActive())
					lastTargetAddr = nullptr;

				return (isSameObject || stuffChanged) && (lastTargetAddr == nullptr);
			}
		}

		bool TimelineCurveDrawer::DrawObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const {
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

			bool rv = false;
			if (ImPlot::BeginPlot(plotName.c_str(), ImVec2(-1.0f, 0.0f), ImPlotFlags_CanvasOnly)) {
				rv = EditCurve(curve);
				ImPlot::EndPlot();
			}

			return rv;
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::TimelineCurveDrawer>() {
		Editor::MainTimelineCurveDrawer()->Register(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<TimelineCurve<float>::EditableTimelineCurveAttribute>());
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::TimelineCurveDrawer>() {
		Editor::MainTimelineCurveDrawer()->Unregister(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<TimelineCurve<float>::EditableTimelineCurveAttribute>());
	}
}
