#include "NavMeshSurface.h"
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


namespace Jimara {
	struct NavMeshSurface::Helpers {
		struct StateSnapshot : public virtual Object {
			const Reference<NavMesh::SurfaceInstance> surfaceInstance;
			
			SpinLock stateLock;
			Reference<NavMesh::Surface> shape;
			Matrix4 pose = Math::Identity();
			bool enabled = false;

			std::atomic_bool flushScheduled = false;

			inline StateSnapshot(const Reference<NavMesh::SurfaceInstance>& surface) 
				: surfaceInstance(surface) {
				assert(surface != nullptr);
			}
			inline virtual ~StateSnapshot() {}

			void Flush(Object*, float) {
				flushScheduled = false;
				Reference<NavMesh::Surface> surface;
				Matrix4 transform;
				bool active;
				{
					std::unique_lock<decltype(stateLock)> lock(stateLock);
					surface = shape;
					transform = pose;
					active = enabled;
				}
				surfaceInstance->Enabled() = active;
				surfaceInstance->Transform() = transform;
				surfaceInstance->Shape() = surface;
			}
		};

		inline static void UpdateSurfaceSnapshot(NavMeshSurface* self) {
			Reference<StateSnapshot> snapshot = self->m_surfaceState;
			assert(snapshot != nullptr);
			const bool enabled = self->ActiveInHierarchy();
			const Transform* const transform = self->GetTransfrom();
			const Matrix4 pose = (transform != nullptr)
				? transform->FrameCachedWorldMatrix() : Math::Identity();
			{
				std::unique_lock<decltype(snapshot->stateLock)> lock(snapshot->stateLock);
				snapshot->pose = pose;
				snapshot->shape = self->m_surface;
				snapshot->enabled = enabled && (snapshot->shape != nullptr);
			}
			if (!snapshot->flushScheduled.load()) {
				snapshot->flushScheduled = true;
				snapshot->surfaceInstance->NavigationMesh()->EnqueueAsynchronousAction(
					Callback(&StateSnapshot::Flush, snapshot.operator->()), snapshot);
			}
		}

		inline static void UpdateSurfaceState(NavMeshSurface* self) {
			UpdateSurfaceSnapshot(self);
			const Callback<>& onUpdate = Callback<>(Helpers::UpdateSurfaceSnapshot, self);
			self->Context()->OnSynchOrUpdate() -= onUpdate;
			if (self->ActiveInHierarchy() && (!self->m_static.load()) && self->m_surface != nullptr)
				self->Context()->OnSynchOrUpdate() += onUpdate;
		}
	};

	NavMeshSurface::NavMeshSurface(Component* parent, const std::string_view& name) 
		: Component(parent, name)
		, m_surfaceState(Object::Instantiate<Helpers::StateSnapshot>(
			Object::Instantiate<NavMesh::SurfaceInstance>(NavMesh::Instance(parent->Context())))) {}

	NavMeshSurface::~NavMeshSurface() {
		assert(m_surface == nullptr);
	}

	void NavMeshSurface::MarkStatic(bool markStatic) {
		if (m_static.load() == markStatic)
			return;
		m_static = markStatic;
		Helpers::UpdateSurfaceState(this);
	}

	void NavMeshSurface::SetSurface(NavMesh::Surface* surface) {
		if (m_surface == surface)
			return;
		m_surface = surface;
		Helpers::UpdateSurfaceState(this);
	}

	void NavMeshSurface::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Surface, SetSurface, "Surface", "Navigation Mesh Surface geometry");
			JIMARA_SERIALIZE_FIELD_GET_SET(IsStatic, MarkStatic,
				"Is Static", "If true, the underlying surface instance pose will not be updated on each frame");
		};
		Helpers::UpdateSurfaceState(this);
	}

	void NavMeshSurface::OnComponentEnabled() {
		Helpers::UpdateSurfaceState(this);
	}

	void NavMeshSurface::OnComponentDisabled() {
		Helpers::UpdateSurfaceState(this);
	}

	void NavMeshSurface::OnParentChainDirty() {
		Helpers::UpdateSurfaceState(this);
	}

	void NavMeshSurface::OnComponentDestroyed() {
		SetSurface(nullptr);
		Helpers::UpdateSurfaceState(this);
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<NavMeshSurface>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<NavMeshSurface>(
			"Nav-Mesh Surface", "Jimara/Navigation/Nav-Mesh Surface", "Navigation Mesh surface geometry");
		report(factory);
	}
}
