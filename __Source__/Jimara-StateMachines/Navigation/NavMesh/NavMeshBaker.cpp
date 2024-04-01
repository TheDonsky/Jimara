#include "NavMeshBaker.h"
#include <Jimara/Core/Stopwatch.h>
#include <Jimara/Data/Geometry/MeshModifiers.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>
#include <Jimara/Data/Serialization/Attributes/SliderAttribute.h>


namespace Jimara {
	struct NavMeshBaker::Helpers {
		using HitList = Stacktor<Vector3, 8u>;
		struct SamplingState {
			size_t sampleIndex = 0u;
			std::vector<HitList> floorSamples;
			std::vector<HitList> roofSamples;
		};

		struct SurfaceFilteringState {
			size_t sampleIndex = 0u;
			std::vector<HitList> filteredFloorSamples;
		};

		struct MeshGenerationState {
			size_t sampleIndex = 0u;
			Reference<TriMesh> mesh;
		};

		struct MeshCleanupState {
			size_t angleIndex = 0u;
			Reference<TriMesh> mesh;
		};

		struct ProcessSettings : public Settings {
			Size2 verticalSampleCount = Size2(0u);

			inline ProcessSettings(const Settings& settings) : Settings(settings) {}
		};

		struct Process : public virtual Object {
			const ProcessSettings settings;
			State state = State::SCENE_SAMPLING;

			const WeakReference<Component> rootObj;
			SamplingState samplingState;
			SurfaceFilteringState filteringState;
			MeshGenerationState meshGenerationState;
			MeshCleanupState meshCleanupState;

			inline Process(const ProcessSettings& s)
				: settings(s), rootObj(s.environmentRoot) {}
			inline ~Process() {}
		};

		inline static Vector2 SampleSize(const Process* proc) {
			return Vector2(
				proc->settings.volumeSize.x / proc->settings.verticalSampleCount.x,
				proc->settings.volumeSize.z / proc->settings.verticalSampleCount.y);
		}

		inline static void SampleScene(Process* proc) {
			assert(proc->state == State::SCENE_SAMPLING);
			const size_t totalSampleCount = size_t(proc->settings.verticalSampleCount.x) * proc->settings.verticalSampleCount.y;
			assert(proc->samplingState.sampleIndex < totalSampleCount);

			Reference<Component> sceneRoot = proc->rootObj;
			if (sceneRoot == nullptr || sceneRoot->Destroyed()) {
				proc->state = State::INVALIDATED;
				return;
			}

			const float cosineThreshold = std::cos(Math::Radians(proc->settings.maxSlopeAngle));
			const Vector2 sampleSize = SampleSize(proc);
			//const Physics::BoxShape shape(Vector3(sampleSize.x, proc->settings.maxStepDistance, sampleSize.y));

			const Vector3 scaledDownDir = proc->settings.volumePose * Vector4(Math::Down(), 0.0f);
			const float yScale = Math::Magnitude(scaledDownDir);
			if (yScale <= std::numeric_limits<float>::epsilon()) {
				proc->state = State::INVALIDATED;
				return;
			}
			const Vector3 down = scaledDownDir / yScale;
			const float volumeSizeY = yScale * std::abs(proc->settings.volumeSize.y);
			const float maxDistance = volumeSizeY + proc->settings.maxStepDistance;

			auto getLocalPose = [&](const Vector3& localPosition) {
				Matrix4 localPose = Math::Identity();
				localPose[3u] = Vector4(localPosition, 1.0f);
				return proc->settings.volumePose * localPose;
			};

			const size_t sampleIdY = (proc->samplingState.sampleIndex / proc->settings.verticalSampleCount.x);
			const size_t sampleIdX = proc->samplingState.sampleIndex - (sampleIdY * proc->settings.verticalSampleCount.x);
			const Vector2 localSamplePos =
				Vector2(sampleSize.x * sampleIdX, sampleSize.y * sampleIdY) -
				(0.5f * Vector2(proc->settings.volumeSize.x, proc->settings.volumeSize.z));

			const float localYOffset = (std::abs(proc->settings.volumeSize.y) + proc->settings.maxStepDistance / yScale) * 0.5f;
			const Matrix4 topPose = getLocalPose(Vector3(localSamplePos.x, localYOffset, localSamplePos.y));
			const Matrix4 bottomPose = getLocalPose(Vector3(localSamplePos.x, -localYOffset, localSamplePos.y));

			auto appendSample = [&](HitList& hits, const RaycastHit& hit, const Matrix4& pose, const Vector3& dir) {
				const Vector3 origin = pose[3u];
				const Vector3 delta = hit.point - origin;
				const float distance = Math::Dot(delta, dir);
				hits.Push(origin + dir * distance);
			};

			HitList& surfaceHits = proc->samplingState.floorSamples.emplace_back();
			const auto onSurfaceHitFound = [&](const RaycastHit& hit) {
				const float slopeCosine = Math::Dot(-down, hit.normal);
				if (slopeCosine < cosineThreshold)
					return;
				appendSample(surfaceHits, hit, topPose, down);
			};

			HitList& roofHits = proc->samplingState.roofSamples.emplace_back();
			const auto onRoofHitFound = [&](const RaycastHit& hit) {
				const float slopeCosine = Math::Dot(down, hit.normal);
				if (slopeCosine < 0.0)
					return;
				appendSample(roofHits, hit, bottomPose, -down);
			};

			// We only care about colliders under the root:
			const auto preFilter = [&](Collider* collider) {
				if (collider != nullptr && !collider->IsTrigger())
					for (Component* ptr = collider; ptr != nullptr; ptr = ptr->Parent())
						if (ptr == proc->settings.environmentRoot)
							return Physics::PhysicsScene::QueryFilterFlag::REPORT;
				return Physics::PhysicsScene::QueryFilterFlag::DISCARD;
			};
			const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*> preFilterCall =
				Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>::FromCall(&preFilter);

			// Raycast for ceiling:
			proc->settings.environmentRoot->Context()->Physics()->Raycast(
				topPose[3u], down, maxDistance,
				Callback<const RaycastHit&>::FromCall(&onSurfaceHitFound),
				proc->settings.surfaceLayers,
				(Physics::PhysicsScene::QueryFlags)Physics::PhysicsScene::QueryFlag::REPORT_MULTIPLE_HITS,
				&preFilterCall);

			// Raycast for roof:
			proc->settings.environmentRoot->Context()->Physics()->Raycast(
				bottomPose[3u], -down, maxDistance,
				Callback<const RaycastHit&>::FromCall(&onRoofHitFound),
				proc->settings.roofLayers,
				(Physics::PhysicsScene::QueryFlags)Physics::PhysicsScene::QueryFlag::REPORT_MULTIPLE_HITS,
				&preFilterCall);

			// Advance state:
			proc->samplingState.sampleIndex++;
			if (proc->samplingState.sampleIndex < totalSampleCount)
				return;
			proc->state = State::SURFACE_FILTERING;
		}

		inline static void FilterSurface(Process* proc) {
			assert(proc->state == State::SURFACE_FILTERING);
			const size_t totalSampleCount = size_t(proc->settings.verticalSampleCount.x) * proc->settings.verticalSampleCount.y;
			assert(proc->samplingState.floorSamples.size() == totalSampleCount);
			assert(proc->samplingState.roofSamples.size() == totalSampleCount);
			assert(proc->filteringState.sampleIndex < totalSampleCount);
			
			Reference<Component> sceneRoot = proc->rootObj;
			if (sceneRoot == nullptr || sceneRoot->Destroyed()) {
				proc->state = State::INVALIDATED;
				return;
			}

			const Vector3 up = Math::Normalize(Vector3(proc->settings.volumePose * Vector4(Math::Up(), 0.0f)));
			const Vector2 sampleSize = SampleSize(proc);
			const float overlapOffset = Math::Min(sampleSize.x, sampleSize.y);
			const Physics::SphereShape sampleSphere = Physics::SphereShape(
				(overlapOffset * 0.5f) * std::cos(Math::Radians(proc->settings.maxSlopeAngle)));
			
			// Sort floor hits:
			HitList sortedHits = proc->samplingState.floorSamples[proc->filteringState.sampleIndex];
			if (sortedHits.Size() > 0u)
				std::sort(sortedHits.Data(), sortedHits.Data() + sortedHits.Size(),
					[&](const Vector3& a, const Vector3& b) {
						return Math::Dot(a, up) < Math::Dot(b, up);
					});

			// Merge hits that are 'close enough' to each other:
			HitList mergedHits;
			if (sortedHits.Size() > 0u) {
				mergedHits.Push(sortedHits[0u]);
				for (size_t i = 0u; i < sortedHits.Size(); i++) {
					Vector3& last = mergedHits[mergedHits.Size() - 1u];
					const Vector3 cur = sortedHits[i];
					assert(Math::Dot(last, up) <= Math::Dot(cur, up));
					const float dist = Math::Magnitude(last - cur);
					if (dist <= proc->settings.maxMergeDistance)
						last = cur;
					else mergedHits.Push(cur);
				}
			}

			// Filter-out hits that have roof on top too close for the agents to fit, or the ones that are simply inside colliders:
			const HitList& roofHits = proc->samplingState.roofSamples[proc->filteringState.sampleIndex];
			HitList& filteredHits = proc->filteringState.filteredFloorSamples.emplace_back();
			for (size_t i = 0u; i < mergedHits.Size(); i++) {
				const Vector3 floor = mergedHits[i];
				const float height = Math::Dot(floor, up);

				auto sampleObstructsFloor = [&](const Vector3& sample) {
					const float rh = Math::Dot(sample, up);
					const float delta = (rh - height);
					return (delta >= 0.0f && delta < proc->settings.minAgentHeight);
				};
				
				// Check top floor sample:
				const bool hasTopFloorTooClose = [&]() -> bool {
					const size_t j = (i + 1u);
					if (j < mergedHits.Size())
						if (sampleObstructsFloor(mergedHits[j]))
							return true;
					return false;
				}();
				if (hasTopFloorTooClose)
					continue;

				// Check roof samples:
				const bool hasRoofTooClose = [&]() -> bool {
					for (size_t j = 0u; j < roofHits.Size(); j++)
						if (sampleObstructsFloor(roofHits[j]))
							return true;
					return false;
				}();
				if (hasRoofTooClose)
					continue;

				// Check overlap:
				Matrix4 overlapPose = Math::Identity();
				overlapPose[3u] = Vector4(floor + up * overlapOffset, 1.0f);
				bool sampleInsideCollider = false;
				auto onOverlapFound = [&](Collider*) { sampleInsideCollider = true; };
				const auto filter = [&](Collider* collider) {
					if (collider != nullptr && !collider->IsTrigger())
						for (Component* ptr = collider; ptr != nullptr; ptr = ptr->Parent())
							if (ptr == proc->settings.environmentRoot)
								return Physics::PhysicsScene::QueryFilterFlag::REPORT;
					return Physics::PhysicsScene::QueryFilterFlag::DISCARD;
				};
				const Function<Physics::PhysicsScene::QueryFilterFlag, Collider*> filterCall =
					Function<Physics::PhysicsScene::QueryFilterFlag, Collider*>::FromCall(&filter);
				sceneRoot->Context()->Physics()->Overlap(sampleSphere, overlapPose,
					Callback<Collider*>::FromCall(&onOverlapFound),
					proc->settings.roofLayers | proc->settings.surfaceLayers,
					0u, &filterCall);
				if (sampleInsideCollider)
					continue;
				
				// All checks passed.. We can include this sample:
				filteredHits.Push(floor);
			}

			// Advance state:
			proc->filteringState.sampleIndex++;
			if (proc->filteringState.sampleIndex < totalSampleCount)
				return;
			proc->state = State::MESH_GENERATION;
		}

		inline static void GenerateSampleMesh(Process* proc) {
			assert(proc->state == State::MESH_GENERATION);
			assert(proc->meshGenerationState.mesh == nullptr);

			proc->meshGenerationState.mesh = Object::Instantiate<TriMesh>("Navigation Mesh Samples");
			TriMesh::Writer mesh(proc->meshGenerationState.mesh);

			const Vector2 tileSize = SampleSize(proc);
			const Vector3 up = Math::Normalize(Vector3(proc->settings.volumePose * Vector4(Math::Up(), 0.0f)));
			const Vector3 right = Math::Normalize(Vector3(proc->settings.volumePose * Vector4(Math::Right(), 0.0f)));
			const Vector3 forward = Math::Normalize(Vector3(proc->settings.volumePose * Vector4(Math::Forward(), 0.0f)));

			for (uint32_t y = 0u; y < proc->settings.verticalSampleCount.y; y++)
				for (uint32_t x = 0u; x < proc->settings.verticalSampleCount.x; x++) {
					const size_t columnIndex = size_t(y) * proc->settings.verticalSampleCount.x + x;
					const HitList& columnHits = proc->filteringState.filteredFloorSamples[columnIndex];
					for (size_t i = 0u; i < columnHits.Size(); i++) {
						const Vector3 pos = columnHits[i];
						uint32_t baseIndex = mesh.VertCount();
						auto corner = [&](float r, float f) -> MeshVertex {
							return MeshVertex(
								pos + (tileSize.x * right * r) + (tileSize.y * forward * f), up,
								Vector2(
									(x + 0.5f + r) / proc->settings.verticalSampleCount.x,
									(y + 0.5f + f) / proc->settings.verticalSampleCount.y));
						};
						mesh.AddVert(corner(-0.5f, -0.5f));
						mesh.AddVert(corner(0.5f, -0.5f));
						mesh.AddVert(corner(0.5f, 0.5f));
						mesh.AddVert(corner(-0.5f, 0.5f));
						mesh.AddFace(TriangleFace(baseIndex, baseIndex + 1u, baseIndex + 2u));
						mesh.AddFace(TriangleFace(baseIndex, baseIndex + 2u, baseIndex + 3u));
					}
				}

			proc->state = State::MESH_CLEANUP;
		}

		inline static void GenerateMesh(Process* proc) {
			assert(proc->state == State::MESH_GENERATION);
			const size_t totalSampleCount = size_t(proc->settings.verticalSampleCount.x) * proc->settings.verticalSampleCount.y;
			assert(proc->filteringState.filteredFloorSamples.size() == totalSampleCount);
			const Size2 cornerCount = proc->settings.verticalSampleCount - 3u;
			const size_t totalCornerCount = size_t(cornerCount.x) * cornerCount.y;
			assert(proc->meshGenerationState.sampleIndex < totalCornerCount);

			const size_t cornerY = proc->meshGenerationState.sampleIndex / cornerCount.x + 1u;
			const size_t cornerX = proc->meshGenerationState.sampleIndex - ((cornerY - 1u) * cornerCount.x) + 1u;

			if (proc->meshGenerationState.mesh == nullptr)
				proc->meshGenerationState.mesh = Object::Instantiate<TriMesh>("Unfiltered Navigation Mesh");
			TriMesh::Writer mesh(proc->meshGenerationState.mesh);

			const Vector3 up = Math::Normalize(Vector3(proc->settings.volumePose * Vector4(Math::Up(), 0.0f)));
			
			auto getSamples = [&](size_t x, size_t y) -> const HitList& {
				return proc->filteringState.filteredFloorSamples[y * proc->settings.verticalSampleCount.x + x];
			};
			
			auto findClosestSample = [&](const HitList& samples, const Vector3& a) -> std::optional<Vector3> {
				std::optional<Vector3> result;
				for (size_t i = 0u; i < samples.Size(); i++) {
					auto dist = [&](const Vector3& pos) {
						return std::abs(Math::Dot(a, up) - Math::Dot(pos, up));
						};
					const Vector3 s = samples[i];
					const float distance = dist(s);
					if (distance > proc->settings.maxStepDistance)
						continue;
					if (result.has_value()) {
						const float lastDistance = dist(result.value());
						if (lastDistance < distance)
							continue;
					}
					result = s;
				}
				return result;
			};

			auto hasAllCloseSamplesAround = [&](const const Vector3& a, size_t x, size_t y) {
				assert(x > 0u);
				assert(y > 0u);
				assert(x < (proc->settings.verticalSampleCount.x - 1u));
				assert(y < (proc->settings.verticalSampleCount.y - 1u));
				for (size_t i = x - 1u; i <= x + 1u; i++)
					for (size_t j = y - 1u; j < y + 1u; j++)
						if (!findClosestSample(getSamples(i, j), a).has_value())
							return false;
				return true;
			};

			const HitList& samplesA = getSamples(cornerX, cornerY);
			const HitList& samplesB = getSamples(cornerX + 1u, cornerY);
			const HitList& samplesC = getSamples(cornerX + 1u, cornerY + 1u);
			const HitList& samplesD = getSamples(cornerX, cornerY + 1u);

			for (size_t aI = 0u; aI < samplesA.Size(); aI++) {
				const Vector3 a = samplesA[aI];

				const std::optional<Vector3> bOpt = findClosestSample(samplesB, a);
				if (!bOpt.has_value())
					continue;
				const std::optional<Vector3> cOpt = findClosestSample(samplesC, a);
				if (!cOpt.has_value())
					continue;
				const std::optional<Vector3> dOpt = findClosestSample(samplesD, a);
				if (!dOpt.has_value())
					continue;
				const Vector3& b = bOpt.value();
				const Vector3& c = cOpt.value();
				const Vector3& d = dOpt.value();

				const bool includeA = hasAllCloseSamplesAround(a, cornerX, cornerY);
				const bool includeB = hasAllCloseSamplesAround(b, cornerX + 1u, cornerY);
				const bool includeC = hasAllCloseSamplesAround(c, cornerX + 1u, cornerY + 1u);
				const bool includeD = hasAllCloseSamplesAround(d, cornerX, cornerY + 1u);

				// __TODO__: Check if the vertices can interconnect....

				auto getNormal = [](const Vector3& a, const Vector3& b, const Vector3& c) {
					return Math::Normalize(Math::Cross(c - a, b - a));
				};
				const Vector3 normalA = getNormal(a, b, c);
				const Vector3 normalB = getNormal(a, c, d);
				const Vector3 normal = Math::Normalize(normalA + normalB);
				auto corner = [&](const Vector3& pos) -> MeshVertex {
					return MeshVertex(pos, normal, Vector2(0.0f));
				};
				const uint32_t indexA = mesh.VertCount();
				if (includeA)
					mesh.AddVert(corner(a));
				const uint32_t indexB = mesh.VertCount();
				if (includeB)
					mesh.AddVert(corner(b));
				const uint32_t indexC = mesh.VertCount();
				if (includeC)
					mesh.AddVert(corner(c));
				const uint32_t indexD = mesh.VertCount();
				if (includeD)
					mesh.AddVert(corner(d));

				if (includeA) {
					if (includeC) {
						if (includeB)
							mesh.AddFace(TriangleFace(indexA, indexB, indexC));
						if (includeD)
							mesh.AddFace(TriangleFace(indexA, indexC, indexD));
					}
					else if (includeB && includeD)
						mesh.AddFace(TriangleFace(indexA, indexB, indexD));
				}
				else if (includeB && includeC && includeD)
					mesh.AddFace(TriangleFace(indexB, indexC, indexD));
			}

			// Advance state:
			proc->meshGenerationState.sampleIndex++;
			if (proc->meshGenerationState.sampleIndex < totalCornerCount)
				return;
			proc->state = State::MESH_CLEANUP;
		}

		inline static void CleanupMesh(Process* proc) {
			assert(proc->state == State::MESH_CLEANUP);
			assert(proc->meshGenerationState.mesh != nullptr);
			if (proc->meshCleanupState.mesh == nullptr) {
				proc->meshCleanupState.mesh = ModifyMesh::ShadeSmooth(proc->meshGenerationState.mesh, true, "Navigation Mesh");
				proc->meshCleanupState.angleIndex = 1u;
			}
			const size_t angleSteps = Math::Max(proc->settings.simplificationSubsteps, size_t(1u));
			const Reference<TriMesh> reducedMesh = //proc->resultMesh;
				ModifyMesh::SimplifyMesh(proc->meshCleanupState.mesh, 
					proc->settings.simplificationAngleThreshold / angleSteps * proc->meshCleanupState.angleIndex,
					1u, "Navigation Mesh");
			if (TriMesh::Reader(reducedMesh).VertCount() != TriMesh::Reader(proc->meshCleanupState.mesh).VertCount()) {
				proc->meshCleanupState.mesh = reducedMesh;
				return;
			}
			else if (proc->meshCleanupState.angleIndex >= angleSteps)
				proc->state = State::DONE;
			else proc->meshCleanupState.angleIndex++;
		}

		inline static void PerformStep(Process* proc) {
			if (proc->state == State::UNINITIALIZED || 
				proc->state == State::INVALIDATED ||
				proc->state == State::DONE)
				return;

			switch (proc->state) {
			case State::UNINITIALIZED:
				break;
			case State::INVALIDATED:
				break;
			case State::SCENE_SAMPLING:
				SampleScene(proc);
				break;
			case State::SURFACE_FILTERING:
				FilterSurface(proc);
				break;
			case State::MESH_GENERATION:
				GenerateMesh(proc);
				break;
			case State::MESH_CLEANUP:
				CleanupMesh(proc);
				break;
			default:
				break;
			}
		}

		inline static Reference<Process> CreateState(ProcessSettings settings) {
			if (settings.environmentRoot == nullptr)
				return nullptr;

			settings.maxStepDistance = Math::Max(settings.maxStepDistance, std::numeric_limits<float>::epsilon());
			settings.maxMergeDistance = Math::Max(settings.maxMergeDistance, std::numeric_limits<float>::epsilon());

			const Vector3 scale(
				Math::Magnitude(Vector3(settings.volumePose[0u])),
				Math::Magnitude(Vector3(settings.volumePose[1u])),
				Math::Magnitude(Vector3(settings.volumePose[2u])));
			settings.volumePose[0u] /= scale.x;
			settings.volumePose[1u] /= scale.y;
			settings.volumePose[2u] /= scale.z;
			settings.volumeSize *= scale;

			settings.verticalSampleInterval.x *= scale.x;
			settings.verticalSampleInterval.y *= scale.z;

			settings.verticalSampleCount = Vector2(
				settings.volumeSize.x / Math::Max(settings.verticalSampleInterval.x, std::numeric_limits<float>::epsilon()),
				settings.volumeSize.z / Math::Max(settings.verticalSampleInterval.y, std::numeric_limits<float>::epsilon()));
			settings.verticalSampleCount.x = Math::Max(Math::Min(settings.verticalSampleCount.x, 100000u), 3u);
			settings.verticalSampleCount.y = Math::Max(Math::Min(settings.verticalSampleCount.y, 100000u), 3u);
			settings.verticalSampleCount += 1u;

			const Vector3 volumeDelta = Vector3(
				settings.volumeSize.x / settings.verticalSampleCount.x,
				0.0f, 
				settings.volumeSize.y / settings.verticalSampleCount.y);
			settings.volumeSize += volumeDelta;
			settings.volumePose[4u] += Vector4(volumeDelta * 0.5f, 0.0f);

			return Object::Instantiate<Helpers::Process>(settings);
		}

		inline static Process* GetState(const NavMeshBaker* baker) {
			return dynamic_cast<Process*>(baker->m_state.operator Jimara::Object * ());
		}
	};

	NavMeshBaker::NavMeshBaker(const Settings& settings)
		: m_state(Helpers::CreateState(settings)) {}

	NavMeshBaker::~NavMeshBaker() {}

	NavMeshBaker::State NavMeshBaker::Progress(float maxTime) {
		Helpers::Process* const proc = Helpers::GetState(this);
		if (proc == nullptr)
			return State::UNINITIALIZED;
		const Stopwatch timer;
		while (
			proc->state != State::UNINITIALIZED && 
			proc->state != State::INVALIDATED &&
			proc->state != State::DONE) {
			Helpers::PerformStep(proc);
			if (timer.Elapsed() >= maxTime)
				break;
		}
		return proc->state;
	}

	NavMeshBaker::State NavMeshBaker::BakeState()const {
		const Helpers::Process* const proc = Helpers::GetState(this);
		return proc == nullptr ? State::UNINITIALIZED : proc->state;
	}

	Reference<TriMesh> NavMeshBaker::Result() {
		Helpers::Process* const proc = Helpers::GetState(this);
		if (proc == nullptr)
			return nullptr;
		while (
			proc->state != State::UNINITIALIZED &&
			proc->state != State::INVALIDATED &&
			proc->state != State::DONE)
			Progress(std::numeric_limits<float>::infinity());
		return proc->meshCleanupState.mesh;
	}

	void NavMeshBaker::Settings::Serializer::GetFields(const Callback<Serialization::SerializedObject>& recordElement, Settings* target)const {
		JIMARA_SERIALIZE_FIELDS(target, recordElement) {
			JIMARA_SERIALIZE_FIELD(target->environmentRoot, "Collider Root", "If provided, any collider that is not within this subtree will be discarded");
			JIMARA_SERIALIZE_FIELD(target->verticalSampleInterval, "Sample Size", "Interval between raycast samples");
			JIMARA_SERIALIZE_FIELD(target->maxStepDistance, "Step Distance",
				"If vertical distance between neighboring samples is less than this value, the faces will be connected");
			JIMARA_SERIALIZE_FIELD(target->maxMergeDistance, "Merge Distance",
				"Samples in the came column will be merged, if the distance between them is less than this value");
			JIMARA_SERIALIZE_FIELD(target->minAgentHeight, "Agent Height",
				"Minimal height of the agent; used for roof-checking");
			JIMARA_SERIALIZE_FIELD(target->maxSlopeAngle, "Max Slope", "Maximal slope angle the agents can walk on", 
				Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 90.0f));
			JIMARA_SERIALIZE_FIELD(target->surfaceLayers, "Surface Layers", "Surface layer mask", Layers::LayerMaskAttribute::Instance());
			JIMARA_SERIALIZE_FIELD(target->roofLayers, "Roof Layers", "Roof layer mask", Layers::LayerMaskAttribute::Instance());
			JIMARA_SERIALIZE_FIELD(target->simplificationSubsteps, "Simplification substeps",
				"Initial mesh will look like a grid of some sorts; it will be simplified based on the angle threshold; "
				"for better stability, the angle threshold will grow in several steps, defined by this number");
			JIMARA_SERIALIZE_FIELD(target->simplificationAngleThreshold, "Simplification Angle", "Simplification angle threshold",
				Object::Instantiate<Serialization::SliderAttribute<float>>(0.0f, 90.0f));
		};
	}
}
