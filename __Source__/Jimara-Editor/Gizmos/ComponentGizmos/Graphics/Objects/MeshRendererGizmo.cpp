#include "MeshRendererGizmo.h"
#include <Jimara/Environment/LogicSimulation/SimulationThreadBlock.h>


namespace Jimara {
	namespace Editor {
		struct MeshRendererGizmo::Helpers {
			inline static void UpdatePosition(const MeshRendererGizmo* self) {
				const MeshRenderer* const target = self->Target<MeshRenderer>();
				if (target == nullptr || target->Mesh() == nullptr)
					return;
				const Transform* const targetTransform = target->GetTransfrom();
				Transform* const wireTransform = self->m_wireframeRenderer->GetTransfrom();
				if (targetTransform == nullptr) {
					wireTransform->SetLocalPosition(Vector3(0.0f));
					wireTransform->SetLocalEulerAngles(Vector3(0.0f));
					wireTransform->SetLocalScale(Vector3(1.0f));
				}
				else {
					wireTransform->SetLocalPosition(targetTransform->WorldPosition());
					wireTransform->SetLocalEulerAngles(targetTransform->WorldEulerAngles());
					wireTransform->SetLocalScale(targetTransform->LossyScale());
				}
				wireTransform->WorldMatrix();
			}

			inline static void UpdateMesh(const MeshRendererGizmo* self) {
				const MeshRenderer* const target = self->Target<MeshRenderer>();
				if (target == nullptr) {
					self->m_wireframeRenderer->SetMesh(nullptr);
					return;
				}
				else {
					self->m_wireframeRenderer->SetEnabled(target->ActiveInHierarchy());
					self->m_wireframeRenderer->SetMesh(target->Mesh());
				}
			}

			class Updater : public virtual ObjectCache<Reference<const Object>>::StoredObject {
			private:
				const Reference<GizmoScene::Context> m_context;
				const Reference<SimulationThreadBlock> m_block;

				std::recursive_mutex m_lock;
				std::set<Reference<MeshRendererGizmo>> m_gizmos;
				std::vector<MeshRendererGizmo*> m_gizmoList;
				bool m_gizmoListDirty = false;

				void Update() {
					std::unique_lock<std::recursive_mutex> lock(m_lock);
					// Refresh gizmo list if set is dirty:
					auto refreshGizmoList = [&]() {
						m_gizmoList.clear();
						for (auto it = m_gizmos.begin(); it != m_gizmos.end(); ++it)
							m_gizmoList.push_back(*it);
						m_gizmoListDirty = false;
					};
					if (m_gizmoListDirty)
						refreshGizmoList();

					// Update gizmo meshes:
					{
						MeshRendererGizmo* const* const firstGizmo = m_gizmoList.data();
						MeshRendererGizmo* const* const lastGizmo = firstGizmo + m_gizmoList.size();
						for (MeshRendererGizmo* const* ptr = firstGizmo; ptr < lastGizmo; ptr++) {
							MeshRendererGizmo* const gizmo = *ptr;
							if (gizmo->Destroyed()) {
								m_gizmos.erase(gizmo);
								m_gizmoListDirty = true;
							}
							else UpdateMesh(gizmo);
						}
						if (m_gizmoListDirty)
							refreshGizmoList();
					}

					// Update gizmo positions:
					{
						typedef void(*UpdateFn)(ThreadBlock::ThreadInfo, void*);
						static const UpdateFn update = [](ThreadBlock::ThreadInfo info, void* selfPtr) {
							Updater* self = (Updater*)selfPtr;
							const size_t elemsPerThread = (self->m_gizmoList.size() + info.threadCount - 1u) / info.threadCount;
							const MeshRendererGizmo* const* const firstGizmo =
								self->m_gizmoList.data() + (info.threadId * elemsPerThread);
							const MeshRendererGizmo* const* const lastGizmo =
								self->m_gizmoList.data() + Math::Min((info.threadId + 1u) * elemsPerThread, self->m_gizmoList.size());
							for (const MeshRendererGizmo* const* ptr = firstGizmo; ptr < lastGizmo; ptr++)
								UpdatePosition(*ptr);
						};

						static const constexpr size_t elemsPerThread = 64u;
						const size_t blockCount = Math::Min((m_gizmoList.size() + elemsPerThread - 1u) / elemsPerThread, m_block->DefaultThreadCount());
						if (blockCount <= 1u) {
							ThreadBlock::ThreadInfo info = {};
							info.threadCount = 1u;
							info.threadId = 0u;
							update(info, (void*)this);
						}
						else m_block->Execute(blockCount, (void*)this, Callback(update));
					}
				}

			public:
				inline Updater(GizmoScene::Context* context) 
					: m_context(context), m_block(SimulationThreadBlock::GetFor(context->TargetContext())) {
					m_context->GizmoContext()->OnUpdate() += Callback<>(&Updater::Update, this);
				}

				inline virtual ~Updater() {
					m_context->GizmoContext()->OnUpdate() -= Callback<>(&Updater::Update, this);
				}

				inline void OnGizmoCreated(MeshRendererGizmo* gizmo) {
					assert(gizmo != nullptr && gizmo->GizmoContext() == m_context);
					std::unique_lock<std::recursive_mutex> lock(m_lock);
					m_gizmos.insert(gizmo);
					m_gizmoListDirty = true;
				}
			};

			struct UpdaterCache : public virtual ObjectCache<Reference<const Object>> {
				inline static Reference<Updater> Get(GizmoScene::Context* context) {
					static UpdaterCache cache;
					static std::mutex allocationLock;
					std::unique_lock<std::mutex> lock(allocationLock);
					return cache.GetCachedOrCreate(context, [&]() {
						const Reference<Updater> updater = Object::Instantiate<Updater>(context);
						context->GizmoContext()->StoreDataObject(updater);
						return updater;
						});
				}
			};
		};

		MeshRendererGizmo::MeshRendererGizmo(Scene::LogicContext* context) 
			: Component(context, "MeshRendererGizmo")
			, m_wireframeRenderer(
				Object::Instantiate<MeshRenderer>(
					Object::Instantiate<Transform>(this, "MeshRendererGizmo_Transform"), 
					"MeshRendererGizmo_Renderer")) {
			m_wireframeRenderer->SetLayer(static_cast<Layer>(GizmoLayers::WORLD_SPACE));
			m_wireframeRenderer->SetGeometryType(Graphics::GraphicsPipeline::IndexType::EDGE);
			Helpers::UpdaterCache::Get(GizmoContext())->OnGizmoCreated(this);
		}

		MeshRendererGizmo::~MeshRendererGizmo() {}
	}


	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::MeshRendererGizmo>(const Callback<const Object*>& report) {
		static const Reference<const Editor::Gizmo::ComponentConnection> connection = 
			Editor::Gizmo::ComponentConnection::Make<Editor::MeshRendererGizmo, MeshRenderer>();
		report(connection);
	}
}

