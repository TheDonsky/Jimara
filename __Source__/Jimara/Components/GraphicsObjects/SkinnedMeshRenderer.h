#pragma once
#include "TriMeshRenderer.h"


namespace Jimara {
	/// <summary>
	/// Component, that let's the render engine know, a skinned mesh has to be drawn somewhere
	/// </summary>
	class SkinnedMeshRenderer : public virtual TriMeshRenderer {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component (should have a Transform in parent heirarchy for the mesh to render) </param>
		/// <param name="name"> SkinnedMeshRenderer name </param>
		/// <param name="mesh"> Mesh to render (yes, does not have to be a skinned mesh to show up, but it better be) </param>
		/// <param name="material"> Material to use </param>
		/// <param name="instanced"> If true, the object will be piled up with similar renderers (with same mesh&material pairs) </param>
		/// <param name="isStatic"> In case you know that the mesh Transform and bones will stay constant, marking it static may save some clock cycles </param>
		/// <param name="bones"> List of bone transforms </param>
		/// <param name="boneCount"> Number of bone transforms </param>
		/// <param name="skeletonRoot"> Skeleton root (optional; mostly useful if one intends to reuse bones and place many instances of the same skinned mesh at multiple places and same pose) </param>
		SkinnedMeshRenderer(Component* parent, const std::string_view& name,
			const TriMesh* mesh = nullptr, const Jimara::Material* material = nullptr, bool instanced = true, bool isStatic = false,
			const Transform** bones = nullptr , size_t boneCount = 0, const Transform* skeletonRoot = nullptr);

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="parent"> Parent component (should have a Transform in parent heirarchy for the mesh to render) </param>
		/// <param name="name"> SkinnedMeshRenderer name </param>
		/// <param name="mesh"> Mesh to render (yes, does not have to be a skinned mesh to show up, but it better be) </param>
		/// <param name="material"> Material to use </param>
		/// <param name="instanced"> If true, the object will be piled up with similar renderers (with same mesh&material pairs) </param>
		/// <param name="isStatic"> In case you know that the mesh Transform and bones will stay constant, marking it static may save some clock cycles </param>
		/// <param name="bones"> List of bone transforms </param>
		/// <param name="boneCount"> Number of bone transforms </param>
		/// <param name="skeletonRoot"> Skeleton root (optional; mostly useful if one intends to reuse bones and place many instances of the same skinned mesh at multiple places and same pose) </param>
		SkinnedMeshRenderer(Component* parent, const std::string_view& name,
			const TriMesh* mesh, const Jimara::Material* material, bool instanced, bool isStatic,
			const Reference<const Transform>* bones, size_t boneCount, const Transform* skeletonRoot);

		/// <summary> Virtual destructor </summary>
		virtual ~SkinnedMeshRenderer();

		/// <summary> Skeleton root transform (this one will mostly be nullptr) </summary>
		const Transform* SkeletonRoot()const;

		/// <summary>
		/// Sets skeleton root transform
		/// Notes: 
		///		0. This is optional and mostly useful if one intends to reuse bones and place many instances of the same skinned mesh at multiple places and same pose;
		///		1. If set, the SkinnedMeshRenderer's transform will 
		/// </summary>
		/// <param name="skeletonRoot"> Transform to calculate bone deformation relative to </param>
		void SetSkeletonRoot(const Transform* skeletonRoot);

		/// <summary> Number of lined bones (may differ from the linked SkinnedTriMesh, the indices not covered here will simply be trated as nulls) </summary>
		size_t BoneCount()const;

		/// <summary>
		/// Linked bone by index
		/// </summary>
		/// <param name="index"> Bone index </param>
		/// <returns> Bone transform (can be null) </returns>
		const Transform* Bone(size_t index)const;

		/// <summary>
		/// Sets linked bone
		/// </summary>
		/// <param name="index"> Bone index (you don't need to pay attention to BoneCount() for this; it tracks SetBone() calls, not the other way around) </param>
		/// <param name="bone"> Bone to use (nullptr may result in BoneCount() decreasing) </param>
		void SetBone(size_t index, const Transform* bone);



	protected:
		/// <summary> 
		/// Invoked, whenever we change the mesh, the material, the material instance becomes dirty, object gets destroyed and etc...
		/// In short, whenever the TriMeshRenderer gets altered, we will enter this function
		/// </summary>
		virtual void OnTriMeshRendererDirty() final override;

	private:
		// Bone binding object
		class BoneBinding : public virtual Object {
		public:
			virtual ~BoneBinding();

			const Transform* Bone()const;

			void SetBone(const Transform* bone);

		private:
			Reference<const Transform> m_bone;

			void BoneDestroyed(Component*);
		};

		// Skeleton root object
		Reference<const Transform> m_skeletonRoot;

		// Bone objects
		std::vector<Reference<BoneBinding>> m_bones;

		// Number of "active bones" withing m_bones
		size_t m_boneCount = 0;

		// Underlying pipeline descriptor
		Reference<Object> m_pipelineDescriptor;

		// When skeleton root goes out of scope, we need to know about it
		void OnSkeletonRootDestroyed(Component*);
	};
}
