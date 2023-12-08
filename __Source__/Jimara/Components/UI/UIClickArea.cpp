#include "UIClickArea.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"


namespace Jimara {
	namespace UI {
		struct UIClickArea::Helpers {
			class Updater : public virtual ObjectCache<Reference<const Object>>::StoredObject {
			private:
				const Reference<SceneContext> m_context;
				std::recursive_mutex m_updateLock;
				std::set<UIClickArea*> m_areas;

				WeakReference<UIClickArea> m_areaOnTop;
				WeakReference<UIClickArea> m_lastFocus;
				WeakReference<UIClickArea> m_releasedArea;
				OS::Input::KeyCode m_focusButton = OS::Input::KeyCode::NONE;

				std::vector<size_t> m_parentChainBuffer[2];

				Reference<UIClickArea> GetAreaOnTop() {
					// Cursor position data:
					const Vector2 cursorOnScreenPosition = Vector2(
						m_context->Input()->GetAxis(OS::Input::Axis::MOUSE_POSITION_X),
						m_context->Input()->GetAxis(OS::Input::Axis::MOUSE_POSITION_Y));
					Canvas* lastCanvas = nullptr;
					Vector2 onCanvasCursorPosition = Vector2(0.0f);

					// 'top' area thus far:
					UIClickArea* topArea = nullptr;
					Canvas* topCanvas = nullptr;
					size_t parentChainId = ~size_t(0u);

					for (decltype(m_areas)::const_iterator it = m_areas.begin(); it != m_areas.end(); ++it) {
						// Make sure we are iterating over a valid area:
						UIClickArea* area = *it;
						assert(area != nullptr);
						if (area->Destroyed() || (!area->ActiveInHeirarchy()))
							continue;

						// Get transform & canvas:
						UITransform* transform = area->GetComponentInParents<UITransform>();
						Canvas* canvas = (transform == nullptr) ? nullptr : transform->Canvas();
						if (canvas == nullptr)
							continue;
						
						
						// If the topCanvas is on top anyway, no need to check any further:
						if ((lastCanvas != nullptr) && (lastCanvas != canvas) &&
							(canvas->RendererCategory() < lastCanvas->RendererCategory() ||
								(canvas->RendererCategory() == lastCanvas->RendererCategory() &&
									canvas->RendererPriority() >= lastCanvas->RendererPriority())))
							continue;

						// Update cursor position:
						if (canvas != lastCanvas) {
							lastCanvas = canvas;
							onCanvasCursorPosition = lastCanvas->ScreenToCanvasPosition(cursorOnScreenPosition);
						}

						// Make sure cursor does not go beyond canvas boundaries:
						{
							const Vector2 canvasHalfSize = canvas->Size() * 0.5f;
							if (onCanvasCursorPosition.x < -canvasHalfSize.x ||
								onCanvasCursorPosition.x > canvasHalfSize.x ||
								onCanvasCursorPosition.y < -canvasHalfSize.y ||
								onCanvasCursorPosition.y > canvasHalfSize.y)
								continue;
						}

						// Check if the cursor is on top of the area to begin with:
						{
							const UITransform::UIPose pose = transform->Pose();
							const Vector2 scale = pose.Scale();
							if (std::abs(scale.x * scale.y) <= std::numeric_limits<float>::epsilon())
								continue;
							const Vector2 offset = onCanvasCursorPosition - pose.center;
							const Vector2 right = pose.right / scale.x;
							const Vector2 up = pose.up / scale.y;
							const float cosA = Math::Dot(right, up);
							if (std::abs(cosA) >= (1.0f - std::numeric_limits<float>::epsilon()))
								continue;
							const Vector2 proj = Vector2(Math::Dot(right, offset), Math::Dot(up, offset));
							const float x = (proj.x - cosA * proj.y) / (1.0f - cosA * cosA);
							const float y = proj.y - cosA * x;
							if (std::abs(x) >= std::abs(pose.size.x * 0.5f * scale.x) ||
								std::abs(y) >= std::abs(pose.size.y * 0.5f * scale.y))
								continue;
						}

						// If no other element is hovered, we just assign:
						if (topArea == nullptr) {
							topArea = area;
							topCanvas = canvas;
							continue;
						}

						// Compare order to what we have on top:
						{
							auto fillParentOrder = [](std::vector<size_t>& list, Component* elem, Canvas* canvas) {
								list.clear();
								while (elem != canvas) {
									list.push_back(elem->IndexInParent());
									elem = elem->Parent();
								}
							};
							if (parentChainId >= 2u) {
								parentChainId = 0u;
								fillParentOrder(m_parentChainBuffer[0u], topArea, topCanvas);
							}
							assert(parentChainId <= 2u);
							const size_t nextChainId = (parentChainId + 1u) & 1u;
							const std::vector<size_t>& oldChain = m_parentChainBuffer[parentChainId & 1u];
							std::vector<size_t>& newChain = m_parentChainBuffer[nextChainId];
							fillParentOrder(newChain, area, canvas);
							
							auto newIsDrawnLater = [&]() {
								const size_t commonChainSize = Math::Min(oldChain.size(), newChain.size());
								for (size_t i = 1u; i <= commonChainSize; i++) {
									const size_t a = oldChain[oldChain.size() - i];
									const size_t b = newChain[newChain.size() - i];
									if (a < b)
										return true;
									else if (a > b)
										return false;
								}
								return oldChain.size() < newChain.size();
							};
							if (newIsDrawnLater()) {
								topArea = area;
								topCanvas = canvas;
								parentChainId = nextChainId;
							}
						}
					}

					return topArea;
				}

				void Update() {
					std::unique_lock<decltype(m_updateLock)> lock(m_updateLock);
					// Retrieve current area on top and the last focus:
					const Reference<UIClickArea> areaOnTop = GetAreaOnTop();
					const Reference<UIClickArea> lastFocus = m_lastFocus;
					m_areaOnTop = areaOnTop;

					// Clear previous focus:
					{
						auto clearSingleFrameFlags = [&](const Reference<UIClickArea>& area) {
							if (area == nullptr)
								return;
							area->m_stateFlags &= (~(StateFlags::GOT_PRESSED | StateFlags::GOT_RELEASED));
							if (area != areaOnTop)
								area->m_stateFlags &= ~StateFlags::HOVERED;
							if (area != lastFocus)
								area->m_stateFlags &= ~StateFlags::PRESSED;
						};
						clearSingleFrameFlags(areaOnTop);
						clearSingleFrameFlags(lastFocus);
						clearSingleFrameFlags(m_releasedArea);
						m_releasedArea = nullptr;
					}

					// Make sure hovered flag is always set:
					if (areaOnTop == lastFocus && areaOnTop != nullptr)
						areaOnTop->m_stateFlags |= StateFlags::HOVERED;

					// Check if we need to exit focus:
					if (lastFocus != nullptr && m_focusButton != OS::Input::KeyCode::NONE) {
						if (lastFocus->Destroyed() || (!lastFocus->ActiveInHeirarchy()) ||
							((lastFocus->ClickFlags() & ClickAreaFlags::AUTO_RELEASE_WHEN_OUT_OF_BOUNDS) != ClickAreaFlags::NONE && areaOnTop != lastFocus) ||
							(!m_context->Input()->KeyPressed(m_focusButton))) {
							m_focusButton = OS::Input::KeyCode::NONE;
							if (!lastFocus->Destroyed()) {
								lastFocus->m_stateFlags = (lastFocus->m_stateFlags | StateFlags::GOT_RELEASED) & (~StateFlags::PRESSED);
								m_releasedArea = lastFocus;
								lastFocus->m_onReleased(lastFocus);
							}
						}
						else {
							if (!lastFocus->Destroyed()) {
								assert((lastFocus->m_stateFlags & StateFlags::PRESSED) != StateFlags::NONE);
								lastFocus->m_onPressed(lastFocus);
							}
							return;
						}
					}

					// Check if hovered area changed:
					if (areaOnTop != lastFocus) {
						m_lastFocus = (areaOnTop != nullptr && (!areaOnTop->Destroyed())) ? areaOnTop : nullptr;
						m_focusButton = OS::Input::KeyCode::NONE;
						if (lastFocus != nullptr) {
							lastFocus->m_stateFlags &= ~(StateFlags::PRESSED | StateFlags::HOVERED);
							if (!lastFocus->Destroyed())
								lastFocus->m_onFocusExit(lastFocus);
						}
						if (areaOnTop != nullptr && (!areaOnTop->Destroyed())) {
							areaOnTop->m_stateFlags |= StateFlags::HOVERED;
							areaOnTop->m_onFocusEnter(areaOnTop);
						}
					}

					// If areaOnTop gets disabled somewhere in the middle, we do not try to click it any more:
					if ((areaOnTop == nullptr) || (!areaOnTop->ActiveInHeirarchy()))
						return;

					// Check if new click happened:
					auto tryClick = [&](ClickAreaFlags flag, OS::Input::KeyCode key) {
						if ((areaOnTop == nullptr) || areaOnTop->Destroyed() ||
							(m_focusButton != OS::Input::KeyCode::NONE) ||
							((areaOnTop->ClickFlags() & flag) == ClickAreaFlags::NONE) ||
							(!m_context->Input()->KeyDown(key)))
							return false;
						m_focusButton = key;
						areaOnTop->m_stateFlags |= StateFlags::GOT_PRESSED | StateFlags::PRESSED;
						areaOnTop->m_onClicked(areaOnTop);
						return true;
					};
					tryClick(ClickAreaFlags::LEFT_BUTTON, OS::Input::KeyCode::MOUSE_LEFT_BUTTON);
					tryClick(ClickAreaFlags::RIGHT_BUTTON, OS::Input::KeyCode::MOUSE_RIGHT_BUTTON);
					tryClick(ClickAreaFlags::MIDDLE_BUTTON, OS::Input::KeyCode::MOUSE_MIDDLE_BUTTON);

					// If it is not clicked, we fire the hovered event:
					if (areaOnTop != nullptr && (!areaOnTop->Destroyed()) && m_focusButton == OS::Input::KeyCode::NONE)
						areaOnTop->m_onHovered(areaOnTop);
				}

			public:
				inline Updater(SceneContext* context) 
					: m_context(context) {
					assert(m_context != nullptr);
					m_context->OnPreUpdate() += Callback(&Updater::Update, this);
				}

				inline virtual ~Updater() {
					m_context->OnPreUpdate() -= Callback(&Updater::Update, this);
					assert(m_areas.empty());
				}

				static Reference<Updater> Instance(SceneContext* context) {
					if (context == nullptr)
						return nullptr;
					struct Cache : public virtual ObjectCache<Reference<const Object>> {
						static Reference<Updater> Get(SceneContext* ctx) {
							static Cache cache;
							std::mutex creationLock;
							std::unique_lock<std::mutex> lock(creationLock);
							return cache.GetCachedOrCreate(ctx, false,
								[&]() { 
									Reference<Updater> instance = Object::Instantiate<Updater>(ctx); 
									ctx->StoreDataObject(instance);
									return instance;
								});
						}
					};
					return Cache::Get(context);
				}

				inline void AddArea(UIClickArea* area) {
					std::unique_lock<decltype(m_updateLock)> lock(m_updateLock);
					m_areas.insert(area);
				}

				inline void RemoveArea(UIClickArea* area) {
					std::unique_lock<decltype(m_updateLock)> lock(m_updateLock);
					m_areas.erase(area);
				}

				inline Reference<UIClickArea> FocusedArea() {
					std::unique_lock<decltype(m_updateLock)> lock(m_updateLock);
					Reference<UIClickArea> rv = m_lastFocus;
					return rv;
				}
			};

			static Updater* FocusUpdater(const UIClickArea* self) {
				return dynamic_cast<Updater*>(self->m_updater.operator Jimara::Object * ());
			}

			static void AddOrRemoveToUpdater(UIClickArea* self) {
				Updater* updater = FocusUpdater(self);
				if (self->ActiveInHeirarchy())
					updater->AddArea(self);
				else updater->RemoveArea(self);
			}
		};

		const Object* UIClickArea::ClickAreaFlagsAttribute() {
			static const Reference<const Object> attribute =
				Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<ClickAreaFlags>>>(true,
					"NONE", ClickAreaFlags::NONE,
					"LEFT_BUTTON", ClickAreaFlags::LEFT_BUTTON,
					"RIGHT_BUTTON", ClickAreaFlags::RIGHT_BUTTON,
					"MIDDLE_BUTTON", ClickAreaFlags::MIDDLE_BUTTON,
					"AUTO_RELEASE_WHEN_OUT_OF_BOUNDS", ClickAreaFlags::AUTO_RELEASE_WHEN_OUT_OF_BOUNDS);
			return attribute;
		}

		Reference<UIClickArea> UIClickArea::FocusedArea(SceneContext* context) {
			const Reference<Helpers::Updater> updater = Helpers::Updater::Instance(context);
			if (updater == nullptr)
				return nullptr;
			else return updater->FocusedArea();
		}

		UIClickArea::UIClickArea(Component* parent, const std::string_view& name) 
			: Component(parent, name)
			, m_updater(Helpers::Updater::Instance(parent->Context())) {
		}

		UIClickArea::~UIClickArea() {
			Helpers::AddOrRemoveToUpdater(this);
			m_updater = nullptr;
		}

		void UIClickArea::GetFields(Callback<Serialization::SerializedObject> recordElement) {
			Component::GetFields(recordElement);
			JIMARA_SERIALIZE_FIELDS(this, recordElement) {
				JIMARA_SERIALIZE_FIELD_GET_SET(ClickFlags, SetClickFlags, "Click Flags", "Flags for area click detection", ClickAreaFlagsAttribute());
			};
		}

		void UIClickArea::OnComponentEnabled() {
			Helpers::AddOrRemoveToUpdater(this);
		}

		void UIClickArea::OnComponentDisabled() {
			Helpers::AddOrRemoveToUpdater(this);
		}
	}

	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<UI::UIClickArea>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<UI::UIClickArea>(
			"UI Click Area", "Jimara/UI/ClickArea", "UI Component, that detects mouse-over UITransform and clicks");
		report(factory);
	}
}
