#pragma once
#include "TriMeshRenderer.h"
#include "GeometryRendererCullingOptions.h"
#include "../../Data/Geometry/MeshBoundingBox.h"


namespace Jimara {
	/// <summary> This will make sure, MeshRenderer is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::MeshRenderer);

	/// <summary>
	/// Component, that let's the render engine know, a mesh has to be drawn somewhere
	/// </summary>
	class JIMARA_API MeshRenderer : public virtual TriMeshRenderer, public virtual BoundedObject {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component (should have a Transform in parent heirarchy for the mesh to render) </param>
		/// <param name="name"> MeshRenderer name </param>
		/// <param name="mesh"> Mesh to render </param>
		/// <param name="material"> Material to use </param>
		/// <param name="instanced"> If this is true, the mesh-material pairs will be batched </param>
		/// <param name="isStatic"> In case you know that the mesh Transform will stay constant, marking it static may save some clock cycles </param>
		MeshRenderer(Component* parent, const std::string_view& name = "MeshRenderer",
			TriMesh* mesh = nullptr, Jimara::Material* material = nullptr, bool instanced = true, bool isStatic = false);

		/// <summary> Virtual destructor </summary>
		virtual ~MeshRenderer();

		/// <summary> Retrieves MeshRenderer boundaries in local-space </summary>
		AABB GetLocalBoundaries()const;

		/// <summary> Retrieves MeshRenderer boundaries in world-space </summary>
		virtual AABB GetBoundaries()const override;

		/// <summary> Renderer cull options </summary>
		using RendererCullingOptions = GeometryRendererCullingOptions;

		/// <summary> Renderer cull options </summary>
		inline const RendererCullingOptions::ConfigurableOptions& CullingOptions()const { return m_cullingOptions; }

		/// <summary> Renderer cull options </summary>
		inline RendererCullingOptions::ConfigurableOptions& CullingOptions() { return m_cullingOptions; }

		/// <summary>
		/// Exposes fields to serialization utilities
		/// </summary>
		/// <param name="recordElement"> Reports elements with this </param>
		virtual void GetFields(Callback<Serialization::SerializedObject> recordElement)override;

	protected:
		/// <summary> 
		/// Invoked, whenever we change the mesh, the material, the material instance becomes dirty, object gets destroyed and etc...
		/// In short, whenever the TriMeshRenderer gets altered, we will enter this function
		/// </summary>
		virtual void OnTriMeshRendererDirty() final override;


	private:
		// Underlying pipeline descriptor
		Reference<Object> m_pipelineDescriptor;

		// Mesh boundaries
		mutable SpinLock m_meshBoundsLock;
		mutable Reference<TriMeshBoundingBox> m_meshBounds;
		RendererCullingOptions::ConfigurableOptions m_cullingOptions;

		// Some private stuff resides in here...
		struct Helpers;
	};


	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<MeshRenderer>(const Callback<TypeId>& report) { 
		report(TypeId::Of<TriMeshRenderer>()); 
		report(TypeId::Of<BoundedObject>());
	}
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<MeshRenderer>(const Callback<const Object*>& report);
}
