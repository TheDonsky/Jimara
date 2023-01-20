#include "TimelineCurveDrawer.h"
#include "../DrawTooltip.h"
#include "../DrawSerializedObject.h"
#include "Math/Curves.h"


namespace Jimara {
	namespace Editor {
		struct TimelineCurveDrawer::Helpers {
			static const constexpr int CREATE_CURVE_NODE_BUTTON = 0;
			static const constexpr int EDIT_CURVE_NODE_BUTTON = 1;
			static const constexpr int REMOVE_CURVE_NODE_BUTTON = 2;
			
			static const constexpr float CURVE_VERTEX_DRAG_SIZE = 4.0f;
			static const constexpr float CURVE_LINE_THICKNESS = 2.0f;

			static const constexpr ImVec4 CURVE_HANDLE_COLOR = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
			static const constexpr float CURVE_HANDLE_SIZE = 48.0f;
			static const constexpr float CURVE_HANDLE_DRAG_SIZE = 3.0f;
			static const constexpr float CURVE_HANDLE_LINE_THICKNESS = 1.0f;

			static const constexpr ImVec4 CURVE_LIMITS_RECT_COLOR = ImVec4(0.35f, 0.35f, 0.35f, 1.0f);
			static const constexpr float CURVE_LIMITS_LINE_THICKNESS = 2.0f;

			template<typename CurveValueType> inline static const constexpr size_t CurveChannelCount() {
				return 
					std::is_same_v<CurveValueType, float> ? 1 :
					std::is_same_v<CurveValueType, Vector2> ? 2 :
					std::is_same_v<CurveValueType, Vector3> ? 3 :
					std::is_same_v<CurveValueType, Vector4> ? 4 : 0;
			}

			template<typename CurveValueType> inline static const ImVec4 CurveShapeColor(size_t channelId) {
				static const ImVec4 colors[] = {
					ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
					ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
					ImVec4(0.0f, 0.0f, 1.0f, 1.0f),
					ImVec4(1.0f, 0.0f, 1.0f, 1.0f)
				};
				return std::is_same_v<CurveValueType, float> ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) : colors[channelId];
			}

			inline static float GetNodeValue(const float& node, size_t channelId) { Unused(channelId); return node; }
			inline static float GetNodeValue(const Vector2& node, size_t channelId) { return node[static_cast<uint8_t>(channelId)]; }
			inline static float GetNodeValue(const Vector3& node, size_t channelId) { return node[static_cast<uint8_t>(channelId)]; }
			inline static float GetNodeValue(const Vector4& node, size_t channelId) { return node[static_cast<uint8_t>(channelId)]; }

			inline static void SetNodeValue(Property<float> node, size_t channelId, float value) { Unused(channelId); node = value; }
			inline static void SetNodeValue(Property<Vector2> node, size_t channelId, float value) { 
				Vector2 vecValue = node; vecValue[static_cast<uint8_t>(channelId)] = value; node = vecValue;
			}
			inline static void SetNodeValue(Property<Vector3> node, size_t channelId, float value) { 
				Vector3 vecValue = node; vecValue[static_cast<uint8_t>(channelId)] = value; node = vecValue;
			}
			inline static void SetNodeValue(Property<Vector4> node, size_t channelId, float value) { 
				Vector4 vecValue = node; vecValue[static_cast<uint8_t>(channelId)] = value; node = vecValue;
			}

			template<typename CurveValueType>
			struct CurvePointInfo {
				float time = 0.0f;
				BezierNode<CurveValueType> node;
			};

			template<typename CurveValueType>
			inline static bool MoveCurveVerts(
				std::map<float, BezierNode<CurveValueType>>* curve, size_t& nodeIndex,
				const Serialization::CurveGraphCoordinateLimits* limits) {

				static thread_local std::vector<CurvePointInfo<CurveValueType>> nodes;
				static thread_local int nodeIdSign = 1;

				bool stuffChanged = false;

				for (size_t channelId = 0u; channelId < CurveChannelCount<CurveValueType>(); channelId++) {
					nodes.clear();
					bool stuffChangedOnThisPass = false;

					for (auto it = curve->begin(); it != curve->end(); ++it) {
						CurvePointInfo<CurveValueType> info = { it->first, it->second };
						
						// Move point:
						ImPlotPoint nodePosition = { info.time, GetNodeValue(info.node.Value(), channelId) };
						if (ImPlot::DragPoint(
							static_cast<int>(nodeIndex) * nodeIdSign,
							&nodePosition.x, &nodePosition.y, CurveShapeColor<CurveValueType>(channelId), CURVE_VERTEX_DRAG_SIZE)) {
							// If we have limits, respect those:
							if (limits != nullptr) {
								nodePosition.x = Math::Min(Math::Max(limits->minT, static_cast<float>(nodePosition.x)), limits->maxT);
								nodePosition.y = Math::Min(Math::Max(limits->minV, static_cast<float>(nodePosition.y)), limits->maxV);
							}
							info.time = static_cast<float>(nodePosition.x);
							SetNodeValue(Property<CurveValueType>(&info.node.Value()), channelId, static_cast<float>(nodePosition.y));
							stuffChangedOnThisPass = true;
						}
						nodeIndex++;

						// Delete point if we have corresponding input:
						if (ImGui::IsMouseClicked(REMOVE_CURVE_NODE_BUTTON) && ImGui::IsWindowFocused()) {
							const ImVec2 mousePos = ImPlot::PlotToPixels(ImPlot::GetPlotMousePos());
							const ImVec2 nodePos = ImPlot::PlotToPixels(nodePosition);
							float distance = Math::Magnitude(Vector2(mousePos.x - nodePos.x, mousePos.y - nodePos.y));
							if (distance <= CURVE_VERTEX_DRAG_SIZE) {
								stuffChangedOnThisPass = true;
								continue;
							}
						}

						nodes.push_back(info);
					}

					// Update original curve if dirty:
					if (stuffChangedOnThisPass) {
						curve->clear();
						for (size_t i = 0; i < nodes.size(); i++) {
							const CurvePointInfo<CurveValueType>& point = nodes[i];
							curve->operator[](point.time) = point.node;
						}
						if (curve->size() != nodes.size()) {
							nodeIdSign *= -1;
						}
						stuffChanged = true;
					}

					nodes.clear();
				}

				return stuffChanged;
			}

			inline static Vector2 GetGraphSize(Vector2 pixelSize) {
				ImPlotPoint a = ImPlot::PixelsToPlot({ 0.0f, 0.0f });
				ImPlotPoint b = ImPlot::PixelsToPlot({ pixelSize.x, pixelSize.y });
				return {
					static_cast<float>(std::abs(b.x - a.x)),
					static_cast<float>(std::abs(b.y - a.y))
				};
			}

			template<typename CurveValueType>
			inline static bool MoveCurveHandles(std::map<float, BezierNode<CurveValueType>>* curve, size_t& nodeIndex) {
				// Calculate scaled handle size:
				const Vector2 HANDLE_LENGTH = GetGraphSize({ CURVE_HANDLE_SIZE , CURVE_HANDLE_SIZE });
				if (std::abs(HANDLE_LENGTH.x) <= std::numeric_limits<float>::epsilon() ||
					std::abs(HANDLE_LENGTH.y) <= std::numeric_limits<float>::epsilon()) return false;

				bool stuffChanged = false;
				for (size_t channelId = 0u; channelId < CurveChannelCount<CurveValueType>(); channelId++) {
					auto prev = curve->end();
					auto next = curve->begin();
					while (next != curve->end()) {
						auto cur = next;
						++next;
						
						// Line that will be drawn:
						ImPlotPoint line[2];
						line[0] = { cur->first, GetNodeValue(cur->second.Value(), channelId) };

						auto drawHandle = [&](Property<CurveValueType> handle, float deltaX) {
							const Vector2 delta(deltaX, GetNodeValue(handle, channelId));

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
								static_cast<double>(direction.y) * HANDLE_LENGTH.y + GetNodeValue(cur->second.Value(), channelId)
							};

							// Drag handle:
							if (ImPlot::DragPoint(
								static_cast<int>(nodeIndex),
								&handleControl.x, &handleControl.y,
								CURVE_HANDLE_COLOR, CURVE_HANDLE_DRAG_SIZE)) {
								setDirection(
									(static_cast<float>(handleControl.x) - cur->first) / Math::Max(HANDLE_LENGTH.x, std::numeric_limits<float>::epsilon()),
									(static_cast<float>(handleControl.y) - GetNodeValue(cur->second.Value(), channelId)) / Math::Max(HANDLE_LENGTH.y, std::numeric_limits<float>::epsilon()));
								float multiplier = (std::abs(direction.x) > std::numeric_limits<float>::epsilon()) ? (1.0f / std::abs(direction.x)) : 1.0f;
								SetNodeValue(handle, channelId, std::abs(delta.x) * multiplier * direction.y);
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
				}

				return stuffChanged;
			}

			template<typename CurveValueType>
			inline static bool DrawContextMenu(
				std::map<float, BezierNode<CurveValueType>>* curve, size_t viewId, 
				const Serialization::CurveGraphCoordinateLimits* limits, OS::Logger* logger) {
				
				const constexpr std::string_view POPUP_NAME = "Jimara-Editor_TimelineCurveDrawer_EditNodesValues";

				static thread_local std::map<float, BezierNode<CurveValueType>>* lastCurve = nullptr;
				static thread_local size_t lastViewId = ~size_t(0u);
				static thread_local CurvePointInfo<CurveValueType> contextMenuItem;
				static thread_local float lastItemTime = 0.0f;

				// Check if we can edit any of the points:
				if (lastCurve == nullptr 
					&& ImGui::IsMouseClicked(EDIT_CURVE_NODE_BUTTON) 
					&& (ImGui::IsWindowFocused() || ImGui::IsWindowHovered()) 
					&& curve->size() > 0u) {
					const ImVec2 mousePos = ImPlot::PlotToPixels(ImPlot::GetPlotMousePos());
					
					for (size_t channelId = 0u; channelId < CurveChannelCount<CurveValueType>(); channelId++) {
						for (auto it = curve->begin(); it != curve->end(); ++it) {
							// Check if cursor is close enough to the node:
							CurvePointInfo<CurveValueType> info = { it->first, it->second };
							const ImVec2 nodePos = ImPlot::PlotToPixels({ info.time, GetNodeValue(info.node.Value(), channelId) });
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
						if (lastCurve != nullptr) break;
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
								logger->Error("TimelineCurveDrawer::Helpers::DrawContextMenu - No object pointers expected! [File: ", __FILE__, "; Line: ", __LINE__, "]");
							return false;
						};
						if (DrawSerializedObject(object, viewId, logger, Function<bool, const Serialization::SerializedObject&>::FromCall(&inspectObjectPtrSerializer)))
							modified = true;
					};

					// Draw time field:
					{
						static const auto serializer = Serialization::FloatSerializer::Create("Time", "'Time' on the timeline");
						inspectElement(serializer->Serialize(contextMenuItem.time));
						if (limits != nullptr) contextMenuItem.time = Math::Min(Math::Max(limits->minT, contextMenuItem.time), limits->maxT);
					}

					// Draw all node parameters:
					{
						static const typename BezierNode<CurveValueType>::Serializer serializer;
						serializer.GetFields(Callback<Serialization::SerializedObject>::FromCall(&inspectElement), &contextMenuItem.node);
						if (limits != nullptr) for (size_t channelId = 0u; channelId < CurveChannelCount<CurveValueType>(); channelId++)
							SetNodeValue(Property<CurveValueType>(&contextMenuItem.node.Value()), channelId,
								Math::Min(Math::Max(limits->minV, GetNodeValue(contextMenuItem.node.Value(), channelId)), limits->maxV));
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

			template<typename CurveValueType>
			inline static bool AddNewNode(std::map<float, BezierNode<CurveValueType>>* curve) {
				// Add node if we have a 'free double-click' on the plot:
				if (ImPlot::IsPlotHovered() 
					&& ImGui::IsItemClicked(CREATE_CURVE_NODE_BUTTON) 
					&& ImGui::IsMouseDoubleClicked(CREATE_CURVE_NODE_BUTTON)) {
					ImPlotPoint point = ImPlot::GetPlotMousePos();
					curve->operator[](static_cast<float>(point.x)) = BezierNode<CurveValueType>(CurveValueType(static_cast<float>(point.y)));
					return true;
				}
				else return false;
			}

			inline static Rect GetPlotRect() {
				const ImVec2 plotSize = ImPlot::GetPlotSize();
				const ImVec2 plotPos = ImPlot::GetPlotPos();
				const ImPlotPoint rangeStart = ImPlot::PixelsToPlot({ plotPos.x, plotPos.y + plotSize.y });
				const ImPlotPoint rangeEnd = ImPlot::PixelsToPlot({ plotPos.x + plotSize.x, plotPos.y });
				return Rect(
					Vector2(static_cast<float>(rangeStart.x), static_cast<float>(rangeStart.y)),
					Vector2(static_cast<float>(rangeEnd.x), static_cast<float>(rangeEnd.y)));
			}

			template<typename CurveValueType>
			inline static void DrawCurve(std::map<float, BezierNode<CurveValueType>>* curve) {
				if (curve->size() <= 0u) return;
				static thread_local std::vector<ImPlotPoint> shape;
				static thread_local std::vector<std::pair<float, CurveValueType>> curveShape;

				// Calculate plot position:
				const Rect plotRect = GetPlotRect();
				const float plotRangeStart = plotRect.start.x;
				const float plotRangeEnd = plotRect.end.x;
				const float plotWidth = ImPlot::GetPlotSize().x;

				curveShape.clear();
				if (curve->empty()) {
					// If the curve is empty, we just draw a flat line:
					curveShape.push_back({ plotRangeStart, CurveValueType(0.0f) });
					curveShape.push_back({ plotRangeEnd, CurveValueType(0.0f) });
				}
				else {
					// 'Establish' iterators:
					auto it = curve->lower_bound(plotRangeStart);
					float at = plotRangeStart;
					float bt = plotRangeStart;
					BezierNode<CurveValueType> a;
					BezierNode<CurveValueType> b;
					bool endReached;
					bool startNotReached = (it == curve->begin());
					if (it != curve->end()) {
						bt = it->first;
						b = it->second;
						endReached = false;
						if (!startNotReached) {
							auto prev = it;
							prev--;
							at = prev->first;
							a = prev->second;
						}
					}
					else {
						bt = curve->rbegin()->first;
						b = curve->rbegin()->second;
						endReached = true;
					}

					// Construct shape:
					const float step = (plotRangeEnd - plotRangeStart) / Math::Max(plotWidth, 1.0f);
					for (float time = plotRangeStart; time <= plotRangeEnd; time += step) {
						if (!endReached)
							while (bt < time) {
								at = bt;
								a = b;
								it++;
								startNotReached = false;
								if (it != curve->end()) {
									bt = it->first;
									b = it->second;
								}
								else {
									endReached = true;
									break;
								}
							}
						const CurveValueType value = (endReached || startNotReached)
							? b.Value()
							: BezierNode<CurveValueType>::Interpolate(a, b, (time - at) / (bt - at));
						curveShape.push_back({ time, value });
					}
				}

				// Draw curve per channel:
				for (size_t channelId = 0u; channelId < CurveChannelCount<CurveValueType>(); channelId++) {
					shape.clear();

					// Construct shape:
					const std::pair<float, CurveValueType>* ptr = curveShape.data();
					const std::pair<float, CurveValueType>* end = ptr + curveShape.size();
					while (ptr < end) {
						shape.push_back({ ptr->first, GetNodeValue(ptr->second, channelId) });
						ptr++;
					}

					// Draw shape:
					if (shape.size() > 0u) {
						ImPlot::SetNextLineStyle(CurveShapeColor<CurveValueType>(channelId), 2);
						ImPlot::PlotLine("##shape", &shape[0].x, &shape[0].y, static_cast<int>(shape.size()), 0, 0, sizeof(ImPlotPoint));
					}

					shape.clear();
				}

				curveShape.clear();
			}

			template<typename CurveValueType>
			inline static bool EditCurve(
				std::map<float, BezierNode<CurveValueType>>* curve, size_t viewId, OS::Logger* logger, 
				const Serialization::CurveGraphCoordinateLimits* limits) {
				
				size_t nodeIndex = 0u;
				bool stuffChanged = MoveCurveHandles(curve, nodeIndex);
				if (MoveCurveVerts(curve, nodeIndex, limits)) stuffChanged = true;
				if (DrawContextMenu(curve, viewId, limits, logger)) stuffChanged = true;
				if (!stuffChanged) stuffChanged = AddNewNode(curve);
				DrawCurve(curve);

				static thread_local const std::map<float, BezierNode<CurveValueType>>* lastTargetAddr = nullptr;
				const bool isSameObject = (lastTargetAddr == curve);

				if (stuffChanged)
					lastTargetAddr = curve;

				if (!ImGui::IsAnyItemActive())
					lastTargetAddr = nullptr;

				return (isSameObject || stuffChanged) && (lastTargetAddr == nullptr);
			}


			inline static const Serialization::CurveGraphCoordinateLimits* SetupAxisLimits(const Serialization::SerializedObject& object) {
				const Serialization::CurveGraphCoordinateLimits* limits = object.Serializer()->FindAttributeOfType<Serialization::CurveGraphCoordinateLimits>();
				if (limits == nullptr) {
					static const Serialization::CurveGraphCoordinateLimits noLimits;
					return &noLimits;
				}

				// Setup limits:
				{
					auto hasLockFlag = [&](Serialization::CurveGraphCoordinateLimits::LockFlags flag, float rangeMin, float rangeMax) {
						if (std::isinf(rangeMin) || std::isinf(rangeMax)) return false;
						typedef std::underlying_type_t<Serialization::CurveGraphCoordinateLimits::LockFlags> Flags;
						return (static_cast<Flags>(limits->lockFlags) & static_cast<Flags>(flag)) == static_cast<Flags>(flag);
					};
					auto setupAxisLimits = [&](Serialization::CurveGraphCoordinateLimits::LockFlags flag, float rangeMin, float rangeMax, ImAxis axis) {
						const float usableAreaOffset =
							((!std::isinf(rangeMin)) && (!std::isinf(rangeMax))) ? ((rangeMax - rangeMin) * 0.05f) :
							((!std::isinf(rangeMin)) || (!std::isinf(rangeMax))) ? 0.25f : 0.0f;
						const float rngMin = std::isinf(rangeMin) ? rangeMin : (rangeMin - usableAreaOffset);
						const float rngMax= std::isinf(rangeMax) ? rangeMax : (rangeMax + usableAreaOffset);
						if (hasLockFlag(flag, rangeMin, rangeMax))
							ImPlot::SetupAxisLimits(axis, rngMin, rngMax, ImPlotCond_Always);
						else ImPlot::SetupAxisLimitsConstraints(axis, rngMin, rngMax);
						
					};
					setupAxisLimits(Serialization::CurveGraphCoordinateLimits::LockFlags::LOCK_ZOOM_X, limits->minT, limits->maxT, ImAxis_X1);
					setupAxisLimits(Serialization::CurveGraphCoordinateLimits::LockFlags::LOCK_ZOOM_Y, limits->minV, limits->maxV, ImAxis_Y1);
				}

				// Draw limits:
				{
					auto drawLine = [&](float x0, float y0, float x1, float y1) {
						const double x[] = { x0, x1 };
						const double y[] = { y0, y1 };
						ImPlot::SetNextLineStyle(CURVE_LIMITS_RECT_COLOR, CURVE_LIMITS_LINE_THICKNESS);
						ImPlot::PlotLine("##boundary", x, y, 2);
					};
					const Rect plotRect = GetPlotRect();
					const float xStart = std::isinf(limits->minT) ? plotRect.start.x : limits->minT;
					const float xEnd = std::isinf(limits->maxT) ? plotRect.end.x : limits->maxT;
					const float yStart = std::isinf(limits->minV) ? plotRect.start.y : limits->minV;
					const float yEnd = std::isinf(limits->maxV) ? plotRect.end.y : limits->maxV;
					if (!std::isinf(limits->minT)) drawLine(limits->minT, yStart, limits->minT, yEnd);
					if (!std::isinf(limits->maxT)) drawLine(limits->maxT, yStart, limits->maxT, yEnd);
					if (!std::isinf(limits->minV)) drawLine(xStart, limits->minV, xEnd, limits->minV);
					if (!std::isinf(limits->maxV)) drawLine(xStart, limits->maxV, xEnd, limits->maxV);
				}

				return limits;
			}

			enum class DrawCurveStatus {
				TYPE_MISMATCH,
				TARGET_ERROR,
				CURVE_EDITED,
				CURVE_NOT_EDITED
			};

			template<typename CurveValueType>
			inline static DrawCurveStatus DrawCurve(const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger) {
				typedef std::map<float, BezierNode<CurveValueType>> CurveType;
				if (dynamic_cast<const Serialization::SerializerList::From<CurveType>*>(object.Serializer()) == nullptr)
					return DrawCurveStatus::TYPE_MISMATCH;
				CurveType* curve = (CurveType*)object.TargetAddr();
				if (curve == nullptr) {
					if (logger != nullptr) logger->Error("TimelineCurveDrawer::Helpers::DrawCurve - NULL curve provided!");
					return DrawCurveStatus::TARGET_ERROR;
				}
				bool rv = false;
				const std::string plotName = CustomSerializedObjectDrawer::DefaultGuiItemName(object, viewId);
				if (ImPlot::BeginPlot(plotName.c_str(), ImVec2(-1.0f, 0.0f), ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect | ImPlotFlags_NoMouseText)) {
					rv = EditCurve(curve, viewId, logger, SetupAxisLimits(object));
					ImPlot::EndPlot();
				}
				return rv ? DrawCurveStatus::CURVE_EDITED : DrawCurveStatus::CURVE_NOT_EDITED;
			}
		};

		bool TimelineCurveDrawer::DrawObject(
			const Serialization::SerializedObject& object, size_t viewId, OS::Logger* logger,
			const Function<bool, const Serialization::SerializedObject&>& drawObjectPtrSerializedObject, const Object* attribute)const {
			Unused(drawObjectPtrSerializedObject, attribute);
			typedef Helpers::DrawCurveStatus(*DrawCurveFn)(const Serialization::SerializedObject&, size_t, OS::Logger*);
			static const DrawCurveFn drawFunctions[] = {
				Helpers::DrawCurve<float>,
				Helpers::DrawCurve<Vector2>,
				Helpers::DrawCurve<Vector3>,
				Helpers::DrawCurve<Vector4>
			};
			static const constexpr size_t drawFunctionCount = sizeof(drawFunctions) / sizeof(DrawCurveFn);
			for (size_t i = 0u; i < drawFunctionCount; i++) {
				Helpers::DrawCurveStatus status = drawFunctions[i](object, viewId, logger);
				if (status == Helpers::DrawCurveStatus::TYPE_MISMATCH) continue;
				else return status == Helpers::DrawCurveStatus::CURVE_EDITED;
			}
			if (logger != nullptr) logger->Error("TimelineCurveDrawer::DrawObject - Unsupported Serializer Type!");
			return false;
		}

		const TimelineCurveDrawer* TimelineCurveDrawer::Instance() {
			static const TimelineCurveDrawer instance;
			return &instance;
		}
	}

	template<> void TypeIdDetails::OnRegisterType<Editor::TimelineCurveDrawer>() {
		Editor::TimelineCurveDrawer::Instance()->Register(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<TimelineCurve<float>::EditableTimelineCurveAttribute>());
		Editor::TimelineCurveDrawer::Instance()->Register(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<TimelineCurve<Vector2>::EditableTimelineCurveAttribute>());
		Editor::TimelineCurveDrawer::Instance()->Register(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<TimelineCurve<Vector3>::EditableTimelineCurveAttribute>());
		Editor::TimelineCurveDrawer::Instance()->Register(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<TimelineCurve<Vector4>::EditableTimelineCurveAttribute>());
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::TimelineCurveDrawer>() {
		Editor::TimelineCurveDrawer::Instance()->Unregister(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<TimelineCurve<float>::EditableTimelineCurveAttribute>());
		Editor::TimelineCurveDrawer::Instance()->Unregister(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<TimelineCurve<Vector2>::EditableTimelineCurveAttribute>());
		Editor::TimelineCurveDrawer::Instance()->Unregister(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<TimelineCurve<Vector3>::EditableTimelineCurveAttribute>());
		Editor::TimelineCurveDrawer::Instance()->Unregister(Serialization::ItemSerializer::Type::SERIALIZER_LIST, TypeId::Of<TimelineCurve<Vector4>::EditableTimelineCurveAttribute>());
	}
}
