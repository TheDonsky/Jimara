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

			template<typename CurveValueType> inline static const constexpr size_t CurveChannelCount() { return 0u; }
			template<> inline static const constexpr size_t CurveChannelCount<float>() { return 1u; }
			template<> inline static const constexpr size_t CurveChannelCount<Vector2>() { return 2u; }
			template<> inline static const constexpr size_t CurveChannelCount<Vector3>() { return 3u; }
			template<> inline static const constexpr size_t CurveChannelCount<Vector4>() { return 4u; }

			template<typename CurveValueType> inline static const ImVec4 CurveShapeColor(size_t channelId) {
				static const ImVec4 colors[] = {
					ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
					ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
					ImVec4(0.0f, 0.0f, 1.0f, 1.0f),
					ImVec4(1.0f, 0.0f, 1.0f, 1.0f)
				};
				return colors[channelId];
			}
			template<> inline static const ImVec4 CurveShapeColor<float>(size_t channelId) { Unused(channelId); return ImVec4(0.0f, 1.0f, 0.0f, 1.0f); }

			template<typename CurveValueType> inline static float GetNodeValue(const CurveValueType& node, size_t channelId) { Unused(node, channelId); return 0.0f; }
			template<> inline static float GetNodeValue<float>(const float& node, size_t channelId) { Unused(channelId); return node; }
			template<> inline static float GetNodeValue<Vector2>(const Vector2& node, size_t channelId) { return node[static_cast<uint8_t>(channelId)]; }
			template<> inline static float GetNodeValue<Vector3>(const Vector3& node, size_t channelId) { return node[static_cast<uint8_t>(channelId)]; }
			template<> inline static float GetNodeValue<Vector4>(const Vector4& node, size_t channelId) { return node[static_cast<uint8_t>(channelId)]; }

			template<typename CurveValueType> inline static void SetNodeValue(Property<CurveValueType> node, size_t channelId, float value) { Unused(node, channelId, value); }
			template<> inline static void SetNodeValue<float>(Property<float> node, size_t channelId, float value) { Unused(channelId); node = value; }
			template<> inline static void SetNodeValue<Vector2>(Property<Vector2> node, size_t channelId, float value) { 
				Vector2 vecValue = node; vecValue[static_cast<uint8_t>(channelId)] = value; node = vecValue;
			}
			template<> inline static void SetNodeValue<Vector3>(Property<Vector3> node, size_t channelId, float value) { 
				Vector3 vecValue = node; vecValue[static_cast<uint8_t>(channelId)] = value; node = vecValue;
			}
			template<> inline static void SetNodeValue<Vector4>(Property<Vector4> node, size_t channelId, float value) { 
				Vector4 vecValue = node; vecValue[static_cast<uint8_t>(channelId)] = value; node = vecValue;
			}

			template<typename CurveValueType>
			struct CurvePointInfo {
				float time = 0.0f;
				BezierNode<CurveValueType> node;
			};

			template<typename CurveValueType>
			inline static bool MoveCurveVerts(std::map<float, BezierNode<CurveValueType>>* curve, size_t& nodeIndex) {
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
							info.time = static_cast<float>(nodePosition.x);
							SetNodeValue<CurveValueType>(Property<CurveValueType>(&info.node.Value()), channelId, static_cast<float>(nodePosition.y));
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

			template<typename CurveValueType>
			inline static bool MoveCurveHandles(std::map<float, BezierNode<CurveValueType>>* curve, size_t& nodeIndex) {
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
				for (size_t channelId = 0u; channelId < CurveChannelCount<CurveValueType>(); channelId++) {
					auto prev = curve->end();
					auto next = curve->begin();
					while (next != curve->end()) {
						auto cur = next;
						++next;
						
						// Line that will be drawn:
						ImPlotPoint line[2];
						line[0] = { cur->first, GetNodeValue<CurveValueType>(cur->second.Value(), channelId) };

						auto drawHandle = [&](Property<CurveValueType> handle, float deltaX) {
							const Vector2 delta(deltaX, GetNodeValue<CurveValueType>(handle, channelId));

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
								static_cast<double>(direction.y) * HANDLE_LENGTH.y + GetNodeValue<CurveValueType>(cur->second.Value(), channelId)
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
			inline static bool DrawContextMenu(std::map<float, BezierNode<CurveValueType>>* curve, size_t viewId, OS::Logger* logger) {
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
							const ImVec2 nodePos = ImPlot::PlotToPixels({ info.time, GetNodeValue<CurveValueType>(info.node.Value(), channelId) });
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
					}

					// Draw all node parameters:
					{
						static const BezierNode<CurveValueType>::Serializer serializer;
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

			template<typename CurveValueType>
			inline static void DrawCurve(std::map<float, BezierNode<CurveValueType>>* curve) {
				if (curve->size() <= 0u) return;
				static thread_local std::vector<ImPlotPoint> shape;

				// Calculate plot position:
				const ImVec2 plotSize = ImPlot::GetPlotSize();
				const ImVec2 plotPos = ImPlot::GetPlotPos();
				const float plotRangeStart = static_cast<float>(ImPlot::PixelsToPlot({ plotPos.x, plotPos.y + plotSize.y }).x);
				const float plotRangeEnd = static_cast<float>(ImPlot::PixelsToPlot({ plotPos.x + plotSize.x, plotPos.y }).x);

				for (size_t channelId = 0u; channelId < CurveChannelCount<CurveValueType>(); channelId++) {
					shape.clear();

					// Construct shape:
					const float step = (plotRangeEnd - plotRangeStart) / Math::Max(plotSize.x, 1.0f);
					for (float time = plotRangeStart; time <= plotRangeEnd; time += step)
						shape.push_back({ time, GetNodeValue(TimelineCurve<CurveValueType>::Value(*curve, time), channelId) });

					// Draw shape:
					if (shape.size() > 0u) {
						ImPlot::SetNextLineStyle(CurveShapeColor<CurveValueType>(channelId), 2);
						ImPlot::PlotLine("##shape", &shape[0].x, &shape[0].y, static_cast<int>(shape.size()), 0, 0, sizeof(ImPlotPoint));
					}

					shape.clear();
				}
			}

			template<typename CurveValueType>
			inline static bool EditCurve(std::map<float, BezierNode<CurveValueType>>* curve, size_t viewId, OS::Logger* logger) {
				size_t nodeIndex = 0u;
				bool stuffChanged = MoveCurveHandles(curve, nodeIndex);
				if (MoveCurveVerts(curve, nodeIndex)) stuffChanged = true;
				if (DrawContextMenu(curve, viewId, logger)) stuffChanged = true;
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
					rv = EditCurve(curve, viewId, logger);
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
