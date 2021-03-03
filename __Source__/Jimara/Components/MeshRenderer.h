#pragma once
#include "Component.h"
#include "../Data/Mesh.h"
#include "../Data/Material.h"


namespace Jimara {
	class MeshRenderer : public virtual Component {
	public:
		MeshRenderer(Component* parent, const std::string& name, const TriMesh* mesh = nullptr, const Jimara::Material* material = nullptr, bool instanced = true);

		virtual ~MeshRenderer();

		const TriMesh* Mesh()const;

		void SetMesh(const TriMesh* mesh);

		const Jimara::Material* Material()const;

		void SetMaterial(const Jimara::Material* material);

		bool IsInstanced()const;

		void RenderInstanced(bool instanced);


	private:
		Reference<const TriMesh> m_mesh;
		Reference<const Jimara::Material> m_material;
		std::atomic<bool> m_instanced;
		std::atomic<bool> m_alive;

		Transform* m_descriptorTransform;
		Reference<Object> m_pipelineDescriptor;

		void RecreatePipelineDescriptor();
		void RecreateOnParentChanged(const Component*, Component*);
		void RecreateWhenDestroyed(const Component*);
	};
}
