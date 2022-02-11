#include "SceneView.h"
#include "../../GUI/Utils/DrawTooltip.h"
#include <Components/Transform.h>
#include <Components/Camera.h>
#include <Environment/GraphicsContext/LightingModels/ForwardRendering/ForwardLightingModel.h>
#include <Environment/GraphicsContext/LightingModels/ObjectIdRenderer/ObjectIdRenderer.h>


namespace Jimara {
	namespace Editor {
		namespace {
			class ViewRootObject : public virtual Component {
			public:
				inline ViewRootObject(Scene::LogicContext* context) : Component(context, "ViewRootObject") {}
				inline virtual ~ViewRootObject() {}
			};

			class RenderJob : public virtual JobSystem::Job {
			private:
				Reference<ViewRootObject> m_root;
				Reference<Transform> m_transform;
				Reference<Camera> m_camera;
				Reference<Scene::GraphicsContext::Renderer> m_renderer;
				Reference<ObjectIdRenderer> m_objectIdRenderer;
				Reference<ViewportObjectQuery> m_viewportObjectQuery;

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

						const Reference<Graphics::Texture> texture = m_camera->Context()->Graphics()->Device()->CreateMultisampledTexture(
							Graphics::Texture::TextureType::TEXTURE_2D,
							Graphics::Texture::PixelFormat::B8G8R8A8_SRGB,
							size, 1, Graphics::Texture::Multisampling::SAMPLE_COUNT_1);
						if (texture == nullptr) {
							m_camera->Context()->Log()->Error("SceneView::RenderJob - Failed to create target texture! [File:", __FILE__, "'; Line: ", __LINE__, "]");
							return nullptr;
						}
						{
							std::unique_lock<SpinLock> lock(m_resolutionLock);
							m_targetTexture = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
						}
						if (m_targetTexture == nullptr)
							m_camera->Context()->Log()->Error("SceneView::RenderJob - Failed to create target texture view! [File:", __FILE__, "'; Line: ", __LINE__, "]");
					}
					return m_targetTexture;
				}

			public:
				inline RenderJob(Scene::LogicContext* context) {
					std::unique_lock<std::recursive_mutex> lock(context->UpdateLock());
					m_root = Object::Instantiate<ViewRootObject>(context);
					m_transform = Object::Instantiate<Transform>(m_root, "SceneView::CameraTransform");
					m_camera = Object::Instantiate<Camera>(m_transform, "SceneView::Camera");
					m_camera->SetEnabled(false); // TODO: Once we can render off-screen, disabling should no longer be necessary...
					m_renderer = ForwardLightingModel::Instance()->CreateRenderer(m_camera->ViewportDescriptor());
					if (m_renderer == nullptr)
						context->Log()->Fatal("SceneView::RenderJob - Failed to create a renderer! [File:", __FILE__, "'; Line: ", __LINE__, "]");
					m_objectIdRenderer = ObjectIdRenderer::GetFor(m_camera->ViewportDescriptor(), true);
					if (m_objectIdRenderer == nullptr) 
						context->Log()->Fatal("SceneView::RenderJob - Failed to create an ObjectIdRenderer! [File:", __FILE__, "'; Line: ", __LINE__, "]");
					m_viewportObjectQuery = ViewportObjectQuery::GetFor(m_camera->ViewportDescriptor());
					if (m_viewportObjectQuery == nullptr)
						context->Log()->Fatal("SceneView::RenderJob - Failed to create a ViewportObjectQuery! [File:", __FILE__, "'; Line: ", __LINE__, "]");
				}

				inline virtual ~RenderJob() {
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

				inline ViewportObjectQuery* Query() {
					return m_viewportObjectQuery;
				}

			protected:
				inline virtual void Execute()override {
					Graphics::Pipeline::CommandBufferInfo commandBuffer = m_camera->Context()->Graphics()->GetWorkerThreadCommandBuffer();
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

			inline static ViewportObjectQuery::Result GetHoverResult(SpinLock& resultLock, ViewportObjectQuery::Result& result) {
				std::unique_lock<SpinLock> lock(resultLock);
				ViewportObjectQuery::Result rv = result;
				return rv;
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

			if (ImGui::IsWindowFocused()) {
				const ViewportObjectQuery::Result currentResult = GetHoverResult(m_hoverResultLock, m_hoverResult);
				std::unique_lock<std::recursive_mutex> lock(editorScene->UpdateLock());
				if (currentResult.component != nullptr && (!currentResult.component->Destroyed())) {
					std::string tip = [&]() {
						std::stringstream stream;
						stream << "window:" << ((size_t)this) << "; component:" << ((size_t)currentResult.component.operator->());
						return stream.str();
					}();
					DrawTooltip(tip, currentResult.component->Name(), true);
				}
				RequestHoverResult(viewportRect);
			}
			// TODO: Implement this stuff!!!....
		}


		void SceneView::RequestHoverResult(const Rect& viewportRect) {
			Vector2 mousePosition = Vector2(
				Context()->InputModule()->GetAxis(OS::Input::Axis::MOUSE_POSITION_X),
				Context()->InputModule()->GetAxis(OS::Input::Axis::MOUSE_POSITION_Y)) - viewportRect.start;
			const Size2 requestPosition(
				mousePosition.x >= 0.0f ? static_cast<uint32_t>(mousePosition.x) : (~((uint32_t)0)),
				mousePosition.y >= 0.0f ? static_cast<uint32_t>(mousePosition.y) : (~((uint32_t)0)));
			void(*queryCallback)(Object*, ViewportObjectQuery::Result) = [](Object* selfPtr, ViewportObjectQuery::Result result) {
				SceneView* self = dynamic_cast<SceneView*>(selfPtr);
				std::unique_lock<SpinLock> lock(self->m_hoverResultLock);
				self->m_hoverResult = result;
			};
			dynamic_cast<RenderJob*>(m_updateJob.operator->())->Query()->QueryAsynch(requestPosition, Callback(queryCallback), this);
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
