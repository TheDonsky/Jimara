#pragma once
#include "TriMeshRenderer.h"


namespace Jimara {
	/// <summary> This will make sure, MeshRenderer is registered with BuiltInTypeRegistrator </summary>
	JIMARA_REGISTER_TYPE(Jimara::MeshRenderer);

	/// <summary>
	/// Component, that let's the render engine know, a mesh has to be drawn somewhere
	/// </summary>
	class JIMARA_API MeshRenderer : public virtual TriMeshRenderer {
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


	protected:
		/// <summary> 
		/// Invoked, whenever we change the mesh, the material, the material instance becomes dirty, object gets destroyed and etc...
		/// In short, whenever the TriMeshRenderer gets altered, we will enter this function
		/// </summary>
		virtual void OnTriMeshRendererDirty() final override;


	private:
		// Transform component used the last time the pipeline descriptor was recreated
		Reference<Transform> m_descriptorTransform;

		// Underlying pipeline descriptor
		Reference<Object> m_pipelineDescriptor;
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<MeshRenderer>(const Callback<TypeId>& report) { report(TypeId::Of<TriMeshRenderer>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<MeshRenderer>(const Callback<const Object*>& report);
}
