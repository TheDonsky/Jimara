#include "UIClickArea.h"


namespace Jimara {
	namespace UI {
		struct UIClickArea::Helpers {
			class Updater : public virtual ObjectCache<Reference<const Object>>::StoredObject {
			private:
				const Reference<SceneContext> m_context;
				std::recursive_mutex m_updateLock;
				std::set<UIClickArea*> m_areas;
				WeakReference<UIClickArea> m_lastFocus;

				std::vector<size_t> m_parentChainBuffer[2];

				Reference<UIClickArea> GetHoveredArea() {
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

						// Get transform:
						UITransform* transform = area->GetComponentInParents<UITransform>();
						if (transform == nullptr || transform->Canvas() == nullptr)
							continue;
						Canvas* canvas = transform->Canvas();
						
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

						// Check if the cursor is on top of the area to begin with:
						{
							const UITransform::UIPose pose = transform->Pose();
							const Vector2 scale = pose.Scale();
							if (std::abs(scale.x * scale.y) <= std::numeric_limits<float>::epsilon())
								continue;
							const Vector2 offset = onCanvasCursorPosition - pose.center;
							const Vector2 relPos = Vector2(
								Math::Dot(pose.right / scale.x, offset) / scale.x,
								Math::Dot(pose.up / scale.y, offset) / scale.y);
							if (std::abs(relPos.x) >= std::abs(pose.size.x * 0.5f) ||
								std::abs(relPos.y) >= std::abs(pose.size.y * 0.5f))
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
					const Reference<UIClickArea> areaOnTop = GetHoveredArea();
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

		UIClickArea::UIClickArea(Component* parent, const std::string_view& name) 
			: Component(parent, name)
			, m_updater(Helpers::Updater::Instance(parent->Context())) {
		}

		UIClickArea::~UIClickArea() {
			Helpers::AddOrRemoveToUpdater(this);
			m_updater = nullptr;
		}

		Reference<UIClickArea> UIClickArea::FocusedArea(SceneContext* context) {
			const Reference<Helpers::Updater> updater = Helpers::Updater::Instance(context);
			if (updater == nullptr)
				return nullptr;
			else return updater->FocusedArea();
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
