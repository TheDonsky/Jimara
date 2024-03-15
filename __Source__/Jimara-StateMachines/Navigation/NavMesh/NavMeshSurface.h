#pragma once
#include "NavMesh.h"
#include <Jimara/Components/Transform.h>


namespace Jimara {
	JIMARA_REGISTER_TYPE(Jimara::NavMeshSurface);

	/// <summary>
	/// Navigation mesh surface component
	/// </summary>
	class JIMARA_STATE_MACHINES_API NavMeshSurface : public virtual Component {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component </param>
		/// <param name="name"> Surface name </param>
		NavMeshSurface(Component* parent, const std::string_view& name = "NavMeshSurface");

		/// <summary> Virtual destructor </summary>
		virtual ~NavMeshSurface();

		/// <summary> If set, this flag prevents the surface from updating pose on each frame </summary>
		inline bool IsStatic()const { return m_static.load(); }

		/// <summary>
		/// Sets static flag
		/// </summary>
		/// <param name="markStatic"> If true, this prevents the surface from updating pose on each frame </param>
		void MarkStatic(bool markStatic);

		/// <summary> Navigation mesh Surface geometry </summary>
		inline NavMesh::Surface* Surface()const { return m_surfaceInstance->Shape(); }

		/// <summary>
		/// Sets navigation mesh surface geometry
		/// </summary>
		/// <param name="surface"> NavMesh::Surface </param>
		inline void SetSurface(NavMesh::Surface* surface) { m_surfaceInstance->Shape() = surface; }

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer should be reported by invoking this callback with serializer & corresonding target as parameters </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

	protected:
		/// <summary> Invoked, when the component becomes active </summary>
		virtual void OnComponentEnabled()override;

		/// <summary> Invoked, when the component becomes inactive </summary>
		virtual void OnComponentDisabled()override;

		/// <summary> Invoked, when the component parent chain gets altered </summary>
		virtual void OnParentChainDirty()override;

		/// <summary> Invoked, when the component is destroyed </summary>
		virtual void OnComponentDestroyed()override;

	private:
		const Reference<NavMesh::SurfaceInstance> m_surfaceInstance;
		std::atomic_bool m_static = false;

		struct Helpers;
	};

	// Report factory
	template<> JIMARA_STATE_MACHINES_API void TypeIdDetails::GetTypeAttributesOf<NavMeshSurface>(const Callback<const Object*>& report);
}
