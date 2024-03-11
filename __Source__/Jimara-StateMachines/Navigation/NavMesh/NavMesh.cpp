#include "NavMesh.h"
#include <Jimara/Data/Geometry/MeshAnalysis.h>
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
				bakedData->geometry = Object::Instantiate<TriMesh>(*self->settings.mesh);
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

		static std::vector<SurfaceEdgeNode> CalculateEdgeSequence(const NavMeshData* data, Vector3 start, Vector3 end, Vector3 agentUp, const AgentOptions& agentOptions);
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
				const Triangle3 tri0 = instance.octree[triId];
				for (size_t nId = 0u; nId < neighbors.Size(); nId++) {
					const size_t neighborId = neighbors[nId];
					if (neighborId == node.triangleId || neighborId == node.otherTriangleId)
						continue;
					const Triangle3 tri1 = instance.octree[neighborId];
					auto reportIfNodesMatch = [&](uint32_t eI0, uint32_t eI1) {
						const Vector3& a0 = tri0[eI0];
						const Vector3& b0 = tri0[(eI0 + 1u) % 3u];
						const Vector3& a1 = tri1[eI1];
						const Vector3& b1 = tri1[(eI1 + 1u) % 3u];
						const float distanceThresh = 0.01f * Math::Magnitude(a0 - b0);
						auto areCloseEnough = [&](const Vector3& a, const Vector3& b) {
							return Math::Magnitude(a - b) <= distanceThresh;
						};
						if (areCloseEnough(a0, b1) && areCloseEnough(b0, a1)) {
							report(SurfaceEdgeNode(
								instance.pose * Vector4((a0 + b0) * 0.5f, 1.0f),
								node.instanceId, triId, neighborId, Size2(eI0, eI1)));
							return true;
						}
						else return false;
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

	std::vector<NavMesh::PathNode> NavMesh::CalculatePath(Vector3 start, Vector3 end, Vector3 agentUp, const AgentOptions& agentOptions)const {
		const Helpers::NavMeshData* data = Helpers::GetData(this);
		assert(data != nullptr);
		std::shared_lock<std::shared_mutex> stateLock(Helpers::StateLock(data));

		auto normal = [&](const Triangle3& tri) {
			return Math::Normalize(Math::Cross(tri[2u] - tri[0u], tri[1u] - tri[0u]));
		};

		auto createPathNode = [&](const Helpers::SurfaceEdgeNode& edgeNode) -> PathNode {
			PathNode node = {};
			node.position = edgeNode.worldPosition;
			const PosedOctree<Triangle3>& instance = data->surfaceGeometry[edgeNode.instanceId];
			node.normal = normal(instance.octree[edgeNode.triangleId]);
			if (edgeNode.otherTriangleId < instance.octree.Size())
				node.normal += normal(instance.octree[edgeNode.otherTriangleId]);
			node.normal = Math::Normalize(Vector3(instance.pose * Vector4(node.normal, 0.0f)));
			return node;
		};

		const std::vector<Helpers::SurfaceEdgeNode> edgeNodes = Helpers::CalculateEdgeSequence(data, start, end, agentUp, agentOptions);

		// __TODO__: Run a funnel to straighten whatever can be straightened...

		std::vector<PathNode> pathNodes;
		for (size_t i = 0u; i < edgeNodes.size(); i++)
			pathNodes.push_back(createPathNode(edgeNodes[i]));
		return pathNodes;
	}



	template<> void TypeIdDetails::GetTypeAttributesOf<NavMesh::Surface>(const Callback<const Object*>& report) {
		static const Reference<const ConfigurableResource::ResourceFactory> factory = ConfigurableResource::ResourceFactory::Create<NavMesh::Surface>(
			"Nav-Mesh Surface", "Jimara/Navigation/Nav-Mesh Surface", "Navigation mesh surface");
		report(factory);
	}
}
