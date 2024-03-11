#pragma once
#include "NavMesh.h"
#include <Jimara/Components/Transform.h>


namespace Jimara {
	JIMARA_REGISTER_TYPE(Jimara::NavMeshSurface);

	class JIMARA_STATE_MACHINES_API NavMeshSurface : public virtual Component {
	public:
		NavMeshSurface(Component* parent, const std::string_view& name = "NavMeshSurface");

		virtual ~NavMeshSurface();

		inline bool IsStatic()const { return m_static.load(); }

		void MarkStatic(bool markStatic);

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer should be reported by invoking this callback with serializer & corresonding target as parameters </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

	protected:
		virtual void OnComponentEnabled()override;

		virtual void OnComponentDisabled()override;

		virtual void OnParentChainDirty()override;

		virtual void OnComponentDestroyed()override;

	private:
		const Reference<NavMesh::SurfaceInstance> m_surfaceInstance;
		std::atomic_bool m_static = false;

		struct Helpers;
	};

	// Report factory
	template<> JIMARA_STATE_MACHINES_API void TypeIdDetails::GetTypeAttributesOf<NavMeshSurface>(const Callback<const Object*>& report);
}
