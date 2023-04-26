#pragma once
#include "../Transform.h"
#include "../../Data/Mesh.h"
#include "../../Data/Material.h"
#include "../../Environment/Layers.h"


namespace Jimara {
	/// <summary>
	/// Common interfaces for MeshRenderer objects such as MeshRenderer/SkinnedMeshRenderer 
	/// and whatever other mesh-dependent renderer the project may have already or down the line
	/// </summary>
	class JIMARA_API TriMeshRenderer : public virtual Component {
	public:
		/// <summary> Constructor </summary>
		TriMeshRenderer();

		/// <summary> Mesh to render </summary>
		TriMesh* Mesh()const;

		/// <summary> 
		/// Sets new mesh to render 
		/// </summary>
		/// <param name="mesh"> New mesh </param>
		void SetMesh(TriMesh* mesh);

		/// <summary> Material to render with </summary>
		Jimara::Material* Material()const;

		/// <summary>
		/// Sets new material to use
		/// </summary>
		/// <param name="material"> New material </param>
		void SetMaterial(Jimara::Material* material);

		/// <summary> Material instance the renderer uses </summary>
		const Jimara::Material::Instance* MaterialInstance();

		/// <summary>
		/// Sets new material instance to use (will discard the Material connection)
		/// </summary>
		/// <param name="materialInstance"> New material instance </param>
		void SetMaterialInstance(const Jimara::Material::Instance* materialInstance);

		/// <summary> Graphics object layer (for renderer filtering) </summary>
		Jimara::Layer Layer()const;

		/// <summary>
		/// Sets graphics object layer (for renderer filtering)
		/// </summary>
		/// <param name="layer"> New layer </param>
		void SetLayer(Jimara::Layer layer);

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

		/// <summary> Tells, how the mesh is supposed to be rendered (refer to Graphics::GraphicsPipeline::IndexType for more details) </summary>
		Graphics::GraphicsPipeline::IndexType GeometryType()const;

		/// <summary>
		/// Sets how the mesh is supposed to be rendered (refer to Graphics::GraphicsPipeline::IndexType for more details)
		/// </summary>
		/// <param name="geometryType"> TRIANGLE/LINE </param>
		void SetGeometryType(Graphics::Legacy::GraphicsPipeline::IndexType geometryType);

		/// <summary> TriMeshRenderer "configuration" (can be used as a key) </summary>
		struct JIMARA_API Configuration;

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
		virtual void OnTriMeshRendererDirty() = 0;

		/// <summary> Invoked by the scene on the first frame this component gets instantiated </summary>
		virtual void OnComponentInitialized()override;

		/// <summary> Invoked, whenever the component becomes active in herarchy </summary>
		virtual void OnComponentEnabled()override;

		/// <summary> Invoked, whenever the component stops being active in herarchy </summary>
		virtual void OnComponentDisabled()override;

		/// <summary> We need to invoke OnTriMeshRendererDirty() the last time before deletion... </summary>
		virtual void OnOutOfScope()const override;


	private:
		// Mesh to render
		Reference<TriMesh> m_mesh;

		// Material to render with
		Reference<Jimara::Material> m_material;

		// Targetted material instance
		Reference<const Jimara::Material::Instance> m_materialInstance;

		// Graphics layer to use
		std::atomic<Jimara::Layer> m_layer = 0;

		// True if instancing is on
		bool m_instanced = true;

		// True, if the geometry is marked static
		bool m_isStatic = false;

		// True, if OnTriMeshRendererDirty call is 'schedules'
		bool m_dirty = false;

		// Tells, how the mesh is supposed to be rendered
		std::atomic<Graphics::GraphicsPipeline::IndexType> m_geometryType = Graphics::GraphicsPipeline::IndexType::TRIANGLE;

		// Schedules OnTriMeshRendererDirty call (this way we avoid entering it multiple times per frame)
		void ScheduleOnTriMeshRendererDirtyCall();

		// Updates TriMeshRenderer state when each time the component heirarchy gets altered
		void RecreateOnParentChanged(ParentChangeInfo);

		// Updates TriMeshRenderer state when when the component gets destroyed
		void RecreateWhenDestroyed(Component*);

		// Updates TriMeshRenderer state when the material instance gets invalidated
		void RecreateOnMaterialInstanceInvalidated(const Jimara::Material* material);
	};

	/// <summary> TriMeshRenderer "configuration" (can be used as a key) </summary>
	struct JIMARA_API TriMeshRenderer::Configuration {
		/// <summary> Scene context (same as the TriMeshRenderer's context) </summary>
		Reference<SceneContext> context;

		/// <summary> Triangle mesh set with SetMesh </summary>
		Reference<const TriMesh> mesh;

		/// <summary> Instance of a material (set by SetMaterial/SetMaterialInstance) </summary>
		Reference<const Material::Instance> material;

		/// <summary> Graphics layer, assigned to the renderer in question (set by SetLayer) </summary>
		Jimara::Layer layer = 0;

		/// <summary> True, if the TriMeshRenderer's 'IsStatic' flag is set </summary>
		bool isStatic = false;

		/// <summary> Renderer's geometry type </summary>
		Graphics::GraphicsPipeline::IndexType geometryType = Graphics::GraphicsPipeline::IndexType::TRIANGLE;

		/// <summary> Default constructor </summary>
		inline Configuration() {}

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="renderer"> TriMeshRenderer </param>
		Configuration(TriMeshRenderer* renderer);

		/// <summary>
		/// Compares to 'other' renderer's configuration
		/// </summary>
		/// <param name="configuration"> Configuration to compare to </param>
		/// <returns> True, is the other is 'greater than' this </returns>
		bool operator<(const Configuration& configuration)const;

		/// <summary>
		/// Compares to 'other' renderer's configuration
		/// </summary>
		/// <param name="configuration"> Configuration to compare to </param>
		/// <returns> True, is the other is 'equal to' this </returns>
		bool operator==(const Configuration& configuration)const;

		/// <summary>
		/// Compares to 'other' renderer's configuration
		/// </summary>
		/// <param name="configuration"> Configuration to compare to </param>
		/// <returns> True, is the other is 'not equal to' this </returns>
		inline bool operator!=(const Configuration& configuration)const { return !((*this) == configuration); }
	};

	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<TriMeshRenderer>(const Callback<TypeId>& report) { report(TypeId::Of<Component>()); }
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<TriMeshRenderer>(const Callback<const Object*>& report);
}

namespace std {
	/// <summary> std::hash override for TriMeshRenderer::Configuration </summary>
	template<>
	struct hash<Jimara::TriMeshRenderer::Configuration> {
		/// <summary>
		/// Hash function
		/// </summary>
		/// <param name="configuration"> TriMeshRenderer::Configuration </param>
		/// <returns> Hash of configuration </returns>
		size_t operator()(const Jimara::TriMeshRenderer::Configuration& configuration)const;
	};
}

