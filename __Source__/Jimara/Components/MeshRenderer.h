#pragma once
#include "Component.h"
#include "Transform.h"
#include "../Data/Mesh.h"
#include "../Data/Material.h"


namespace Jimara {
	/// <summary>
	/// Component, that let's the render engine know, a mesh has to be drawn somewhere
	/// </summary>
	class MeshRenderer : public virtual Component {
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
		MeshRenderer(Component* parent, const std::string& name, const TriMesh* mesh = nullptr, const Jimara::Material* material = nullptr, bool instanced = true, bool isStatic = false);

		/// <summary> Virtual destructor </summary>
		virtual ~MeshRenderer();

		/// <summary> Mesh to render </summary>
		const TriMesh* Mesh()const;

		/// <summary> 
		/// Sets new mesh to render 
		/// </summary>
		/// <param name="mesh"> New mesh </param>
		void SetMesh(const TriMesh* mesh);

		/// <summary> Material to render with </summary>
		const Jimara::Material* Material()const;

		/// <summary>
		/// Sets new material to use
		/// </summary>
		/// <param name="material"> New material </param>
		void SetMaterial(const Jimara::Material* material);

		/// <summary> Material instance the renderer uses </summary>
		const Jimara::Material::Instance* MaterialInstance()const;

		/// <summary>
		/// Sets new material instance to use (will discard the Material connection)
		/// </summary>
		/// <param name="materialInstance"> New material instance </param>
		void SetMaterialInstance(const Jimara::Material::Instance* materialInstance);

		/// <summary> True, if the mesh is expected to be instanced </summary>
		bool IsInstanced()const;

		/// <summary>
		/// Turns instancing on & off
		/// </summary>
		/// <param name="instanced"> If true, the mesh will be instanced </param>
		void RenderInstanced(bool instanced);

		/// <summary> If true, the renderer assumes the mesh transform stays constant and saves some CPU cycles doing that </summary>
		bool IsStatic()const;

		/// <summary>
		/// Marks the mesh renderer static
		/// </summary>
		/// <param name="isStatic"> If true, the renderer will assume the mesh transform stays constant and saves some CPU cycles doing that </param>
		void MarkStatic(bool isStatic);


	private:
		// Mesh to render
		Reference<const TriMesh> m_mesh;

		// Material to render with
		Reference<const Jimara::Material> m_material;

		// Targetted material instance
		Reference<const Jimara::Material::Instance> m_materialInstance;

		// True if instancing is on
		std::atomic<bool> m_instanced;

		// True, if the geometry is marked static
		std::atomic<bool> m_isStatic;

		// Becomes false after the mesh gets destroyed
		std::atomic<bool> m_alive;

		// Transform component used the last time the pipeline descriptor was recreated
		Reference<Transform> m_descriptorTransform;

		// Underlying pipeline descriptor
		Reference<Object> m_pipelineDescriptor;

		// Recreates descriptor
		void RecreatePipelineDescriptor();

		// Recreates descriptor each time the component heirarchy gets altered
		void RecreateOnParentChanged(const Component*);

		// Retire descriptor when the component gets destroyed
		void RecreateWhenDestroyed(Component*);

		// Recreates descriptor each time the shared instance of the material goes out of scope
		void RecreateOnMaterialInstanceInvalidated(const Jimara::Material* material);
	};
}
