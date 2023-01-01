#include "TimelineCurveDrawer.h"
#include "../DrawTooltip.h"
#include "../DrawSerializedObject.h"
#include "Math/Curves.h"


namespace Jimara {
	namespace Editor {
		namespace {
			static const constexpr int CREATE_CURVE_NODE_BUTTON = 0;
			static const constexpr int EDIT_CURVE_NODE_BUTTON = 1;
			static const constexpr int REMOVE_CURVE_NODE_BUTTON = 2;
			
			static const constexpr ImVec4 CURVE_SHAPE_COLOR = ImVec4(0, 1.0f, 0, 1);
			static const constexpr float CURVE_VERTEX_DRAG_SIZE = 4.0f;
			static const constexpr float CURVE_LINE_THICKNESS = 2.0f;

			static const constexpr ImVec4 CURVE_HANDLE_COLOR = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
			static const constexpr float CURVE_HANDLE_SIZE = 48.0f;
			static const constexpr float CURVE_HANDLE_DRAG_SIZE = 3.0f;
			static const constexpr float CURVE_HANDLE_LINE_THICKNESS = 1.0f;

			typedef std::map<float, BezierNode<float>> FloatingPointCurve;

			inline static const TimelineCurveDrawer* MainTimelineCurveDrawer() {
				static const Reference<TimelineCurveDrawer> drawer = Object::Instantiate<TimelineCurveDrawer>();
				return drawer;
			}

			struct FloatCurvePointInfo {
				float time = 0.0f;
				BezierNode<float> node;
			};

			inline static bool MoveCurveVerts(FloatingPointCurve* curve, size_t& nodeIndex) {
				static thread_local std::vector<FloatCurvePointInfo> nodes;
				static thread_local int nodeIdSign = 1;

				bool stuffChanged = false;
				nodes.clear();

				for (auto it = curve->begin(); it != curve->end(); ++it) {
					FloatCurvePointInfo info = { it->first, it->second };

					// Move point:
					ImPlotPoint nodePosition = { info.time, info.node.Value() };
					if (ImPlot::DragPoint(
						static_cast<int>(nodeIndex) * nodeIdSign,
						&nodePosition.x, &nodePosition.y, CURVE_SHAPE_COLOR, CURVE_VERTEX_DRAG_SIZE)) {
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
						if (distance <= CURVE_VERTEX_DRAG_SIZE) {
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
				if (std::abs(HANDLE_LENGTH.x) <= std::numeric_limits<float>::epsilon() ||
					std::abs(HANDLE_LENGTH.y) <= std::numeric_limits<float>::epsilon()) return false;

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
							CURVE_HANDLE_COLOR, CURVE_HANDLE_DRAG_SIZE)) {
							setDirection(
								(static_cast<float>(handleControl.x) - cur->first) / Math::Max(HANDLE_LENGTH.x, std::numeric_limits<float>::epsilon()),
								(static_cast<float>(handleControl.y) - cur->second.Value()) / Math::Max(HANDLE_LENGTH.y, std::numeric_limits<float>::epsilon()));
							float multiplier = (std::abs(direction.x) > std::numeric_limits<float>::epsilon()) ? (1.0f / std::abs(direction.x)) : 1.0f;
							handle = std::abs(delta.x) * multiplier * direction.y;
							stuffChanged = true;
						}
						nodeIndex++;

						// Draw handle:
						ImPlot::SetNextLineStyle(CURVE_HANDLE_COLOR, CURVE_HANDLE_LINE_THICKNESS);
						ImPlot::PlotLine("##handle", &line[0].x, &line[0].y, 2, 0, 0, sizeof(ImPlotPoint));
					};

					// Draw handle controls:
					auto getDeltaX = [&](const auto& it) -> float {
						if (it == curve->end()) return 1.0f;
						else return std::abs(it->first - cur->first) / 3.0f * HANDLE_LENGTH.y / HANDLE_LENGTH.x;
					};
					if (cur->second.IndependentHandles()) {
						drawHandle(cur->second.PrevHandle(), -getDeltaX(prev));
						drawHandle(cur->second.NextHandle(), getDeltaX(next));
					}
					else {
						// Calculate mean value of 'time' differences between the previous and next nodes:
						const float avgDeltaX = [&]() {
							float deltaX = 1.0f;
							float weight = 0.0f;
							auto incorporate = [&](const auto& it) {
								if (it == curve->end()) return;
								weight++;
								deltaX = Math::Lerp(deltaX, getDeltaX(it), 1.0f / weight);
							};
							incorporate(prev);
							incorporate(next);
							return deltaX;
						}();
						drawHandle(cur->second.PrevHandle(), -avgDeltaX);
						drawHandle(cur->second.NextHandle(), avgDeltaX);
					}

					prev = cur;
				}

				return stuffChanged;
			}


			inline static bool DrawContextMenu(FloatingPointCurve* curve, size_t viewId, OS::Logger* logger) {
				const constexpr std::string_view POPUP_NAME = "Jimara-Editor_TimelineCurveDrawer_EditNodesValues";

				static thread_local FloatingPointCurve* lastCurve = nullptr;
				static thread_local size_t lastViewId = ~size_t(0u);
				static thread_local FloatCurvePointInfo contextMenuItem;
				static thread_local float lastItemTime = 0.0f;

				// Check if we can edit any of the points:
				if (lastCurve == nullptr 
					&& ImGui::IsMouseClicked(EDIT_CURVE_NODE_BUTTON) 
					&& (ImGui::IsWindowFocused() || ImGui::IsWindowHovered()) 
					&& curve->size() > 0u) {
					const ImVec2 mousePos = ImPlot::PlotToPixels(ImPlot::GetPlotMousePos());
					
					for (auto it = curve->begin(); it != curve->end(); ++it) {
						// Check if cursor is close enough to the node:
						FloatCurvePointInfo info = { it->first, it->second };
						const ImVec2 nodePos = ImPlot::PlotToPixels({ info.time, info.node.Value() });
						float distance = Math::Magnitude(Vector2(mousePos.x - nodePos.x, mousePos.y - nodePos.y));
						if (distance <= CURVE_VERTEX_DRAG_SIZE) {
							contextMenuItem = info;
							lastItemTime = info.time;
							lastCurve = curve;
							lastViewId = viewId;
							ImGui::OpenPopup(POPUP_NAME.data());
							break;
						}
					}
				}

				if (lastCurve == nullptr || lastCurve != curve || lastViewId != viewId)
					return false;

				// Draw actual popup:
				bool modified = false;
				if (ImGui::BeginPopup(POPUP_NAME.data())) {

					// Serialized object drawing:
					auto inspectElement = [&](const Serialization::SerializedObject& object) {
						auto inspectObjectPtrSerializer = [&](const Serialization::SerializedObject&) {
							if (logger != nullptr) 
								logger->Error("TimelineCurveDrawer::DrawContextMenu - No object pointers expected! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return false;
						};
						if (DrawSerializedObject(object, viewId, logger, Function<bool, const Serialization::SerializedObject&>::FromCall(&inspectObjectPtrSerializer)))
							modified = true;
					};

					// Draw time field:
					{
						static const auto serializer = Serialization::FloatSerializer::Create("Time", "'Time' on the timeline");
						inspectElement(serializer->Serialize(contextMenuItem.time));
					}

					// Draw all node parameters:
					{
						static const BezierNode<float>::Serializer serializer;
						serializer.GetFields(Callback<Serialization::SerializedObject>::FromCall(&inspectElement), &contextMenuItem.node);
					}

					// Draw 'Remove' Button:
					if (ImGui::Button("Remove##Jimara-Editor_TimelineCurveDrawer_EditNodesValues_REMOVE_BUTTON")) {
						modified = true;
						lastCurve = nullptr;
					}

					// Update mapping if modified:
					if (modified) {
						curve->erase(lastItemTime);
						if (lastCurve != nullptr)
							curve->operator[](contextMenuItem.time) = contextMenuItem.node;
						lastItemTime = contextMenuItem.time;
					}

					ImGui::EndPopup();
				}
				else lastCurve = nullptr;

				return modified;
			}

			inline static bool AddNewNode(FloatingPointCurve* curve) {
				// Add node if we have a 'free double-click' on the plot:
				if (ImPlot::IsPlotHovered() 
					&& ImGui::IsItemClicked(CREATE_CURVE_NODE_BUTTON) 
					&& ImGui::IsMouseDoubleClicked(CREATE_CURVE_NODE_BUTTON)) {
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

				// Calculate plot position:
				const ImVec2 plotSize = ImPlot::GetPlotSize();
				const ImVec2 plotPos = ImPlot::GetPlotPos();
				const ImPlotPoint plotRangeStart = ImPlot::PixelsToPlot({ plotPos.x, plotPos.y + plotSize.y });
				const ImPlotPoint plotRangeEnd = ImPlot::PixelsToPlot({ plotPos.x + plotSize.x, plotPos.y });

				// Construct shape:
				const float step = (plotRangeEnd.x - plotRangeStart.x) / Math::Max(plotSize.x, 1.0f);
				for (float time = plotRangeStart.x; time <= plotRangeEnd.x; time += step)
					shape.push_back({ time, TimelineCurve<float>::Value(*curve, time) });

				// Draw shape:
				if (shape.size() > 0u) {
					ImPlot::SetNextLineStyle(CURVE_SHAPE_COLOR, 2);
					ImPlot::PlotLine("##shape", &shape[0].x, &shape[0].y, static_cast<int>(shape.size()), 0, 0, sizeof(ImPlotPoint));
				}

				shape.clear();
			}

			inline static bool EditCurve(FloatingPointCurve* curve, size_t viewId, OS::Logger* logger) {
				size_t nodeIndex = 0u;
				bool stuffChanged = MoveCurveHandles(curve, nodeIndex);
				if (MoveCurveVerts(curve, nodeIndex)) stuffChanged = true;
				if (DrawContextMenu(curve, viewId, logger)) stuffChanged = true;
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
			if (ImPlot::BeginPlot(plotName.c_str(), ImVec2(-1.0f, 0.0f), ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText)) {
				rv = EditCurve(curve, viewId, logger);
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
