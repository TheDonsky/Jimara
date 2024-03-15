#include "NavMesh.h"
#include <Jimara/Data/Geometry/MeshAnalysis.h>
#include <Jimara/Data/Geometry/MeshModifiers.h>
#include <Jimara/Data/Serialization/Attributes/EnumAttribute.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>
#include <Jimara/Math/Algorithms/Pathfinding.h>


namespace Jimara {
	struct NavMesh::Surface::SurfaceHelpers {
		struct SurfaceData : public virtual Object {
			std::recursive_mutex stateLock;
			mutable SpinLock fieldLock;

			SurfaceSettings settings;

			Reference<const BakedSurfaceData> bakedData;
			std::atomic_bool dataDirty = true;
			mutable EventInstance<> onDirty;

			inline virtual ~SurfaceData() {}
		};

		inline static SurfaceData* GetData(Surface* surface) {
			return dynamic_cast<SurfaceData*>(surface->m_data.operator Jimara::Object * ());
		}

		inline static const SurfaceData* GetData(const Surface* surface) {
			return dynamic_cast<const SurfaceData*>(surface->m_data.operator Jimara::Object * ());
		}

		inline static void RebuildIfDirty(SurfaceData* self) {
			std::unique_lock<std::recursive_mutex> lock(self->stateLock);
			if (!self->dataDirty.load())
				return;
			self->dataDirty = false;

			// Early exit, if there's no mesh:
			if (self->settings.mesh == nullptr) {
				std::unique_lock<SpinLock> fieldLock(self->fieldLock);
				self->bakedData = nullptr;
				return;
			}

			// Create 'reduced/optimized' mesh for navigation:
			Reference<BakedSurfaceData> bakedData = Object::Instantiate<BakedSurfaceData>();
			{
				bakedData->geometry = ModifyMesh::ShadeSmooth(self->settings.mesh, true);
				// __TODO__: Reduce mesh complexity, based on simplificationThreshold...
			}
			const TriMesh::Reader mesh(bakedData->geometry);

			// Create Octree:
			{
				std::vector<Triangle3> faces;
				for (uint32_t i = 0u; i < mesh.FaceCount(); i++) {
					const TriangleFace& face = mesh.Face(i);
					faces.push_back(Triangle3(
						mesh.Vert(face.a).position,
						mesh.Vert(face.b).position,
						mesh.Vert(face.c).position));
				}
				bakedData->octree = Octree<Triangle3>::Build(faces.begin(), faces.end());
			};

			// Establish neighboring-face information:
			bakedData->triNeighbors = GetMeshFaceNeighborIndices(mesh, false);

			// Update mesh data:
			{
				std::unique_lock<SpinLock> fieldLock(self->fieldLock);
				self->bakedData = bakedData;
			}
		}

		static void OnMeshDirty(Surface* self, const TriMesh* mesh) {
			SurfaceHelpers::SurfaceData* data = SurfaceHelpers::GetData(self);
			{
				std::unique_lock<std::recursive_mutex> lock(data->stateLock);
				assert(data->settings.mesh == mesh);
				data->dataDirty = true;
				// __TODO__: Implement this crap!
				if ((data->settings.flags & SurfaceFlags::UPDATE_ASYNCHRONOUSLY) != SurfaceFlags::NONE) {
					RebuildIfDirty(data); // Schedule asynchronous rebuild instead...
				}
				else RebuildIfDirty(data);
			}
			data->onDirty();
		}
	};

	NavMesh::Surface::Surface(const ConfigurableResource::CreateArgs& createArgs) 
		: m_data(Object::Instantiate<SurfaceHelpers::SurfaceData>()) {
		Unused(createArgs);
	}

	NavMesh::Surface::~Surface() {
		Settings() = SurfaceSettings{};
	}

	NavMesh::SurfaceSettings NavMesh::Surface::Settings()const {
		const SurfaceHelpers::SurfaceData* data = SurfaceHelpers::GetData(this);
		std::unique_lock<SpinLock> lock(data->fieldLock);
		SurfaceSettings rv = data->settings;
		return rv;
	}

	Property<NavMesh::SurfaceSettings> NavMesh::Surface::Settings() {
		typedef SurfaceSettings(*GetFn)(Surface*);
		typedef void (*SetFn)(Surface*, const SurfaceSettings&);
		return Property<NavMesh::SurfaceSettings>(
			(GetFn)[](Surface* self) -> SurfaceSettings {
				const SurfaceHelpers::SurfaceData* data = SurfaceHelpers::GetData(self);
				std::unique_lock<SpinLock> lock(data->fieldLock);
				SurfaceSettings rv = data->settings;
				return rv;
			},
			(SetFn)[](Surface* self, const SurfaceSettings& value) {
				SurfaceHelpers::SurfaceData* data = SurfaceHelpers::GetData(self);
				{
					std::unique_lock<std::recursive_mutex> stateLock(data->stateLock);
					if (data->settings.mesh == value.mesh &&
						data->settings.flags == value.flags)
						return;
					const Callback<const TriMesh*> onMeshDirty = Callback<const TriMesh*>(SurfaceHelpers::OnMeshDirty, self);
					if (data->settings.mesh != nullptr)
						data->settings.mesh->OnDirty() -= onMeshDirty;
					{
						std::unique_lock<SpinLock> fieldLock(data->fieldLock);
						data->settings = value;
						data->dataDirty = true;
					}
					if (data->settings.mesh != nullptr)
						data->settings.mesh->OnDirty() += onMeshDirty;
					Surface::SurfaceHelpers::RebuildIfDirty(data);
				}
				data->onDirty();
			},
			this);
	}

	Reference<const NavMesh::BakedSurfaceData> NavMesh::Surface::Data()const {
		const SurfaceHelpers::SurfaceData* data = SurfaceHelpers::GetData(this);
		std::unique_lock<SpinLock> lock(data->fieldLock);
		Reference<const NavMesh::BakedSurfaceData> bakedData = data->bakedData;
		return bakedData;
	}

	Event<>& NavMesh::Surface::OnDirty()const {
		const SurfaceHelpers::SurfaceData* data = SurfaceHelpers::GetData(this);
		return data->onDirty;
	}

	void NavMesh::Surface::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			SurfaceSettings settings = Settings();
			JIMARA_SERIALIZE_FIELD(settings.mesh, "Mesh", "Surface Geometry");
			JIMARA_SERIALIZE_FIELD(settings.flags, "Flags", "Configuration Flags",
				Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<SurfaceFlags>>>(true,
					"UPDATE_ASYNCHRONOUSLY", SurfaceFlags::UPDATE_ASYNCHRONOUSLY));
			Settings() = settings;
		};
	}
	




	struct NavMesh::Helpers {
		struct SurfaceInstanceInfo {
			SurfaceInstance* instance = nullptr;
			Reference<const BakedSurfaceData> bakedData;
		};

		struct NavMeshData : public virtual Object {
			const Reference<SceneContext> context;
			mutable std::shared_mutex stateLock;
			VoxelGrid<PosedOctree<Triangle3>> surfaceGeometry;
			std::vector<SurfaceInstanceInfo> surfaces;

			inline NavMeshData(SceneContext* ctx) : context(ctx) {
				surfaceGeometry.BoundingBox() = AABB(Vector3(0.0f), Vector3(0.0f));
				surfaceGeometry.GridSize() = Size3(1u);
			}
			inline virtual ~NavMeshData() {}
		};


		static NavMeshData* GetData(const NavMesh* navMesh) {
			if (navMesh == nullptr)
				return nullptr;
			else return dynamic_cast<NavMeshData*>(navMesh->m_data.operator Jimara::Object * ());
		}

		static std::shared_mutex& StateLock(const NavMeshData* data) {
			if (data == nullptr) {
				static thread_local std::shared_mutex fallback;
				return fallback;
			}
			else return data->stateLock;
		}

		static void RebuildNavMeshGeometry(Helpers::NavMeshData* navMeshData, const std::unique_lock<std::shared_mutex>& lock) {
			if (navMeshData->surfaceGeometry.Size() <= 0u)
				return;
			AABB bounds = AABB(Vector3(0.0f), Vector3(0.0f));
			Vector3 averageSize = Vector3(1.0f);
			size_t numValidEntries = 0u;
			for (size_t i = 0u; i < navMeshData->surfaceGeometry.Size(); i++) {
				const Jimara::PosedOctree<Jimara::Triangle3>& octree = navMeshData->surfaceGeometry[i];
				if (octree.octree.Size() <= 0u)
					continue;
				const AABB bnd = octree.BoundingBox();
				if (numValidEntries > 0u) {
					bounds = AABB(
						Vector3(Math::Min(bounds.start.x, bnd.start.x), Math::Min(bounds.start.y, bnd.start.y), Math::Min(bounds.start.z, bnd.start.z)),
						Vector3(Math::Max(bounds.end.x, bnd.end.x), Math::Max(bounds.end.y, bnd.end.y), Math::Max(bounds.end.z, bnd.end.z)));
				}
				else bounds = bnd;
				numValidEntries++;
				averageSize = Math::Lerp(averageSize, bnd.end - bnd.start, 1.0f / static_cast<float>(numValidEntries));
			}
			const Vector3 totalSize = (bounds.end - bounds.start);
			navMeshData->surfaceGeometry.BoundingBox() = AABB(bounds.start - totalSize, bounds.start - totalSize);
			navMeshData->surfaceGeometry.GridSize() = static_cast<Size3>(Vector3(
				Math::Max(4.0f, Math::Min(totalSize.x * 4.0f / Math::Max(averageSize.x, std::numeric_limits<float>::epsilon()), 100.0f)),
				Math::Max(4.0f, Math::Min(totalSize.y * 4.0f / Math::Max(averageSize.y, std::numeric_limits<float>::epsilon()), 100.0f)),
				Math::Max(4.0f, Math::Min(totalSize.z * 4.0f / Math::Max(averageSize.z, std::numeric_limits<float>::epsilon()), 100.0f))));
			navMeshData->surfaceGeometry.BoundingBox() = bounds;
		}

		static void SurfaceInstanceDirty(const SurfaceInstance* instance, Helpers::NavMeshData* navMeshData, const std::unique_lock<std::shared_mutex>& lock) {
			if (!instance->m_activeIndex.has_value())
				return;

			const size_t index = instance->m_activeIndex.value();
			const Surface* const surface = instance->m_shape;
			assert(index < navMeshData->surfaces.size());
			assert(navMeshData->surfaces[index].instance == instance);
			assert(navMeshData->surfaces.size() == navMeshData->surfaceGeometry.Size());

			Reference<const BakedSurfaceData> bakedData;
			if (surface != nullptr) 
				bakedData = surface->Data();
			navMeshData->surfaces[index].bakedData = bakedData;
			PosedOctree<Triangle3> instanceShape;
			bool geometryNeedsRebuild = false;
			if (bakedData != nullptr) {
				instanceShape.octree = bakedData->octree;
				instanceShape.pose = instance->m_transform;

				const AABB shapeBBox = instanceShape.BoundingBox();
				const AABB worldBBox = navMeshData->surfaceGeometry.BoundingBox();
				geometryNeedsRebuild = (
					shapeBBox.start.x < worldBBox.start.x || shapeBBox.end.x > worldBBox.end.x ||
					shapeBBox.start.y < worldBBox.start.y || shapeBBox.end.y > worldBBox.end.y ||
					shapeBBox.start.z < worldBBox.start.z || shapeBBox.end.z > worldBBox.end.z);
			}
			navMeshData->surfaceGeometry[index] = instanceShape;
			
			if (geometryNeedsRebuild)
				RebuildNavMeshGeometry(navMeshData, lock);
		}

		static void OnSurfaceInstanceDirty(const SurfaceInstance* instance) {
			Helpers::NavMeshData* navMeshData = Helpers::GetData(instance->m_navMesh);
			if (navMeshData == nullptr)
				return;
			std::unique_lock<std::shared_mutex> lock(Helpers::StateLock(navMeshData));
			SurfaceInstanceDirty(instance, navMeshData, lock);
		}

		struct SurfaceEdgeNode;

		static std::vector<SurfaceEdgeNode> CalculateEdgeSequence(
			const NavMeshData* data, 
			Vector3 start, Vector3 end, 
			Vector3 agentUp, const AgentOptions& agentOptions);
	
		static void FunnelPath(
			const NavMeshData* data, std::vector<PathNode>& result,
			const SurfaceEdgeNode* path, size_t pathSize, 
			const AgentOptions& agentOptions);

		static void ReprojectPath(const NavMeshData* data, PathNode* path, size_t pathSize, const AgentOptions& agentOptions);
	};

	NavMesh::SurfaceInstance::SurfaceInstance(NavMesh* navMesh) : m_navMesh(navMesh) {}

	NavMesh::SurfaceInstance::~SurfaceInstance() {
		Shape() = nullptr;
		assert(m_shape == nullptr);
		Enabled() = false;
		assert(!m_activeIndex.has_value());
	}

	NavMesh::Surface* NavMesh::SurfaceInstance::Shape()const { return m_shape; }

	Property<NavMesh::Surface*> NavMesh::SurfaceInstance::Shape() {
		using SurfaceRef = Surface*;
		typedef SurfaceRef(*GetFn)(SurfaceInstance*);
		typedef void(*SetFn)(SurfaceInstance*, const SurfaceRef&);
		typedef void(*OnSurfaceDirtyFn)(SurfaceInstance*);
		return Property<NavMesh::Surface*>(
			(GetFn)[](SurfaceInstance* self) -> SurfaceRef { return self->m_shape; },
			(SetFn)[](SurfaceInstance* self, const SurfaceRef& value) {
				const Callback<> onDirty(Helpers::OnSurfaceInstanceDirty, self);
				{
					Helpers::NavMeshData* navMeshData = Helpers::GetData(self->m_navMesh);
					std::unique_lock<std::shared_mutex> lock(Helpers::StateLock(navMeshData));
					if (value == self->m_shape)
						return;
					if (self->m_shape != nullptr)
						self->m_shape->OnDirty() -= onDirty;
					self->m_shape = value;
					if (self->m_shape != nullptr)
						self->m_shape->OnDirty() += onDirty;
				}
				onDirty();
			}, this);
	}

	Matrix4 NavMesh::SurfaceInstance::Transform()const {
		return m_transform;
	}

	Property<Matrix4> NavMesh::SurfaceInstance::Transform() {
		typedef Matrix4(*GetFn)(SurfaceInstance*);
		typedef void (*SetFn)(SurfaceInstance*, const Matrix4&);
		return Property<Matrix4>(
			(GetFn)[](SurfaceInstance* self) -> Matrix4 { return self->m_transform; },
			(SetFn)[](SurfaceInstance* self, const Matrix4& value) {
				if (self->m_transform == value)
					return;
				self->m_transform = value;
				Helpers::OnSurfaceInstanceDirty(self);
			}, this);
	}

	bool NavMesh::SurfaceInstance::Enabled()const {
		return m_enabled.load();
	}

	Property<bool> NavMesh::SurfaceInstance::Enabled() {
		typedef bool(*GetFn)(SurfaceInstance*);
		typedef void (*SetFn)(SurfaceInstance*, const bool&);
		return Property<bool>(
			(GetFn)[](SurfaceInstance* self) -> bool { return self->m_enabled.load(); },
			(SetFn)[](SurfaceInstance* self, const bool& value) {
				Helpers::NavMeshData* navMeshData = Helpers::GetData(self->m_navMesh);
				if (navMeshData == nullptr)
					return;
				std::unique_lock<std::shared_mutex> lock(Helpers::StateLock(navMeshData));
				self->m_enabled = value;
				if (value == self->m_activeIndex.has_value())
					return;
				else if (value) {
					self->m_activeIndex = navMeshData->surfaces.size();
					navMeshData->surfaces.push_back({ self, nullptr });
					navMeshData->surfaceGeometry.Push(PosedOctree<Triangle3>());
					assert(navMeshData->surfaces.size() == navMeshData->surfaceGeometry.Size());
					Helpers::SurfaceInstanceDirty(self, navMeshData, lock);
				}
				else {
					const size_t index = self->m_activeIndex.value();
					{
						std::swap(navMeshData->surfaces.back(), navMeshData->surfaces[index]);
						navMeshData->surfaces[index].instance->m_activeIndex = index;
						navMeshData->surfaces.pop_back();
					}
					{
						const PosedOctree<Triangle3> last = navMeshData->surfaceGeometry[navMeshData->surfaceGeometry.Size() - 1u];
						navMeshData->surfaceGeometry[index] = last;
						navMeshData->surfaceGeometry.Pop();
					}
					self->m_activeIndex = std::optional<size_t>();
					assert(navMeshData->surfaces.size() == navMeshData->surfaceGeometry.Size());
					assert(!self->m_activeIndex.has_value());
				}
			}, this);
	}





	Reference<NavMesh> NavMesh::Instance(SceneContext* context) {
#pragma warning(disable: 4250)
		struct CachedInstance : public virtual NavMesh, public virtual ObjectCache<Reference<const Object>>::StoredObject {
			inline CachedInstance(Helpers::NavMeshData* data) : NavMesh(data) {}
			inline virtual ~CachedInstance() {}
		};
#pragma warning(default: 4250)
		struct Cache : public virtual ObjectCache<Reference<const Object>> {
			inline static Reference<CachedInstance> Get(SceneContext* context) {
				static Cache cache;
				return cache.GetCachedOrCreate(context, [&]() -> Reference<CachedInstance> {
					const Reference<Helpers::NavMeshData> data = Object::Instantiate<Helpers::NavMeshData>(context);
					return Object::Instantiate<CachedInstance>(data);
					});
			}
		};
		return Cache::Get(context);
	}

	Reference<NavMesh> NavMesh::Create(SceneContext* context) {
		const Reference<Object> data = Object::Instantiate<Helpers::NavMeshData>(context);
		const Reference<NavMesh> instance = new NavMesh(data);
		instance->ReleaseRef();
		return instance;
	}

	NavMesh::~NavMesh() {}

	struct NavMesh::Helpers::SurfaceEdgeNode {
		Vector3 worldPosition = {};
		size_t instanceId = ~size_t(0u);
		size_t triangleId = ~size_t(0u);
		size_t otherTriangleId = ~size_t(0u);
		Size2 edgeId = Size2(~uint32_t(0u)); // Values equal to or larger than 3 mean start or end hit points...

		inline SurfaceEdgeNode() {}
		inline SurfaceEdgeNode(const Vector3& pos, size_t instance, size_t tri0, size_t tri1, Size2 edge)
			: worldPosition(pos), instanceId(instance)
			, triangleId(Math::Min(tri0, tri1)), otherTriangleId(Math::Max(tri0, tri1)), edgeId(edge) {
			if (triangleId != tri0)
				std::swap(edgeId.x, edgeId.y);
		}
		inline bool operator<(const SurfaceEdgeNode& other)const {
			return
				(instanceId < other.instanceId) ? true : (instanceId > other.instanceId) ? false :
				(triangleId < other.triangleId) ? true : (triangleId > other.triangleId) ? false :
				(edgeId.x < other.edgeId.x);
		}
		inline bool operator==(const SurfaceEdgeNode& other)const {
			return
				(instanceId == other.instanceId) &&
				(triangleId == other.triangleId) &&
				(edgeId.x == other.edgeId.x);
		}
	};

	std::vector<NavMesh::Helpers::SurfaceEdgeNode> NavMesh::Helpers::CalculateEdgeSequence(
		const NavMeshData* data, Vector3 start, Vector3 end, Vector3 agentUp, const AgentOptions& agentOptions) {
		const auto startHit = data->surfaceGeometry.Raycast(start + agentUp * agentOptions.radius, -agentUp);
		const auto endHit = data->surfaceGeometry.Raycast(end + agentUp * agentOptions.radius, -agentUp);
		if ((!startHit) || (!endHit))
			return {};

		const size_t startInstanceId = data->surfaceGeometry.IndexOf(startHit.target);
		const size_t endInstanceId = data->surfaceGeometry.IndexOf(endHit.target);
		if (startInstanceId != endInstanceId)
			return {}; // For now, we do not [yet] support jumps...

		const size_t startTriangleId = data->surfaceGeometry[startInstanceId].octree.IndexOf(startHit.hit.target);
		const size_t endTriangleId = data->surfaceGeometry[endInstanceId].octree.IndexOf(endHit.hit.target);

		const SurfaceEdgeNode startEdge(static_cast<Math::SweepHitPoint>(startHit), startInstanceId, startTriangleId, startTriangleId, Size2(4u));
		const SurfaceEdgeNode endEdge(static_cast<Math::SweepHitPoint>(endHit), endInstanceId, endTriangleId, endTriangleId, Size2(5u));

		const float normalThreshold = std::cos(Math::Radians(agentOptions.maxTiltAngle));

		if (startEdge.triangleId == endEdge.triangleId)
			return { startEdge, endEdge };

		auto heuristic = [&](const SurfaceEdgeNode& node) {
			return Math::Magnitude(endEdge.worldPosition - node.worldPosition);
		};

		auto getNeighbors = [&](const SurfaceEdgeNode& node, auto reportNeighbor) {
			const PosedOctree<Triangle3>& instance = data->surfaceGeometry[node.instanceId];
			const Helpers::SurfaceInstanceInfo& instanceInfo = data->surfaces[node.instanceId];
			const std::vector<Stacktor<uint32_t, 3u>>& triNeighbors = instanceInfo.bakedData->triNeighbors;

			auto report = [&](const SurfaceEdgeNode& neighbor) {
				const float distance = Math::Magnitude(neighbor.worldPosition - node.worldPosition);
				reportNeighbor(neighbor, distance);
			};

			auto reportTriangleEdges = [&](size_t triId, uint32_t edgeId) {
				if (triId >= triNeighbors.size())
					return;

				if (node.instanceId == endEdge.instanceId &&
					triId == endEdge.triangleId &&
					node.edgeId.x != endEdge.edgeId.x) {
					report(endEdge);
				}

				const Stacktor<uint32_t, 3u>& neighbors = triNeighbors[triId];
				auto calculateNormal = [&](const Triangle3& tri) {
					return Math::Normalize(Vector3(instance.pose * Vector4(
						Math::Cross(tri[2u] - tri[0u], tri[1u] - tri[0u]), 0.0f)));
				};
				const Triangle3 tri0 = instance.octree[triId];
				const Vector3 normal0 =
					((agentOptions.flags & AgentFlags::FIXED_UP_DIRECTION) != AgentFlags::NONE)
					? agentUp : calculateNormal(tri0);
				for (size_t nId = 0u; nId < neighbors.Size(); nId++) {
					const size_t neighborId = neighbors[nId];
					if (neighborId == node.triangleId || neighborId == node.otherTriangleId)
						continue;
					const Triangle3 tri1 = instance.octree[neighborId];
					const Vector3 normal1 = calculateNormal(tri1);
					if (Math::Dot(normal1, normal0) < normalThreshold)
						continue;
					auto reportIfNodesMatch = [&](uint32_t eI0, uint32_t eI1) {
						const Vector3& a0 = tri0[eI0];
						const Vector3& b0 = tri0[(eI0 + 1u) % 3u];
						const Vector3& a1 = tri1[eI1];
						const Vector3& b1 = tri1[(eI1 + 1u) % 3u];
						const float distanceThresh = 0.01f * Math::Magnitude(a0 - b0);
						auto areCloseEnough = [&](const Vector3& a, const Vector3& b) {
							return Math::Magnitude(a - b) <= distanceThresh;
						};
						if ((!areCloseEnough(a0, b1)) || (!areCloseEnough(b0, a1)))
						//if (a0 != b1 || b0 != a1)
							return false;
						const Vector3 localMidpoint = (a0 + b0) * 0.5f;
						const Vector3 worldMidpoint = instance.pose * Vector4(localMidpoint, 1.0f);
						//*
						const Vector3 worldOffset = instance.pose * Vector4(b0 - a0, 0.0f);
						if (edgeId >= 3u) {
							if ((Math::Magnitude(worldOffset) * 0.5f) < agentOptions.radius)
								return false;
						}
						else {
							const Vector3 dir = Math::Normalize(worldMidpoint - node.worldPosition);
							const Vector3 axisOffset = worldOffset - dir * Math::Dot(worldOffset, dir);
							if ((Math::Magnitude(axisOffset) * 0.5f) < agentOptions.radius)
								return false;
						}
						//*/
						report(SurfaceEdgeNode(worldMidpoint, node.instanceId, triId, neighborId, Size2(eI0, eI1)));
						return true;
					};
					for (uint32_t eI1 = 0u; eI1 < 3u; eI1++)
						for (uint32_t eI0 = 0u; eI0 < 3u; eI0++)
							if (eI0 != edgeId)
								if (reportIfNodesMatch(eI0, eI1)) {
									eI1 = 8u;
									break;
								}
				}
			};

			reportTriangleEdges(node.triangleId, node.edgeId.x);
			if (node.otherTriangleId != node.triangleId)
				reportTriangleEdges(node.otherTriangleId, node.edgeId.y);
		};

		const std::vector<SurfaceEdgeNode> nodes = Algorithms::AStar(startEdge, endEdge, heuristic, getNeighbors);
		return nodes;
	}

	void NavMesh::Helpers::FunnelPath(
		const NavMeshData* data, std::vector<PathNode>& result,
		const SurfaceEdgeNode* path, size_t pathSize,
		const AgentOptions& agentOptions) {
		if (pathSize < 2u)
			return;

		static const auto safeNormalize = [](const Vector3& v) {
			const float magn = Math::Magnitude(v);
			if (magn > std::numeric_limits<float>::epsilon())
				return v / magn;
			else return Vector3(0.0f);
		};

		auto normal = [](const Triangle3& tri) {
			return safeNormalize(Math::Cross(tri[2u] - tri[0u], tri[1u] - tri[0u]));
		};

		auto edgeNormal = [&](const SurfaceEdgeNode& node) {
			const PosedOctree<Triangle3>& instance = data->surfaceGeometry[node.instanceId];
			Vector3 result = normal(instance.octree[node.triangleId]);
			if (node.otherTriangleId < instance.octree.Size())
				result += normal(instance.octree[node.otherTriangleId]);
			result = safeNormalize(Vector3(instance.pose * Vector4(result, 0.0f)));
			return result;
		};

		struct Portal {
			Vector3 a = {};
			Vector3 b = {};
		};

		auto getPortal = [&](size_t pathNodeId) -> Portal {
			const SurfaceEdgeNode& node = path[pathNodeId];
			if (pathNodeId <= 0u || pathNodeId >= (pathSize - 1u))
				return { node.worldPosition, node.worldPosition };
			const SurfaceEdgeNode& prev = path[pathNodeId - 1u];
			const PosedOctree<Triangle3>& geometry = data->surfaceGeometry[node.instanceId];
			const Triangle3& face = geometry.octree[node.triangleId];
			const Vector3 scaledFaceNormal = geometry.pose * Vector4(normal(face), 0.0f);
			Portal portal = {
				Vector3(geometry.pose * Vector4(face[node.edgeId.x], 1.0f)),
				Vector3(geometry.pose * Vector4(face[(node.edgeId.x + 1u) % 3u], 1.0f))
			};
			if (Math::Dot(scaledFaceNormal, Math::Cross(portal.a - prev.worldPosition, portal.b - prev.worldPosition)) < 0.0f)
				std::swap(portal.a, portal.b);
			return portal;
		};

		std::vector<Portal> portals;
		for (size_t i = 0u; i < pathSize; i++)
			portals.push_back(getPortal(i));
		{
			auto addPortalInsets = [&](const auto& getPos, float dirMultiplier) {
				for (size_t s = 1u; s < (pathSize - 1u);) {
					size_t e = s + 1u;
					const Vector3 cur = getPos(portals[s]);
					while (e < (pathSize - 1u) &&
						Math::Magnitude(getPos(portals[e]) - cur) <= std::numeric_limits<float>::epsilon())
						e++;

					Vector3 avgDir = Vector3(0.0f);
					float dirScale = 1.0f;
					auto calculateAvgDirAndScale = [&](const Vector3& prevDir, const Vector3& nextDir) -> bool {
						avgDir = (prevDir + nextDir) * 0.5f;
						const float cosSqr = Math::SqrMagnitude(avgDir);
						if (cosSqr <= std::numeric_limits<float>::epsilon())
							return false;
						const float cosine = std::sqrt(cosSqr);
						dirScale = (cosine + (1.0f - cosSqr) / cosine);
						avgDir /= cosine;
						return true;
					};
					{
						Portal prevPortal = getPortal(s - 1u);
						Portal nextPortal = getPortal(e);
						if (!calculateAvgDirAndScale(safeNormalize(getPos(prevPortal) - cur), safeNormalize(getPos(nextPortal) - cur))) {
							const Vector3 prevDir = safeNormalize(portals[s].b - portals[s].a) * dirMultiplier;
							const Vector3 nextDir = safeNormalize(portals[e - 1u].b - portals[e - 1u].a) * dirMultiplier;
							if (!calculateAvgDirAndScale(prevDir, nextDir))
								avgDir = Math::Cross(safeNormalize(edgeNormal(path[s]) + edgeNormal(path[e - 1u])), nextDir);
						}
					}
					float dirSide = 1.0f;

					float minPortalSize = std::numeric_limits<float>::infinity();
					for (size_t i = s; i < e; i++) {
						const Portal portal = getPortal(i);
						const Vector3 portalOffset = (portal.b - portal.a) * dirMultiplier;
						const float portalSize = Math::Magnitude(portalOffset);
						minPortalSize = Math::Min(minPortalSize, portalSize * 0.5f);
						if (Math::Dot(avgDir, portalOffset) < 0.0f)
							dirSide = -1.0f;
					}

					const Vector3 delta = avgDir * dirSide * Math::Min(agentOptions.radius * dirScale, minPortalSize * 0.5f);
					for (size_t i = s; i < e; i++)
						getPos(portals[i]) += delta;

					s = e;
				}
			};
			addPortalInsets([&](Portal& p) -> Vector3& { return p.a; }, 1.0f);
			addPortalInsets([&](Portal& p) -> Vector3& { return p.b; }, -1.0f);
			/*
			for (size_t i = 0u; i < pathSize; i++)
				result.push_back(PathNode{ portals[i].a,edgeNormal(path[i]) });
			for (size_t i = pathSize - 1u; i--> 0u;)
				result.push_back(PathNode{ portals[i].b,edgeNormal(path[i]) });
			return;
			//*/
		}

		Vector3 chainStart = path[0u].worldPosition;
		size_t chainStartId = 0u;
		size_t portalId = 1u;
		size_t cornerA = 1u;
		size_t cornerB = 1u;
		size_t initialNodeCount = result.size();
		result.push_back(PathNode{ chainStart, edgeNormal(path[chainStartId]) });

		auto appendNodes = [&](const Vector3& chainEnd, size_t chainEndId) {
			auto append = [&](const PathNode& node) {
				PathNode& last = result.back();
				if (Math::Magnitude(last.position - node.position) < agentOptions.radius) {
					last.position = node.position;
					last.normal = safeNormalize(last.normal + node.normal);
				}
				else result.push_back(node);
			};
			//*
			const Portal endPortal = getPortal(chainEndId);
			while (chainStartId < (chainEndId - 1u)) {
				chainStartId++;
				const Portal portal = getPortal(chainStartId);
				if (Math::Min(
					Math::SqrMagnitude(portal.a - endPortal.a),
					Math::SqrMagnitude(portal.b - endPortal.b)) < std::numeric_limits<float>::epsilon())
					break;

				const Vector3 lastPoint = result.back().position;
				const Vector3 rawDir = safeNormalize(chainEnd - lastPoint);
				const Vector3 startDelta = (portal.a - lastPoint);

				const Vector3 deltaR = (portal.b - portal.a);
				const float rangeR = Math::Max(Math::Magnitude(deltaR), std::numeric_limits<float>::epsilon());
				const Vector3 right = deltaR / rangeR;

				const Vector3 forward = safeNormalize(startDelta - right * Math::Dot(startDelta, right));
				const Vector3 up = safeNormalize(Math::Cross(forward, right));

				const Vector3 dir = safeNormalize(rawDir - up * Math::Dot(rawDir, up));
				const Vector3 delta = startDelta - up * Math::Dot(startDelta, up);
				const float distanceF = Math::Dot(delta, forward);
				const float speedF = Math::Dot(dir, forward);
				
				float time = 0.0f;
				if (std::abs(speedF * distanceF) < std::numeric_limits<float>::epsilon()) {
					time = Math::Max(0.0f, Math::Min(
						Math::Dot(startDelta, dir),
						Math::Dot(portal.b - lastPoint, dir)));
				}
				else time = std::abs(distanceF / speedF);

				const Vector3 pnt = lastPoint + dir * time;
				if (Math::Dot(pnt - portal.a, right) >= 0.0f &&
					Math::Dot(pnt - portal.b, right) <= 0.0f)
					append(PathNode{ pnt, edgeNormal(path[chainStartId]) });
			}
			//*/
			chainStartId = chainEndId;
			portalId = chainStartId + 1u;
			cornerA = portalId;
			cornerB = portalId;
			const Vector3 endEdgeNormal = edgeNormal(path[chainStartId]);
			const Vector3 endEdgeDir = safeNormalize(endPortal.b - endPortal.a);
			append(PathNode{ 
				//endPortal.a + endEdgeDir * Math::Dot(chainEnd - endPortal.a, endEdgeDir)
				chainEnd 
				//- endEdgeNormal * Math::Dot(chainEnd - (endPortal.a + endPortal.b) * 0.5f, endEdgeNormal)
				, endEdgeNormal});
			chainStart = result.back().position;
		};

		while (portalId < (pathSize - 1u)) {
			const Portal& portalA = portals[cornerA];
			const Portal& portalB = portals[cornerB];
			const Vector3 dirA = safeNormalize(portalA.a - chainStart);
			const Vector3 dirB = safeNormalize(portalB.b - chainStart);
			const Vector3 up = edgeNormal(path[chainStartId]);

			if (Math::Dot(edgeNormal(path[portalId]), up) < 0.25f) {
				appendNodes(path[portalId].worldPosition, portalId);
				continue;
			}
			portalId++;

			const Portal& portal = portals[portalId];
			const Vector3 newDirA = safeNormalize(portal.a - chainStart);
			const Vector3 newDirB = safeNormalize(portal.b - chainStart);

			if (Math::Dot(up, Math::Cross(dirA, newDirB)) < 0.0f && Math::SqrMagnitude(dirA) > 0.0f)
				appendNodes(portalA.a, cornerA);
			else if (Math::Dot(up, Math::Cross(newDirA, dirB)) < 0.0f && Math::SqrMagnitude(dirB) > 0.0f)
				appendNodes(portalB.b, cornerB);
			else {
				if (Math::Dot(up, Math::Cross(dirA, newDirA)) >= 0.0f)
					cornerA = portalId;
				if (Math::Dot(up, Math::Cross(newDirB, dirB)) >= 0.0f)
					cornerB = portalId;
			}
		}
		assert(portalId == (pathSize - 1u));
		appendNodes(path[portalId].worldPosition, portalId);
	}

	void NavMesh::Helpers::ReprojectPath(const NavMeshData* data, PathNode* path, size_t pathSize, const AgentOptions& agentOptions) {
		for (size_t i = 0u; i < pathSize; i++) {
			PathNode& node = path[i];
			auto castResult = data->surfaceGeometry.Raycast(
				node.position + node.normal * agentOptions.radius, -node.normal, agentOptions.radius * 3.0f);
			if (!castResult) {
				castResult = data->surfaceGeometry.Raycast(node.position, node.normal, agentOptions.radius * 2.0f);
				if (!castResult)
					continue;
			}
			const Vector3 oldPos = node.position;
			node.position = Math::SweepHitPoint(castResult);
			if (Math::Magnitude(oldPos - node.position) > agentOptions.radius) {
				const Triangle3& tri = *castResult.hit.target;
				node.normal = Math::Normalize(Vector3(castResult.target->pose *
					Vector4(Math::Cross(tri[2u] - tri[0u], tri[1u] - tri[0u]), 0.0f)));
			}
		}
	}

	std::vector<NavMesh::PathNode> NavMesh::CalculatePath(Vector3 start, Vector3 end, Vector3 agentUp, const AgentOptions& agentOptions)const {
		const Helpers::NavMeshData* data = Helpers::GetData(this);
		assert(data != nullptr);
		std::shared_lock<std::shared_mutex> stateLock(Helpers::StateLock(data));
		const std::vector<Helpers::SurfaceEdgeNode> edgeNodes = Helpers::CalculateEdgeSequence(data, start, end, Math::Normalize(agentUp), agentOptions);
		std::vector<PathNode> result;
		Helpers::FunnelPath(data, result, edgeNodes.data(), edgeNodes.size(), agentOptions);
		Helpers::ReprojectPath(data, result.data(), result.size(), agentOptions);
		return result;
	}



	template<> void TypeIdDetails::GetTypeAttributesOf<NavMesh::Surface>(const Callback<const Object*>& report) {
		static const Reference<const ConfigurableResource::ResourceFactory> factory = ConfigurableResource::ResourceFactory::Create<NavMesh::Surface>(
			"Nav-Mesh Surface", "Jimara/Navigation/Nav-Mesh Surface", "Navigation mesh surface");
		report(factory);
	}
}
