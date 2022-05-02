#include "RenderStack.h"
#include <algorithm>


namespace Jimara {
	namespace {
		struct RendererListEntry {
			Reference<RenderStack::Renderer> renderer;
			uint64_t priority = 0;
			inline RendererListEntry() {}
			inline RendererListEntry(RenderStack::Renderer* r) : renderer(r) {
				if (r != nullptr) priority =
					(static_cast<uint64_t>(r->Category()) << 32) |
					static_cast<uint64_t>((~uint32_t(0)) - r->Priority());
			}
			inline bool operator<(const RendererListEntry& other)const {
				return priority < other.priority || (priority == other.priority && renderer < other.renderer);
			}
		};
	}

	struct RenderStack::Data : public virtual Object {
		// Weak pointer to data:
		class WeakPtr {
		private:
			mutable SpinLock m_lock;
			Data* m_addr = nullptr;

		public:
			inline WeakPtr& operator=(Data* stack) {
				std::unique_lock<SpinLock> lock(m_lock);
				m_addr = stack;
				return *this;
			}

			inline operator Reference<Data>()const {
				std::unique_lock<SpinLock> lock(m_lock);
				Reference<Data> ref = m_addr;
				return ref;
			}
		};





		// Set of renderers, added to RenderStack
		struct RendererSet : public virtual Object {
			const Reference<Data> owner;
			ObjectSet<Renderer> renderers;

			inline RendererSet(RenderStack::Data* data) : owner(data) {}
			inline virtual void OnOutOfScope()const final override {
				{
					std::unique_lock<std::recursive_mutex> lock(owner->stateLock);
					if (RefCount() > 0) return;
					owner->rendererSet = nullptr;
				}
				Object::OnOutOfScope();
			}
		};





		// Actual render job:
		struct RenderJob : public virtual JobSystem::Job {
			const Reference<Scene::GraphicsContext> graphics;
			Reference<RenderImages> images;
			std::vector<RendererListEntry> renderers;

			inline RenderJob(Scene::GraphicsContext* context) : graphics(context) {}

			inline virtual void Execute() final override {
				if (images == nullptr) return;
				
				size_t count = renderers.size();
				if (count <= 0) {
					graphics->RenderJobs().Remove(this);
					return;
				}

				const Graphics::Pipeline::CommandBufferInfo commandBufferInfo = graphics->GetWorkerThreadCommandBuffer();
				for (size_t i = 0; i < count; i++)
					renderers[i].renderer->Render(commandBufferInfo, images);
			}

			inline virtual void CollectDependencies(Callback<Job*> addDependency) final override {
				size_t count = renderers.size();
				for (size_t i = 0; i < count; i++)
					renderers[i].renderer->GetDependencies(addDependency);
			}
		};





		// Synch point job for preparing the renderer
		struct SynchJob : public virtual JobSystem::Job {
			WeakPtr owner;

			inline virtual void Execute() final override {
				// Obtain strong data reference:
				const Reference<Data> data = owner;
				if (data == nullptr) return;
				std::unique_lock<std::recursive_mutex> stateLock(data->stateLock);
				
				// If dead, doubly and triply make sure the RendererSet is empty:
				if (data->dead.load() && data->rendererSet != nullptr) {
					data->rendererSet->renderers.Clear();
					data->sceneContext->EraseDataObject(data->rendererSet);
					data->rendererSet = nullptr;
				}

				// Update RenderJob images:
				{
					std::unique_lock<SpinLock> renderImageLock(data->renderImageLock);
					if (data->renderJob->images == nullptr ||
						data->renderJob->images->Resolution() != data->resolution ||
						data->renderJob->images->SampleCount() != data->sampleCount) {
						if (data->resolution == Size2(0, 0)) data->renderJob->images = nullptr;
						else data->renderJob->images =
							Object::Instantiate<RenderImages>(data->sceneContext->Graphics()->Device(), data->resolution, data->sampleCount);
					}
				}

				// Update RenderJob storage:
				const bool renderJobHadTasks = (data->renderJob->renderers.size() > 0);
				data->renderJob->renderers.clear();
				if (data->rendererSet != nullptr)
					for (size_t i = 0; i < data->rendererSet->renderers.Size(); i++)
						data->renderJob->renderers.push_back(RendererListEntry(data->rendererSet->renderers[i]));
				if (data->renderJob->renderers.size() > 0) {
					std::sort(data->renderJob->renderers.begin(), data->renderJob->renderers.end());
					if (!renderJobHadTasks) data->renderJob->graphics->RenderJobs().Add(data->renderJob);
				}
				else if (renderJobHadTasks) data->renderJob->graphics->RenderJobs().Remove(data->renderJob);
			}

			inline virtual void CollectDependencies(Callback<Job*>) final override {}
		};





		// Data variables:
		const Reference<Scene::LogicContext> sceneContext;
		const Reference<SynchJob> synchJob;
		const Reference<RenderJob> renderJob;
		std::atomic<bool> dead = false;

		mutable std::recursive_mutex stateLock;
		mutable RendererSet* rendererSet = nullptr;

		SpinLock renderImageLock;
		Size2 resolution = Size2(1920, 1080);
		Graphics::Texture::Multisampling sampleCount = Graphics::Texture::Multisampling::MAX_AVAILABLE;

		inline Data(Scene::LogicContext* context) 
			: sceneContext(context)
			, synchJob(Object::Instantiate<SynchJob>())
			, renderJob(Object::Instantiate<RenderJob>(context->Graphics())) {
			sampleCount = min(Graphics::Texture::Multisampling::MAX_AVAILABLE, context->Graphics()->Device()->PhysicalDevice()->MaxMultisapling());
			synchJob->owner = this;
			sceneContext->Graphics()->SynchPointJobs().Add(synchJob);
		}
		inline void Cleanup()const {
			std::unique_lock<std::recursive_mutex> lock(stateLock);
			synchJob->owner = nullptr;
			sceneContext->Graphics()->SynchPointJobs().Remove(synchJob);
			sceneContext->Graphics()->RenderJobs().Remove(renderJob);
			if (rendererSet != nullptr) {
				rendererSet->renderers.Clear();
				sceneContext->EraseDataObject(rendererSet);
				rendererSet = nullptr;
			}
		}
		inline virtual ~Data() {
			if (!dead.load())
				sceneContext->Log()->Error("RenderStack::Data::~Data - [Internal error] Data destroyed, but not marked dead! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			Cleanup();
		}
		inline virtual void OnOutOfScope()const override {
			Cleanup();
			if (RefCount() <= 0) Object::OnOutOfScope();
		}
		inline static Data* Of(const RenderStack* stack) { return dynamic_cast<Data*>(stack->m_data.operator->()); }
	};

	namespace {
		class RenderStack_Cache : public virtual ObjectCache<Reference<Object>> {
		private:
#pragma warning(disable: 4250)
			class CachedRenderStack 
				: public virtual RenderStack
				, public virtual ObjectCache<Reference<Object>>::StoredObject {
			public:
				inline CachedRenderStack(Scene::LogicContext* context) : RenderStack(context) {}
				inline virtual ~CachedRenderStack() {}
			};
#pragma warning(default: 4250)
		public:
			inline static Reference<RenderStack> GetMain(Scene::LogicContext* context) {
				if (context == nullptr) return nullptr;
				static RenderStack_Cache cache;
				return cache.GetCachedOrCreate(context, false, [&]() -> Reference<CachedRenderStack> {
					return Object::Instantiate<CachedRenderStack>(context);
					});
			}
		};
	}

	Reference<RenderStack> RenderStack::Main(Scene::LogicContext* context) {
		return RenderStack_Cache::GetMain(context);
	}

	RenderStack::RenderStack(Scene::LogicContext* context, Size2 initialResolution) 
		: m_data(Object::Instantiate<Data>(context)) {
		SetResolution(initialResolution);
	}

	RenderStack::~RenderStack() { Data::Of(this)->dead = true; }

	Size2 RenderStack::Resolution()const {
		Data* data = Data::Of(this);
		std::unique_lock<SpinLock> lock(data->renderImageLock);
		Size2 resolution = data->resolution;
		return resolution;
	}

	void RenderStack::SetResolution(Size2 resolution) {
		Data* data = Data::Of(this);
		std::unique_lock<SpinLock> lock(data->renderImageLock);
		data->resolution = resolution;
	}

	Reference<RenderImages> RenderStack::Images()const {
		Data* data = Data::Of(this);
		std::unique_lock<SpinLock> lock(data->renderImageLock);
		Reference<RenderImages> images = data->renderJob->images;
		return images;
	}

	void RenderStack::AddRenderer(Renderer* renderer) {
		if (renderer == nullptr) return;
		Data* data = Data::Of(this);
		std::unique_lock<std::recursive_mutex> stateLock(data->stateLock);
		Reference<Data::RendererSet> set = data->rendererSet;
		if (set == nullptr) {
			set = Object::Instantiate<Data::RendererSet>(data);
			data->rendererSet = set;
			data->sceneContext->StoreDataObject(data->rendererSet);
		}
		set->renderers.Add(renderer);
	}

	void RenderStack::RemoveRenderer(Renderer* renderer) {
		Data* data = Data::Of(this);
		std::unique_lock<std::recursive_mutex> stateLock(data->stateLock);
		if (data->rendererSet == nullptr) return;
		data->rendererSet->renderers.Remove(renderer);
		if (data->rendererSet->renderers.Size() <= 0) {
			data->sceneContext->EraseDataObject(data->rendererSet);
			data->rendererSet = nullptr;
		}
	}
}
