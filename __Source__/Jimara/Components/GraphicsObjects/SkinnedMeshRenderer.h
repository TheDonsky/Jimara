#pragma once
#include "TriMeshRenderer.h"


namespace Jimara {
	class SkinnedMeshRenderer : public virtual TriMeshRenderer {
	public:
		SkinnedMeshRenderer(Component* parent, const std::string_view& name,
			const TriMesh* mesh = nullptr, const Jimara::Material* material = nullptr, bool instanced = true, bool isStatic = false,
			const Transform** bones = nullptr , size_t boneCount = 0, const Transform* skeletonRoot = nullptr);

		SkinnedMeshRenderer(Component* parent, const std::string_view& name,
			const TriMesh* mesh, const Jimara::Material* material, bool instanced, bool isStatic,
			const Reference<const Transform>* bones, size_t boneCount, const Transform* skeletonRoot);

		const Transform* SkeletonRoot()const;

		void SetSkeletonRoot(const Transform* skeletonRoot);

		size_t BoneCount()const;

		const Transform* Bone(size_t index)const;

		void SetBone(size_t index, const Transform* bone);

	protected:
		/// <summary> 
		/// Invoked, whenever we change the mesh, the material, the material instance becomes dirty, object gets destroyed and etc...
		/// In short, whenever the TriMeshRenderer gets altered, we will enter this function
		/// </summary>
		virtual void OnTriMeshRendererDirty() override;

	private:
		class BoneBinding : public virtual Object {
		public:
			const Transform* Bone()const;

			void SetBone(const Transform* bone);

		private:
			Reference<const Transform> m_bone;

			void BoneDestroyed(Component*);
		};
		Reference<const Transform> m_skeletonRoot;
		std::vector<Reference<BoneBinding>> m_bones;
		size_t m_boneCount = 0;

		// Underlying pipeline descriptor
		Reference<Object> m_pipelineDescriptor;

		void OnSkeletonRootDestroyed(Component*);
	};
}
