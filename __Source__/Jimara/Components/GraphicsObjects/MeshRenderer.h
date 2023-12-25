#pragma once
#include "TriMeshRenderer.h"
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

		/// <summary> Retrieves MeshRenderer boundaries in local-space </summary>
		AABB GetLocalBoundaries()const;

		/// <summary> Retrieves MeshRenderer boundaries in world-space </summary>
		virtual AABB GetBoundaries()const override;

		/// <summary> Renderer cull options </summary>
		struct JIMARA_API RendererCullingOptions {
			/// <summary> 
			/// 'Natural' culling boundary of the geometry will be expanded by this amount in each direction in local space 
			/// (Useful for the cases when the shader does some vertex displacement and the visible geometry goes out of initial boundaries)
			/// </summary>
			Vector3 boundaryThickness = Vector3(0.0f);

			/// <summary> Local-space culling boundary will be offset by this amount </summary>
			Vector3 boundaryOffset = Vector3(0.0f);

			/// <summary>
			/// Comparizon operator
			/// </summary>
			/// <param name="other"> Other options </param>
			/// <returns> True, if the options are the same </returns>
			bool operator==(const RendererCullingOptions& other)const;

			/// <summary>
			/// Comparizon operator
			/// </summary>
			/// <param name="other"> Other options </param>
			/// <returns> True, if the options are not the same </returns>
			inline bool operator!=(const RendererCullingOptions& other)const { return !((*this) == other); }

			/// <summary> Default serializer of CullingOptions </summary>
			struct JIMARA_API Serializer;
		};

		/// <summary> Renderer cull options </summary>
		inline const RendererCullingOptions& CullingOptions()const { return m_cullingOptions; }

		/// <summary>
		/// Updates cull options
		/// </summary>
		/// <param name="options"> Culling options to use </param>
		void SetCullingOptions(const RendererCullingOptions& options);

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
		RendererCullingOptions m_cullingOptions;

		// Some private stuff resides in here...
		struct Helpers;
	};


	/// <summary> Default serializer of CullingOptions </summary>
	struct JIMARA_API MeshRenderer::RendererCullingOptions::Serializer : public virtual Serialization::SerializerList::From<MeshRenderer::RendererCullingOptions> {
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="name"> Name of the ItemSerializer </param>
		/// <param name="hint"> Target hint (editor helper texts on hover and what not) </param>
		/// <param name="attributes"> Serializer attributes </param>
		inline Serializer(const std::string_view& name, const std::string_view& hint = "", const std::vector<Reference<const Object>>& attributes = {})
			: Serialization::ItemSerializer(name, hint, attributes) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~Serializer() {}

		/// <summary>
		/// Gives access to sub-serializers/fields
		/// </summary>
		/// <param name="recordElement"> Each sub-serializer will be reported by invoking this callback with serializer & corresonding target as parameters </param>
		/// <param name="targetAddr"> Serializer target object </param>
		virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, RendererCullingOptions* target)const override;
	};



	// Type detail callbacks
	template<> inline void TypeIdDetails::GetParentTypesOf<MeshRenderer>(const Callback<TypeId>& report) { 
		report(TypeId::Of<TriMeshRenderer>()); 
		report(TypeId::Of<BoundedObject>());
	}
	template<> JIMARA_API void TypeIdDetails::GetTypeAttributesOf<MeshRenderer>(const Callback<const Object*>& report);
}
