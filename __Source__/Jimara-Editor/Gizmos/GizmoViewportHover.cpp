#include "GizmoViewportHover.h"
#include <Environment/Rendering/LightingModels/ObjectIdRenderer/ObjectIdRenderer.h>

namespace Jimara {
	namespace Editor {
		namespace {
			struct GizmoViewportHoverUpdater : public virtual Object {
				const Reference<GizmoViewport> gizmoViewport;
				const Reference<ObjectIdRenderer> targetSceneObjectIdRenderer;
				const Reference<ObjectIdRenderer> gizmoSceneObjectIdRendererSelection;
				const Reference<ObjectIdRenderer> gizmoSceneObjectIdRendererHandles;
				const Reference<ViewportObjectQuery> targetSceneQuery;
				const Reference<ViewportObjectQuery> gizmoSceneQuerySelection;
				const Reference<ViewportObjectQuery> gizmoSceneQueryHandles;

				mutable SpinLock hoverResultLock;
				ViewportObjectQuery::Result targetSceneResult;
				ViewportObjectQuery::Result gizmoSceneResultSelection;
				ViewportObjectQuery::Result gizmoSceneResultHandles;

				inline Vector2 MousePosition() {
					const OS::Input* input = gizmoViewport->GizmoSceneViewport()->Context()->Input();
					return Vector2(
						input->GetAxis(OS::Input::Axis::MOUSE_POSITION_X),
						input->GetAxis(OS::Input::Axis::MOUSE_POSITION_Y));
				}

				inline void Update() {
					Vector2 mousePosition = MousePosition();
					const Size2 requestPosition(
						mousePosition.x >= 0.0f ? static_cast<uint32_t>(mousePosition.x) : (~((uint32_t)0)),
						mousePosition.y >= 0.0f ? static_cast<uint32_t>(mousePosition.y) : (~((uint32_t)0)));
					{
						void(*queryCallback)(Object*, ViewportObjectQuery::Result) = [](Object* selfPtr, ViewportObjectQuery::Result result) {
							GizmoViewportHoverUpdater* self = dynamic_cast<GizmoViewportHoverUpdater*>(selfPtr);
							std::unique_lock<SpinLock> lock(self->hoverResultLock);
							self->targetSceneResult = result;
						};
						targetSceneQuery->QueryAsynch(requestPosition, Callback(queryCallback), this);
						targetSceneObjectIdRenderer->SetResolution(gizmoViewport->Resolution());
					}
					{
						void(*queryCallback)(Object*, ViewportObjectQuery::Result) = [](Object* selfPtr, ViewportObjectQuery::Result result) {
							GizmoViewportHoverUpdater* self = dynamic_cast<GizmoViewportHoverUpdater*>(selfPtr);
							std::unique_lock<SpinLock> lock(self->hoverResultLock);
							self->gizmoSceneResultSelection = result;
						};
						gizmoSceneQuerySelection->QueryAsynch(requestPosition, Callback(queryCallback), this);
						gizmoSceneObjectIdRendererSelection->SetResolution(gizmoViewport->Resolution());
					}
					{
						void(*queryCallback)(Object*, ViewportObjectQuery::Result) = [](Object* selfPtr, ViewportObjectQuery::Result result) {
							GizmoViewportHoverUpdater* self = dynamic_cast<GizmoViewportHoverUpdater*>(selfPtr);
							std::unique_lock<SpinLock> lock(self->hoverResultLock);
							self->gizmoSceneResultHandles = result;
						};
						gizmoSceneQueryHandles->QueryAsynch(requestPosition, Callback(queryCallback), this);
						gizmoSceneObjectIdRendererHandles->SetResolution(gizmoViewport->Resolution());
					}
				}

				inline GizmoViewportHoverUpdater(GizmoViewport* viewport)
					: gizmoViewport(viewport)
					, targetSceneObjectIdRenderer(ObjectIdRenderer::GetFor(viewport->TargetSceneViewport(), GraphicsLayerMask::All()))
					, gizmoSceneObjectIdRendererSelection(ObjectIdRenderer::GetFor(viewport->GizmoSceneViewport(), GraphicsLayerMask(
						static_cast<GraphicsLayer>(GizmoLayers::SELECTION_WORLD_SPACE),
						static_cast<GraphicsLayer>(GizmoLayers::SELECTION_WORLD_SPACE_INVISIBLE),
						static_cast<GraphicsLayer>(GizmoLayers::SELECTION_OVERLAY),
						static_cast<GraphicsLayer>(GizmoLayers::SELECTION_OVERLAY_INVISIBLE))))
					, gizmoSceneObjectIdRendererHandles(ObjectIdRenderer::GetFor(viewport->GizmoSceneViewport(), GraphicsLayerMask(
						static_cast<GraphicsLayer>(GizmoLayers::HANDLE),
						static_cast<GraphicsLayer>(GizmoLayers::HANDLE_INVISIBLE))))
					, targetSceneQuery(ViewportObjectQuery::GetFor(viewport->TargetSceneViewport(), GraphicsLayerMask::All()))
					, gizmoSceneQuerySelection(ViewportObjectQuery::GetFor(viewport->GizmoSceneViewport(), GraphicsLayerMask(
						static_cast<GraphicsLayer>(GizmoLayers::SELECTION_WORLD_SPACE),
						static_cast<GraphicsLayer>(GizmoLayers::SELECTION_WORLD_SPACE_INVISIBLE),
						static_cast<GraphicsLayer>(GizmoLayers::SELECTION_OVERLAY),
						static_cast<GraphicsLayer>(GizmoLayers::SELECTION_OVERLAY_INVISIBLE))))
					, gizmoSceneQueryHandles(ViewportObjectQuery::GetFor(viewport->GizmoSceneViewport(), GraphicsLayerMask(
						static_cast<GraphicsLayer>(GizmoLayers::HANDLE),
						static_cast<GraphicsLayer>(GizmoLayers::HANDLE_INVISIBLE)))) {}

				inline virtual ~GizmoViewportHoverUpdater() {}

				inline static Callback<> UpdateCall(Object* updater) {
					return Callback(&GizmoViewportHoverUpdater::Update, dynamic_cast<GizmoViewportHoverUpdater*>(updater));
				}

				inline static Scene::LogicContext* GetGizmoContext(Object* updater) {
					return dynamic_cast<GizmoViewportHoverUpdater*>(updater)->gizmoViewport->GizmoSceneViewport()->Context();
				}
			};
		}

		class GizmoViewportHover::Cache : public virtual ObjectCache<Reference<Object>> {
		public:
			inline static Reference<GizmoViewportHover> GetFor(GizmoViewport* viewport) {
				static Cache instance;
				if (viewport == nullptr) 
					return nullptr;
				return instance.GetCachedOrCreate(viewport, false, [&]() -> Reference<GizmoViewportHover> {
					Reference<GizmoViewportHover> hover = new GizmoViewportHover(viewport);
					hover->ReleaseRef();
					return hover;
					});
			}
		};

		Reference<GizmoViewportHover> GizmoViewportHover::GetFor(GizmoViewport* viewport) { return Cache::GetFor(viewport); }

		GizmoViewportHover::GizmoViewportHover(GizmoViewport* viewport) 
			: m_updater(Object::Instantiate<GizmoViewportHoverUpdater>(viewport)) {
			Scene::LogicContext* context = GizmoViewportHoverUpdater::GetGizmoContext(m_updater);
			context->OnUpdate() += GizmoViewportHoverUpdater::UpdateCall(m_updater);
			context->StoreDataObject(m_updater);
		}

		GizmoViewportHover::~GizmoViewportHover() {
			Scene::LogicContext* context = GizmoViewportHoverUpdater::GetGizmoContext(m_updater);
			std::unique_lock<std::recursive_mutex> lock(context->UpdateLock());
			context->OnUpdate() -= GizmoViewportHoverUpdater::UpdateCall(m_updater);
			context->EraseDataObject(m_updater);
		}


		ViewportObjectQuery::Result GizmoViewportHover::TargetSceneHover()const {
			GizmoViewportHoverUpdater* updater = dynamic_cast<GizmoViewportHoverUpdater*>(m_updater.operator->());
			std::unique_lock<SpinLock> lock(updater->hoverResultLock);
			if (updater->gizmoSceneResultSelection.component != nullptr ||
				updater->gizmoSceneResultHandles.component != nullptr) return {};
			ViewportObjectQuery::Result rv = updater->targetSceneResult;
			return rv;
		}

		ViewportObjectQuery::Result GizmoViewportHover::SelectionGizmoHover()const {
			GizmoViewportHoverUpdater* updater = dynamic_cast<GizmoViewportHoverUpdater*>(m_updater.operator->());
			std::unique_lock<SpinLock> lock(updater->hoverResultLock);
			if (updater->gizmoSceneResultHandles.component != nullptr) return {};
			ViewportObjectQuery::Result rv = updater->gizmoSceneResultSelection;
			return rv;
		}

		ViewportObjectQuery::Result GizmoViewportHover::HandleGizmoHover()const {
			GizmoViewportHoverUpdater* updater = dynamic_cast<GizmoViewportHoverUpdater*>(m_updater.operator->());
			std::unique_lock<SpinLock> lock(updater->hoverResultLock);
			ViewportObjectQuery::Result rv = updater->gizmoSceneResultHandles;
			return rv;
		}

		ViewportObjectQuery* GizmoViewportHover::TargetSceneQuery()const { 
			return dynamic_cast<GizmoViewportHoverUpdater*>(m_updater.operator->())->targetSceneQuery;
		}

		ViewportObjectQuery* GizmoViewportHover::SelectionGizmoQuery()const {
			return dynamic_cast<GizmoViewportHoverUpdater*>(m_updater.operator->())->gizmoSceneQuerySelection;
		}

		ViewportObjectQuery* GizmoViewportHover::HandleGizmoQuery()const{
			return dynamic_cast<GizmoViewportHoverUpdater*>(m_updater.operator->())->gizmoSceneQueryHandles;
		}

		Vector2 GizmoViewportHover::CursorPosition()const {
			Scene::LogicContext* context = GizmoViewportHoverUpdater::GetGizmoContext(m_updater);
			return Vector2(
				context->Input()->GetAxis(OS::Input::Axis::MOUSE_POSITION_X),
				context->Input()->GetAxis(OS::Input::Axis::MOUSE_POSITION_Y));
		}
	}
}
