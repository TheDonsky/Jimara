#include "MeshRenderer.h"

namespace Jimara {
	MeshRenderer::MeshRenderer(Component* parent, const std::string& name, const TriMesh* mesh, const Jimara::Material* material, bool instanced)
		: Component(parent, name), m_mesh(mesh), m_material(material), m_instanced(instanced) {}

	const TriMesh* MeshRenderer::Mesh()const { return m_mesh; }

	void MeshRenderer::SetMesh(const TriMesh* mesh) {
		if (mesh == m_mesh) return;
		m_mesh = mesh;
		// __TODO__: Implement this crap!!!
		Context()->Log()->Warning("MeshRenderer::SetMesh - Not yet implemented!");
	}

	const Jimara::Material* MeshRenderer::Material()const { return m_material; }

	void MeshRenderer::SetMaterial(const Jimara::Material* material) {
		if (material == m_material) return;
		m_material = material;
		// __TODO__: Implement this crap!!!
		Context()->Log()->Warning("MeshRenderer::SetMaterial - Not yet implemented!");
	}

	bool MeshRenderer::IsInstanced()const { return m_instanced; }

	void MeshRenderer::RenderInstanced(bool instanced) {
		if (instanced == m_instanced) return;
		m_instanced = instanced;
		// __TODO__: Implement this crap!!!
		Context()->Log()->Warning("MeshRenderer::RenderInstanced - Not yet implemented!");
	}
}
