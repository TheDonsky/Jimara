#include "NavMeshSurface.h"
#include <Jimara/Data/Serialization/Helpers/SerializerMacros.h>


namespace Jimara {
	struct NavMeshSurface::Helpers {
		inline static void UpdateSurfaceTransform(NavMeshSurface* self) {
			const Transform* transform = self->GetTransfrom();
			self->m_surfaceInstance->Transform() = (transform != nullptr)
				? transform->FrameCachedWorldMatrix() : Math::Identity();
		}

		inline static void UpdateSurfaceState(NavMeshSurface* self) {
			const bool active = self->ActiveInHeirarchy();
			self->m_surfaceInstance->Enabled() = active;
			const Callback<>& onUpdate = Callback(Helpers::UpdateSurfaceTransform, self);
			self->Context()->OnUpdate() -= onUpdate;
			if (self->m_static.load())
				UpdateSurfaceTransform(self);
			else if (active)
				self->Context()->OnUpdate() += onUpdate;
		}
	};

	NavMeshSurface::NavMeshSurface(Component* parent, const std::string_view& name) 
		: Component(parent, name)
		, m_surfaceInstance(Object::Instantiate<NavMesh::SurfaceInstance>(NavMesh::Instance(parent->Context()))) {}

	NavMeshSurface::~NavMeshSurface() {
		assert(!m_surfaceInstance->Enabled());
		assert(m_surfaceInstance->Shape() == nullptr);
	}

	void NavMeshSurface::MarkStatic(bool markStatic) {
		if (m_static.load() == markStatic)
			return;
		m_static = markStatic;
		Helpers::UpdateSurfaceState(this);
	}

	void NavMeshSurface::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Surface, SetSurface, "Surface", "Navigation Mesh Surface geometry");
			JIMARA_SERIALIZE_FIELD_GET_SET(IsStatic, MarkStatic,
				"Is Static", "If true, the underlying surface instance pose will not be updated on each frame");
		};
	}

	void NavMeshSurface::OnComponentEnabled() {
		Helpers::UpdateSurfaceState(this);
		Helpers::UpdateSurfaceTransform(this);
	}

	void NavMeshSurface::OnComponentDisabled() {
		Helpers::UpdateSurfaceState(this);
		Helpers::UpdateSurfaceTransform(this);
	}

	void NavMeshSurface::OnParentChainDirty() {
		Helpers::UpdateSurfaceState(this);
		Helpers::UpdateSurfaceTransform(this);
	}

	void NavMeshSurface::OnComponentDestroyed() { 
		m_surfaceInstance->Enabled() = false;
		m_surfaceInstance->Shape() = nullptr;
		Helpers::UpdateSurfaceState(this);
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<NavMeshSurface>(const Callback<const Object*>& report) {
		static const Reference<ComponentFactory> factory = ComponentFactory::Create<NavMeshSurface>(
			"Nav-Mesh Surface", "Jimara/Navigation/Nav-Mesh Surface", "Navigation Mesh surface geometry");
		report(factory);
	}
}
