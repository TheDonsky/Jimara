#include "NavMeshSurfaceBakeWindow.h"
#include <Jimara/Components/GraphicsObjects/MeshRenderer.h>
#include <Jimara/OS/Input/NoInput.h>
#include <Jimara/OS/IO/FileDialogues.h>
#include <Jimara/Data/Formats/WavefrontOBJ.h>
#include <Jimara/Data/Serialization/Helpers/ComponentHierarchySerializer.h>
#include <Jimara/Data/Serialization/Helpers/SerializeToJson.h>
#include <Jimara-Editor/GUI/ImGuiWrappers.h>
#include <Jimara-Editor/GUI/Utils/DrawSerializedObject.h>
#include <Jimara-Editor/GUI/Utils/DrawObjectPicker.h>

namespace Jimara {
	namespace Editor {
		struct NavMeshSurfaceBakeWindow::Helpers {
			inline static Reference<Scene> CreateScene(Component* rootObj) {
				Scene::CreateArgs createArgs;
				{
					createArgs.logic.logger = rootObj->Context()->Log();
					createArgs.logic.input = Object::Instantiate<OS::NoInput>();
					createArgs.logic.assetDatabase = rootObj->Context()->AssetDB();
				}
				{
					createArgs.graphics.graphicsDevice = rootObj->Context()->Graphics()->Device();
					createArgs.graphics.shaderLibrary = rootObj->Context()->Graphics()->Configuration().ShaderLibrary();
					createArgs.graphics.maxInFlightCommandBuffers = rootObj->Context()->Graphics()->Configuration().MaxInFlightCommandBufferCount();
					createArgs.graphics.bindlessResources.bindlessArrays = rootObj->Context()->Graphics()->Bindless().Buffers();
					createArgs.graphics.bindlessResources.bindlessArrayBindings = rootObj->Context()->Graphics()->Bindless().BufferBinding();
					createArgs.graphics.bindlessResources.bindlessSamplers = rootObj->Context()->Graphics()->Bindless().Samplers();
					createArgs.graphics.bindlessResources.bindlessSamplerBindings = rootObj->Context()->Graphics()->Bindless().SamplerBinding();
					createArgs.graphics.synchPointThreadCount = 1u;
					createArgs.graphics.renderThreadCount = 1u;
				}
				{
					createArgs.physics.physicsInstance = rootObj->Context()->Physics()->APIInstance();
					createArgs.physics.simulationThreadCount = 1u;
					createArgs.physics.sceneFlags = Physics::PhysicsInstance::SceneCreateFlags::NONE;
				}
				{
					createArgs.audio.audioDevice = rootObj->Context()->Audio()->AudioScene()->Device();
				}
				createArgs.createMode = Scene::CreateArgs::CreateMode::ERROR_ON_MISSING_FIELDS;
				return Scene::Create(createArgs);
			}

			inline static bool CopyHierarchy(Component* src, Component* dst) {
				ComponentHierarchySerializerInput srcInput;
				srcInput.rootComponent = src;
				bool error = false;
				nlohmann::json snapshot = Serialization::SerializeToJson(
					ComponentHierarchySerializer::Instance()->Serialize(srcInput), src->Context()->Log(), error,
					[&](const Serialization::SerializedObject&, bool& error) -> nlohmann::json {
						src->Context()->Log()->Error(
							"NavMeshSurfaceBakeWindow::Helpers::CopyHierarchy - ComponentHierarchySerializer is not expected to have any Component references (serialize)!");
						error = true;
						return {};
					});
				if (error) {
					src->Context()->Log()->Error("NavMeshSurfaceBakeWindow::Helpers::CopyHierarchy - Failed to create scene snapshot!");
					return false;
				}

				ComponentHierarchySerializerInput dstInput;
				dstInput.rootComponent = dst;
				if (!Serialization::DeserializeFromJson(
					ComponentHierarchySerializer::Instance()->Serialize(dstInput), snapshot, dst->Context()->Log(),
					[&](const Serialization::SerializedObject&, const nlohmann::json&) -> bool {
						dst->Context()->Log()->Error(
							"NavMeshSurfaceBakeWindow::Helpers::CopyHierarchy - ComponentHierarchySerializer is not expected to have any Component references (deserialize)!");
						return false;
					})) {
					dst->Context()->Log()->Error("NavMeshSurfaceBakeWindow::Helpers::CopyHierarchy - Failed to load scene snapshot!");
					return false;
				}

				return true;
			}
			
			inline static bool IsUnbound(const AABB& bbox) {
				return !(
					std::isfinite(bbox.start.x) && std::isfinite(bbox.end.x) &&
					std::isfinite(bbox.start.y) && std::isfinite(bbox.end.y) &&
					std::isfinite(bbox.start.z) && std::isfinite(bbox.end.z));
			}

			inline static AABB CalculateCombinedBoundaries(Component* rootObj) {
				std::vector<Collider*> colliders = rootObj->GetComponentsInChildren<Collider>(true);
				AABB bounds = AABB(
					Vector3(std::numeric_limits<float>::quiet_NaN()),
					Vector3(std::numeric_limits<float>::quiet_NaN()));
				for (size_t i = 0u; i < colliders.size(); i++) {
					const BoundedObject* boundedObject = dynamic_cast<BoundedObject*>(colliders[i]);
					if (boundedObject == nullptr)
						continue;
					const AABB bnd = boundedObject->GetBoundaries();
					if (IsUnbound(bnd))
						continue;
					if (IsUnbound(bounds))
						bounds = bnd;
					bounds = AABB(
						Vector3(
							Math::Min(bounds.start.x, bnd.start.x, bnd.end.x),
							Math::Min(bounds.start.y, bnd.start.y, bnd.end.y),
							Math::Min(bounds.start.z, bnd.start.z, bnd.end.z)),
						Vector3(
							Math::Max(bounds.end.x, bnd.start.x, bnd.end.x),
							Math::Max(bounds.end.y, bnd.start.y, bnd.end.y),
							Math::Max(bounds.end.z, bnd.start.z, bnd.end.z)));
				}
				return bounds;
			}

			inline static void SerializeSettings(NavMeshSurfaceBakeWindow* self, EditorScene* scene) {
				Reference<Component> rootObject = self->m_root;
				{
					bool found = false;
					for (Component* ptr = rootObject; ptr != nullptr; ptr = ptr->Parent())
						if (ptr == scene->RootObject()) {
							found = true;
							break;
						}
					if (!found)
						rootObject = nullptr;
				}
				self->m_settings.environmentRoot = rootObject;
				static const NavMeshBaker::Settings::Serializer serializer("Settings");
				DrawSerializedObject(serializer.Serialize(self->m_settings), (size_t)self, self->EditorWindowContext()->Log(),
					[&](const Serialization::SerializedObject& object) -> bool {
						const std::string name = CustomSerializedObjectDrawer::DefaultGuiItemName(object, (size_t)self);
						static thread_local std::vector<char> searchBuffer;
						return DrawObjectPicker(object, name, self->EditorWindowContext()->Log(), scene->RootObject(), nullptr, &searchBuffer);
					});
				self->m_root = self->m_settings.environmentRoot;
				if (self->m_settings.environmentRoot == nullptr)
					self->m_settings.environmentRoot = scene->RootObject();
			}

			inline static void StoreMesh(NavMeshSurfaceBakeWindow* self, TriMesh* mesh) {
				if (mesh == nullptr)
					return;
				std::optional<OS::Path> path = OS::SaveDialogue("Save NavMesh geometry", "", {
					OS::FileDialogueFilter("Wavefront OBJ (.obj)", { std::string("*") + ".obj" }) });
				if (!path.has_value())
					return;
				path.value().replace_extension(".obj");
				{
					TriMesh::Writer writer(mesh);
					writer.Name() = OS::Path(path.value().stem());
				}
				StoreAsWavefrontOBJ(path.value(), std::vector<Reference<const TriMesh>>{ mesh });
			}

			struct StateSnapshot {
				NavMeshBaker::State state = NavMeshBaker::State::SURFACE_SAMPLING;
				float stateProgress = 0.0f;
			};

			struct Baker : public virtual Object {
				Reference<Scene> scene;
				NavMeshBaker baker;

				StateSnapshot stateSnapshot;
				SpinLock snapshotLock;

				std::atomic_bool dead = false;
				std::thread process;

				inline Baker(Scene* scn, const NavMeshBaker::Settings& prop)
					: scene(scn), baker(prop) {
					process = std::thread([](Baker* self) {
						while (!self->dead.load()) {
							const NavMeshBaker::State state = self->baker.Progress(1.0f / 60.0f);
							const float stateProgress = self->baker.StateProgress();
							{
								std::unique_lock<SpinLock> lock(self->snapshotLock);
								self->stateSnapshot.state = state;
								self->stateSnapshot.stateProgress = stateProgress;
							}
							if (state == NavMeshBaker::State::UNINITIALIZED ||
								state == NavMeshBaker::State::INVALIDATED ||
								state == NavMeshBaker::State::DONE)
								break;
						}
						}, this);
				}

				inline virtual ~Baker() {
					dead.store(true);
					process.join();
				}
			};

			inline static void BakeIfRequested(NavMeshSurfaceBakeWindow* self) {
				if (!Button("Bake ### NavMeshSurfaceBakeWindow_Bake"))
					return;

				const Reference<Scene> targetScene = Helpers::CreateScene(self->m_settings.environmentRoot);
				if (targetScene == nullptr)
					return;

				if (!Helpers::CopyHierarchy(self->m_settings.environmentRoot, targetScene->RootObject()))
					return;

				for (size_t i = 0u; i < 2u; i++)
					targetScene->Update(2.0f / Math::Max(targetScene->Context()->Physics()->UpdateRate(), 1.0f));

				self->m_settings.environmentRoot = targetScene->RootObject();
				const AABB bounds = Helpers::CalculateCombinedBoundaries(self->m_settings.environmentRoot);
				if (Helpers::IsUnbound(bounds))
					return;

				self->m_settings.volumePose = Math::Identity();
				self->m_settings.volumePose[3u] = Vector4(0.5f * (bounds.start + bounds.end), 1.0f);
				self->m_settings.volumeSize = (bounds.end - bounds.start) * 1.01f;

				self->m_bakeProcess = Object::Instantiate<Baker>(targetScene, self->m_settings);
			}
		};

		NavMeshSurfaceBakeWindow::NavMeshSurfaceBakeWindow(EditorContext* context) 
			: EditorWindow(context, "Bake NavMesh Surface") {
			assert(context != nullptr);
		}

		NavMeshSurfaceBakeWindow::~NavMeshSurfaceBakeWindow() {}

		void NavMeshSurfaceBakeWindow::DrawEditorWindow() {
			Reference<EditorScene> scene = EditorWindowContext()->GetScene();
			Reference<Helpers::Baker> baker = m_bakeProcess;
			if (baker != nullptr) {
				Helpers::StateSnapshot state;
				{
					std::unique_lock<SpinLock> lock(baker->snapshotLock);
					state = baker->stateSnapshot;
				}
				if (state.state == NavMeshBaker::State::INVALIDATED) {
					m_bakeProcess = nullptr;
					return;
				}

				if (state.state != NavMeshBaker::State::DONE) {
					auto message = [&](const std::string_view& stateText) {
						std::stringstream stream;
						stream << stateText << "... [" << (state.stateProgress * 100.0f) << "%]";
						const std::string text = stream.str();
						Label(text);
					};
					switch (state.state) {
					case NavMeshBaker::State::UNINITIALIZED:
						m_bakeProcess = nullptr;
						break;
					case NavMeshBaker::State::INVALIDATED:
						EditorWindowContext()->Log()->Error("NavMeshSurfaceBakeWindow::DrawEditorWindow - Failed to generate NavMesh surface!");
						m_bakeProcess = nullptr;
						break;
					case NavMeshBaker::State::SURFACE_SAMPLING:
						message("Sampling geometry");
						break;
					case NavMeshBaker::State::MESH_GENERATION:
						message("Generating mesh");
						break;
					case NavMeshBaker::State::MESH_SMOOTHING:
						message("Smoothing mesh");
						break;
					case NavMeshBaker::State::MESH_SIMPLIFICATION:
						message("Simplifying mesh");
						break;
					default:
						break;
					}
					return;
				}

				Reference<TriMesh> mesh = baker->baker.Result();
				m_bakeProcess = nullptr;

				if (mesh == nullptr)
					return;
				Helpers::StoreMesh(this, mesh);

				if (scene == nullptr)
					return;
				std::unique_lock<std::recursive_mutex> lock(scene->UpdateLock());
				const Reference<Transform> transform = Object::Instantiate<Transform>(scene->RootObject(), "NavMesh shape");
				transform->SetLocalPosition(Vector3(0.0f, 0.1f, 0.0f));
				Object::Instantiate<MeshRenderer>(transform)->SetMesh(mesh);
			}
			else {
				if (scene == nullptr)
					return;
				std::unique_lock<std::recursive_mutex> lock(scene->UpdateLock());
				Helpers::SerializeSettings(this, scene);
				Helpers::BakeIfRequested(this);
			}
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<Editor::NavMeshSurfaceBakeWindow>(const Callback<const Object*>& report) {
		Reference<Editor::EditorMainMenuCallback> createAction = Object::Instantiate<Editor::EditorMainMenuCallback>(
			"State Machines/Navigation/Bake NavMesh Surface", "Utility for baking navigation mesh geometry",
			Callback<Editor::EditorContext*>([](Editor::EditorContext* context) {
				Object::Instantiate<Editor::NavMeshSurfaceBakeWindow>(context);
			}));
		report(createAction);
	}
}
