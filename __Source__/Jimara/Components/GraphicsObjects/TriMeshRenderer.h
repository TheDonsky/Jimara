#pragma once
#include "../Transform.h"
#include "../../Data/Mesh.h"
#include "../../Data/Material.h"


namespace Jimara {
	/// <summary>
	/// Common interfaces for MeshRenderer objects such as MeshRenderer/SkinnedMeshRenderer 
	/// and whatever other mesh-dependent renderer the project may have already or down the line
	/// </summary>
	class TriMeshRenderer : public virtual Component {
	public:
		/// <summary> Constructor </summary>
		TriMeshRenderer();

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
		/// Turns instancing on and off
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



	protected:
		/// <summary> 
		/// Invoked, whenever we change the mesh, the material, the material instance becomes dirty, object gets destroyed and etc...
		/// In short, whenever the TriMeshRenderer gets altered, we will enter this function
		/// </summary>
		virtual void OnTriMeshRendererDirty() = 0;

		/// <summary> Invoked, whenever the component becomes active in herarchy </summary>
		virtual void OnComponentEnabled()override;

		/// <summary> Invoked, whenever the component stops being active in herarchy </summary>
		virtual void OnComponentDisabled()override;

		/// <summary> We need to invoke OnTriMeshRendererDirty() the last time before deletion... </summary>
		virtual void OnOutOfScope()const override;


	private:
		// Mesh to render
		Reference<const TriMesh> m_mesh;

		// Material to render with
		Reference<const Jimara::Material> m_material;

		// Targetted material instance
		Reference<const Jimara::Material::Instance> m_materialInstance;

		// True if instancing is on
		std::atomic<bool> m_instanced = true;

		// True, if the geometry is marked static
		std::atomic<bool> m_isStatic = false;

		// Updates TriMeshRenderer state when each time the component heirarchy gets altered
		void RecreateOnParentChanged(const Component*);

		// Updates TriMeshRenderer state when when the component gets destroyed
		void RecreateWhenDestroyed(Component*);

		// Updates TriMeshRenderer state when the material instance gets invalidated
		void RecreateOnMaterialInstanceInvalidated(const Jimara::Material* material);
	};
}
