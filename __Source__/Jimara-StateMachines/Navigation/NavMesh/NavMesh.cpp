#include "NavMesh.h"


namespace Jimara {
	struct NavMesh::Surface::SurfaceHelpers {
		struct MeshData {
			Octree<Triangle3> octree;
			std::vector<Stacktor<size_t, 3u>> triNeighbors;
		};

		struct SurfaceData : public virtual Object {
			std::recursive_mutex stateLock;
			mutable SpinLock fieldLock;

			Reference<TriMesh> mesh;
			std::atomic<float> simplificationThreshold = 0.0f;
			std::atomic<SurfaceFlags> flags = SurfaceFlags::NONE;

			MeshData meshData;
			std::atomic_bool dataDirty = true;
		};

		inline static SurfaceData* GetData(Surface* surface) {
			return dynamic_cast<SurfaceData*>(surface->m_data.operator Jimara::Object * ());
		}

		inline static const SurfaceData* GetData(const Surface* surface) {
			return dynamic_cast<const SurfaceData*>(surface->m_data.operator Jimara::Object * ());
		}

		inline static MeshData GetMeshData(const Surface* surface) {
			const SurfaceData* data = GetData(surface);
			std::unique_lock<SpinLock> lock(data->fieldLock);
			const MeshData rv = data->meshData;
			return rv;
		}

		inline static void RebuildIfDirty(SurfaceData* self) {
			std::unique_lock<std::recursive_mutex> lock(self->stateLock);
			if (!self->dataDirty.load())
				return;
			self->dataDirty = false;

			// Early exit, if there's no mesh:
			if (self->mesh == nullptr) {
				std::unique_lock<SpinLock> fieldLock(self->fieldLock);
				self->meshData = MeshData();
			}

			// Create 'reduced/optimized' mesh for navigation:
			TriMesh reducedMesh;
			{
				reducedMesh = *self->mesh;
				// __TODO__: Reduce mesh complexity, based on simplificationThreshold...
			}
			const TriMesh::Reader mesh(reducedMesh);

			// Create Octree:
			Octree<Triangle3> octree;
			{
				std::vector<Triangle3> faces;
				for (uint32_t i = 0u; i < mesh.FaceCount(); i++) {
					const TriangleFace& face = mesh.Face(i);
					faces.push_back(Triangle3(
						mesh.Vert(face.a).position,
						mesh.Vert(face.b).position,
						mesh.Vert(face.c).position));
				}
				octree = Octree<Triangle3>::Build(faces.begin(), faces.end());
			};

			
			// Establish neighboring-face information:
			std::vector<Stacktor<size_t, 3u>> neighbors;
			{
				neighbors.resize(mesh.FaceCount());
				// __TODO__: build triNeighbors information...
			}

			// Update mesh data:
			{
				std::unique_lock<SpinLock> fieldLock(self->fieldLock);
				self->meshData.octree = std::move(octree);
				self->meshData.triNeighbors = std::move(neighbors);
			}
		}

		static void OnMeshDirty(Surface* self, const TriMesh* mesh) {
			SurfaceHelpers::SurfaceData* data = SurfaceHelpers::GetData(self);
			std::unique_lock<std::recursive_mutex> lock(data->stateLock);
			assert(data->mesh == mesh);
			data->dataDirty = true;
			// __TODO__: Implement this crap!
			if ((data->flags.load() & SurfaceFlags::UPDATE_ASYNCHRONOUSLY) != SurfaceFlags::NONE) {
				RebuildIfDirty(data); // Schedule asynchronous rebuild instead...
			}
			else RebuildIfDirty(data);
		}
	};

	NavMesh::Surface::Surface(const ConfigurableResource::CreateArgs& createArgs) 
		: m_data(Object::Instantiate<SurfaceHelpers::SurfaceData>()){
		Unused(createArgs);
	}

	NavMesh::Surface::~Surface() {
		SetMesh(nullptr);
	}

	Reference<TriMesh> NavMesh::Surface::Mesh()const {
		const SurfaceHelpers::SurfaceData* data = SurfaceHelpers::GetData(this);
		std::unique_lock<SpinLock> lock(data->fieldLock);
		const Reference<TriMesh> mesh = data->mesh;
		return mesh;
	}

	void NavMesh::Surface::SetMesh(TriMesh* mesh) {
		SurfaceHelpers::SurfaceData* data = SurfaceHelpers::GetData(this);
		std::unique_lock<std::recursive_mutex> lock(data->stateLock);
		if (data->mesh == mesh)
			return;
		const Callback<const TriMesh*> onMeshDirty = Callback<const TriMesh*>(SurfaceHelpers::OnMeshDirty, this);
		if (data->mesh != nullptr)
			data->mesh->OnDirty() -= onMeshDirty;
		{
			std::unique_lock<SpinLock> lock(data->fieldLock);
			data->mesh = mesh;
		}
		if (data->mesh != nullptr)
			data->mesh->OnDirty() += onMeshDirty;
	}

	float NavMesh::Surface::SimplificationAngleThreshold()const { 
		return SurfaceHelpers::GetData(this)->simplificationThreshold.load(); 
	}

	void NavMesh::Surface::SetSimplificationAngleThreshold(float threshold) {
		SurfaceHelpers::SurfaceData* data = SurfaceHelpers::GetData(this);
		std::unique_lock<std::recursive_mutex> lock(data->stateLock);
		if (data->simplificationThreshold.load() == threshold)
			return;
		data->simplificationThreshold = threshold;
		data->dataDirty = true;
		Surface::SurfaceHelpers::RebuildIfDirty(data);
	}

	NavMesh::SurfaceFlags NavMesh::Surface::Flags()const { 
		return SurfaceHelpers::GetData(this)->flags.load(); 
	}

	void NavMesh::Surface::SetFlags(SurfaceFlags flags) {
		SurfaceHelpers::SurfaceData* data = SurfaceHelpers::GetData(this);
		std::unique_lock<std::recursive_mutex> lock(data->stateLock);
		if (data->flags.load() == flags)
			return;
		data->flags = flags;
		data->dataDirty = true;
		Surface::SurfaceHelpers::RebuildIfDirty(data);
	}

	void NavMesh::Surface::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		// __TODO__: Implement this crap!
	}
	



	NavMesh::SurfaceInstance::SurfaceInstance(NavMesh* navMesh) {
		// __TODO__: Implement this crap!
	}

	NavMesh::SurfaceInstance::~SurfaceInstance() {
		// __TODO__: Implement this crap!
	}

	NavMesh::Surface* NavMesh::SurfaceInstance::Shape()const {
		// __TODO__: Implement this crap!
	}

	void NavMesh::SurfaceInstance::SetShape(Surface* surface) {
		// __TODO__: Implement this crap!
	}

	Matrix4 NavMesh::SurfaceInstance::Transform()const {
		// __TODO__: Implement this crap!
		return {};
	}

	void NavMesh::SurfaceInstance::SetTransform(const Matrix4& transform) {
		// __TODO__: Implement this crap!
	}

	bool NavMesh::SurfaceInstance::Enabled()const {
		// __TODO__: Implement this crap!
		return false;
	}

	bool NavMesh::SurfaceInstance::Enable(bool enable) {
		// __TODO__: Implement this crap!
	}




	struct NavMesh::Helpers {

	};

	Reference<NavMesh> NavMesh::Instance(const SceneContext* context) {
		// __TODO__: Implement this crap!
		return {};
	}

	Reference<NavMesh> NavMesh::Create(const SceneContext* context) {
		// __TODO__: Implement this crap!
		return {};
	}

	NavMesh::~NavMesh() {
		// __TODO__: Implement this crap!
	}

	std::vector<NavMesh::PathNode> NavMesh::CalculatePath(Vector3 start, Vector3 end, Vector3 agentUp, const AgentOptions& agentOptions) {
		// __TODO__: Implement this crap!
	}
}
