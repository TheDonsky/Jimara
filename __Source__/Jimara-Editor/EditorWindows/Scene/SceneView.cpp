#include "SceneView.h"
#include "../../GUI/Utils/DrawTooltip.h"
#include <Components/Transform.h>
#include <Components/Camera.h>
#include <Environment/GraphicsContext/LightingModels/ForwardRendering/ForwardLightingModel.h>
#include <Environment/GraphicsContext/LightingModels/ObjectIdRenderer/ObjectIdRenderer.h>


namespace Jimara {
	namespace Editor {
		namespace {
			static const OS::Input::KeyCode DRAG_KEY = OS::Input::KeyCode::MOUSE_RIGHT_BUTTON;
			static const OS::Input::KeyCode ROTATE_KEY = OS::Input::KeyCode::MOUSE_MIDDLE_BUTTON;

			class ViewRootObject : public virtual Component {
			private:
				Reference<Transform> m_transform;
				Reference<Camera> m_camera;
				Reference<ViewportObjectQuery> m_viewportObjectQuery;

				mutable SpinLock m_hoverResultLock;
				Rect m_viewportRect = Rect(Vector2(0.0f), Vector2(0.0f));
				ViewportObjectQuery::Result m_hoverResult;

				inline Vector2 MousePosition() {
					std::unique_lock<SpinLock> lock(m_hoverResultLock);
					return Vector2(
						Context()->Input()->GetAxis(OS::Input::Axis::MOUSE_POSITION_X),
						Context()->Input()->GetAxis(OS::Input::Axis::MOUSE_POSITION_Y)) - m_viewportRect.start;
				}

				inline void MakeViewportQuery() {
					Vector2 mousePosition = MousePosition();
					const Size2 requestPosition(
						mousePosition.x >= 0.0f ? static_cast<uint32_t>(mousePosition.x) : (~((uint32_t)0)),
						mousePosition.y >= 0.0f ? static_cast<uint32_t>(mousePosition.y) : (~((uint32_t)0)));
					void(*queryCallback)(Object*, ViewportObjectQuery::Result) = [](Object* selfPtr, ViewportObjectQuery::Result result) {
						ViewRootObject* self = dynamic_cast<ViewRootObject*>(selfPtr);
						std::unique_lock<SpinLock> lock(self->m_hoverResultLock);
						self->m_hoverResult = result;
					};
					m_viewportObjectQuery->QueryAsynch(requestPosition, Callback(queryCallback), this);
				}



				Vector2 m_actionMousePositionOrigin = Vector2(0.0f);
				Vector3 m_dragStartPosition = Vector3(0.0f);
				float m_dragSpeed = 0.0f;

				inline bool Drag(Vector2 viewportSize) {
					if (Context()->Input()->KeyDown(DRAG_KEY)) {
						m_dragStartPosition = m_transform->WorldPosition();
						if (m_hoverResult.component == nullptr)
							m_dragSpeed = 1.0f;
						else {
							Vector3 deltaPosition = (m_hoverResult.objectPosition - m_dragStartPosition);
							float distance = Math::Dot(deltaPosition, m_transform->Forward());
							m_dragSpeed = distance * std::tan(Math::Radians(m_camera->FieldOfView()) * 0.5f) * 2.0f;
						}
						m_actionMousePositionOrigin = MousePosition();
					}
					else if (Context()->Input()->KeyPressed(DRAG_KEY)) {
						Vector2 mousePosition = MousePosition();
						Vector2 mouseDelta = (mousePosition - m_actionMousePositionOrigin) / viewportSize.y;
						m_transform->SetWorldPosition(m_dragStartPosition +
							m_dragSpeed * (m_transform->Right() * -mouseDelta.x) +
							m_dragSpeed * (m_transform->Up() * mouseDelta.y));
					}
					else return false;
					return true;
				}

				inline bool Rotate() {
					if (Context()->Input()->KeyDown(ROTATE_KEY)) {
						
					}
					else if (Context()->Input()->KeyPressed(ROTATE_KEY)) {

					}
					else return false;
					return true;
				}

				inline bool Zoom() {
					return false;
				}

				inline void OnGraphicsSynch() {
					MakeViewportQuery();
					Vector2 viewportSize = ViewportRect().Size();
					if ((!ActiveInHeirarchy()) || (viewportSize.x * viewportSize.y) <= std::numeric_limits<float>::epsilon()) return;
					else if (Drag(viewportSize)) return;
					else if (Rotate()) return;
					else if (Zoom()) return;
				}

			public:
				inline ViewRootObject(Scene::LogicContext* context) : Component(context, "ViewRootObject") {
					m_transform = Object::Instantiate<Transform>(this, "SceneView::CameraTransform");
					m_camera = Object::Instantiate<Camera>(m_transform, "SceneView::Camera");
					m_camera->SetEnabled(false); // TODO: Once we can render off-screen, disabling should no longer be necessary...
					m_viewportObjectQuery = ViewportObjectQuery::GetFor(m_camera->ViewportDescriptor());
					if (m_viewportObjectQuery == nullptr)
						context->Log()->Fatal("SceneView::ViewRootObject - Failed to create a ViewportObjectQuery! [File:", __FILE__, "'; Line: ", __LINE__, "]");
					Context()->Graphics()->OnGraphicsSynch() += Callback(&ViewRootObject::OnGraphicsSynch, this);
				}
				inline virtual ~ViewRootObject() {}

				inline const LightingModel::ViewportDescriptor* ViewportDescriptor()const { 
					return m_camera->ViewportDescriptor(); 
				}

				inline Rect ViewportRect()const {
					std::unique_lock<SpinLock> lock(m_hoverResultLock);
					Rect rv = m_viewportRect;
					return rv;
				}

				inline void SetViewportRect(const Rect& viewportRect) {
					std::unique_lock<SpinLock> lock(m_hoverResultLock);
					m_viewportRect = viewportRect;
				}

				inline ViewportObjectQuery::Result GetHoverResults()const {
					std::unique_lock<SpinLock> lock(m_hoverResultLock);
					ViewportObjectQuery::Result rv = m_hoverResult;
					return rv;
				}

			protected:
				inline virtual void OnComponentDestroyed()final override {
					Context()->Graphics()->OnGraphicsSynch() -= Callback(&ViewRootObject::OnGraphicsSynch, this);
				}
			};

			class RenderJob : public virtual JobSystem::Job {
			private:
				Reference<ViewRootObject> m_root;
				Reference<Scene::GraphicsContext::Renderer> m_renderer;
				Reference<ObjectIdRenderer> m_objectIdRenderer;

				SpinLock m_resolutionLock;
				Size2 m_targetResolution;
				Reference<Graphics::TextureView> m_targetTexture;

				Reference<Graphics::TextureView> GetTargetTexture() {
					Size3 size = [&]() {
						std::unique_lock<SpinLock> lock(m_resolutionLock);
						return Size3(max(m_targetResolution.x, 1), max(m_targetResolution.y, 1), 1);;
					}();
					if (m_targetTexture == nullptr || m_targetTexture->TargetTexture()->Size() != size) {
						m_objectIdRenderer->SetResolution(size);

						const Reference<Graphics::Texture> texture = m_root->Context()->Graphics()->Device()->CreateMultisampledTexture(
							Graphics::Texture::TextureType::TEXTURE_2D,
							Graphics::Texture::PixelFormat::B8G8R8A8_SRGB,
							size, 1, Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
						if (texture == nullptr) {
							m_root->Context()->Log()->Error("SceneView::RenderJob - Failed to create target texture! [File:", __FILE__, "'; Line: ", __LINE__, "]");
							return nullptr;
						}
						{
							std::unique_lock<SpinLock> lock(m_resolutionLock);
							m_targetTexture = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
						}
						if (m_targetTexture == nullptr)
							m_root->Context()->Log()->Error("SceneView::RenderJob - Failed to create target texture view! [File:", __FILE__, "'; Line: ", __LINE__, "]");
					}
					return m_targetTexture;
				}

			public:
				inline RenderJob(Scene::LogicContext* context) {
					std::unique_lock<std::recursive_mutex> lock(context->UpdateLock());
					m_root = Object::Instantiate<ViewRootObject>(context);
					m_renderer = ForwardLightingModel::Instance()->CreateRenderer(m_root->ViewportDescriptor());
					if (m_renderer == nullptr)
						context->Log()->Fatal("SceneView::RenderJob - Failed to create a renderer! [File:", __FILE__, "'; Line: ", __LINE__, "]");
					m_objectIdRenderer = ObjectIdRenderer::GetFor(m_root->ViewportDescriptor(), true);
					if (m_objectIdRenderer == nullptr) 
						context->Log()->Fatal("SceneView::RenderJob - Failed to create an ObjectIdRenderer! [File:", __FILE__, "'; Line: ", __LINE__, "]");
				}

				inline virtual ~RenderJob() {
					std::unique_lock<std::recursive_mutex> lock(m_root->Context()->UpdateLock());
					if (!m_root->Destroyed())
						m_root->Destroy();
				}

				inline void SetResolution(Size2 resolution) {
					std::unique_lock<SpinLock> lock(m_resolutionLock);
					m_targetResolution = resolution;
				}

				inline Reference<Graphics::TextureView> ViewImage() {
					std::unique_lock<SpinLock> lock(m_resolutionLock);
					const Reference<Graphics::TextureView> image = m_targetTexture;
					return image;
				}

				inline ViewRootObject* Root()const {
					return m_root;
				}

			protected:
				inline virtual void Execute()override {
					Graphics::Pipeline::CommandBufferInfo commandBuffer = m_root->Context()->Graphics()->GetWorkerThreadCommandBuffer();
					Reference<Graphics::TextureView> target = GetTargetTexture();
					if (target != nullptr)
						m_renderer->Render(commandBuffer, target);
					// TODO: Move the camera around, maybe?
				}

				inline virtual void CollectDependencies(Callback<Job*> addDependency) {
					Unused(addDependency);
				}
			};

			inline static Reference<JobSystem::Job> CreateJob(Reference<Scene::LogicContext> context) {
				std::unique_lock<std::recursive_mutex> lock(context->UpdateLock());
				Reference<JobSystem::Job> job = Object::Instantiate<RenderJob>(context);
				context->Graphics()->RenderJobs().Add(job);
				return job;
			}

			inline static void RemoveJob(Reference<Scene::LogicContext>& context, Reference<JobSystem::Job>& job) {
				if (context == nullptr || job == nullptr) return;
				context->Graphics()->RenderJobs().Remove(job);
				context = nullptr;
				job = nullptr;
			}

			inline static void UpdateRenderJob(EditorScene* editorScene, Reference<Scene::LogicContext>& viewContext, Reference<JobSystem::Job>& updateJob) {
				Reference<Scene::LogicContext> context = editorScene->RootObject()->Context();
				if (viewContext != context) {
					RemoveJob(viewContext, updateJob);
					viewContext = context;
					updateJob = CreateJob(viewContext);
				}
			}

			inline static Rect GetViewportRect() {
				auto toVec2 = [](const ImVec2& v) { return Jimara::Vector2(v.x, v.y); };
				const Vector2 viewportOffset = toVec2(ImGui::GetItemRectSize()) * Vector2(0.0f, 1.0f);
				const Vector2 viewportPosition = toVec2(ImGui::GetWindowPos()) + viewportOffset;
				const Vector2 viewportSize = toVec2(ImGui::GetWindowSize()) - viewportOffset;
				return Rect(viewportPosition, viewportPosition + viewportSize);
			}

			inline static void RenderToViewport(RenderJob* job, const Rect& viewportRect) {
				Reference<Graphics::TextureView> image = job->ViewImage();
				if (image != nullptr)
					ImGuiRenderer::Texture(image->TargetTexture(), viewportRect);
				job->SetResolution(Size2((uint32_t)viewportRect.Size().x, (uint32_t)viewportRect.Size().y));
			}
		}


		SceneView::SceneView(EditorContext* context) 
			: EditorSceneController(context), EditorWindow(context, "Scene View") {}

		SceneView::~SceneView() {
			RemoveJob(m_viewContext, m_updateJob);
		}

		void SceneView::DrawEditorWindow() {
			Reference<EditorScene> editorScene = GetOrCreateScene();
			UpdateRenderJob(editorScene, m_viewContext, m_updateJob);
			
			RenderJob* job = dynamic_cast<RenderJob*>(m_updateJob.operator->());
			const Rect viewportRect = GetViewportRect();

			RenderToViewport(job, viewportRect);
			job->Root()->SetViewportRect(viewportRect);

			bool focused = ImGui::IsWindowFocused();
			job->Root()->SetEnabled(focused);
			if (focused) {
				const ViewportObjectQuery::Result currentResult = job->Root()->GetHoverResults();
				std::unique_lock<std::recursive_mutex> lock(editorScene->UpdateLock());
				if (currentResult.component != nullptr && (!currentResult.component->Destroyed())) {
					std::string tip = [&]() {
						std::stringstream stream;
						stream << "window:" << ((size_t)this) << "; component:" << ((size_t)currentResult.component.operator->());
						return stream.str();
					}();
					DrawTooltip(tip, currentResult.component->Name(), true);
				}
			}
			// TODO: Implement this stuff!!!....
		}


		namespace {
			static const EditorMainMenuCallback editorMenuCallback(
				"Scene/SceneView", Callback<EditorContext*>([](EditorContext* context) {
					Object::Instantiate<SceneView>(context);
					}));
			static EditorMainMenuAction::RegistryEntry action;
		}
	}

	template<> void TypeIdDetails::GetParentTypesOf<Editor::SceneView>(const Callback<TypeId>& report) {
		report(TypeId::Of<Editor::EditorSceneController>());
		report(TypeId::Of<Editor::EditorWindow>());
	}
	template<> void TypeIdDetails::OnRegisterType<Editor::SceneView>() {
		Editor::action = &Editor::editorMenuCallback;
	}
	template<> void TypeIdDetails::OnUnregisterType<Editor::SceneView>() {
		Editor::action = nullptr;
	}
}
