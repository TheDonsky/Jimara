#include "NavMesh.h"
#include <Jimara/Data/Geometry/MeshAnalysis.h>
#include <Jimara/Data/Serialization/Attributes/EnumAttribute.h>
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


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

		static void OnSurfaceInstanceDirty(const SurfaceInstance* instance) {
			// __TODO__: Implement this crap!
		}
	};

	NavMesh::SurfaceInstance::SurfaceInstance(NavMesh* navMesh) : m_navMesh(navMesh) {}

	NavMesh::SurfaceInstance::~SurfaceInstance() {
		Shape() = nullptr;
		Enabled() = false;
	}

	const NavMesh::Surface* NavMesh::SurfaceInstance::Shape()const { return m_shape; }

	Property<const NavMesh::Surface*> NavMesh::SurfaceInstance::Shape() {
		using SurfaceRef = const Surface*;
		typedef SurfaceRef(*GetFn)(SurfaceInstance*);
		typedef void(*SetFn)(SurfaceInstance*, const SurfaceRef&);
		typedef void(*OnSurfaceDirtyFn)(SurfaceInstance*);
		return Property<const NavMesh::Surface*>(
			(GetFn)[](SurfaceInstance* self) -> SurfaceRef { return self->m_shape; },
			(SetFn)[](SurfaceInstance* self, const SurfaceRef& value) {
				const Callback<> onDirty(Helpers::OnSurfaceInstanceDirty, self);
				{
					std::unique_lock<decltype(self->m_lock)> lock(self->m_lock);
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
				self->m_enabled = value;
				Helpers::OnSurfaceInstanceDirty(self);
			}, this);
	}





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
