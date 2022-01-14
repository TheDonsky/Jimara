#include "TriMeshRenderer.h"


namespace Jimara {
	TriMeshRenderer::TriMeshRenderer() {
		OnParentChanged() += Callback(&TriMeshRenderer::RecreateOnParentChanged, this);
		OnDestroyed() += Callback(&TriMeshRenderer::RecreateWhenDestroyed, this);
	}

	const TriMesh* TriMeshRenderer::Mesh()const { return m_mesh; }

	void TriMeshRenderer::SetMesh(const TriMesh* mesh) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		if (Destroyed()) mesh = nullptr;
		if (mesh == m_mesh) return;
		m_mesh = mesh;
		OnTriMeshRendererDirty();
	}

	const Jimara::Material* TriMeshRenderer::Material()const { return m_material; }

	void TriMeshRenderer::SetMaterial(const Jimara::Material* material) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		if (Destroyed()) material = nullptr;
		if (material == m_material) return;
		if (m_material != nullptr)
			m_material->OnInvalidateSharedInstance() -= Callback(&TriMeshRenderer::RecreateOnMaterialInstanceInvalidated, this);
		m_material = material;
		if (m_material != nullptr) {
			if (!Destroyed())
				m_material->OnInvalidateSharedInstance() += Callback(&TriMeshRenderer::RecreateOnMaterialInstanceInvalidated, this);
			Jimara::Material::Reader reader(material);
			Reference<const Jimara::Material::Instance> instance = reader.SharedInstance();
			if (instance == m_materialInstance) return; // Stuff will auto-resolve in this case
			m_materialInstance = instance;
		}
		else m_materialInstance = nullptr;
		OnTriMeshRendererDirty();
	}

	const Jimara::Material::Instance* TriMeshRenderer::MaterialInstance()const { return m_materialInstance; }

	void TriMeshRenderer::SetMaterialInstance(const Jimara::Material::Instance* materialInstance) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		if (Destroyed()) materialInstance = nullptr;
		if (m_material != nullptr) SetMaterial(nullptr);
		else if (m_materialInstance == materialInstance) return;
		m_materialInstance = materialInstance;
		OnTriMeshRendererDirty();
	}

	bool TriMeshRenderer::IsInstanced()const { return m_instanced; }

	void TriMeshRenderer::RenderInstanced(bool instanced) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		if (instanced == m_instanced) return;
		m_instanced = instanced;
		OnTriMeshRendererDirty();
	}

	bool TriMeshRenderer::IsStatic()const { return m_isStatic; }

	void TriMeshRenderer::MarkStatic(bool isStatic) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		if (isStatic == m_isStatic) return;
		m_isStatic = isStatic;
		OnTriMeshRendererDirty();
	}

	void TriMeshRenderer::OnComponentEnabled() {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		OnTriMeshRendererDirty();
	}

	void TriMeshRenderer::OnComponentDisabled() {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		OnTriMeshRendererDirty();
	}

	void TriMeshRenderer::OnOutOfScope()const {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		TriMeshRenderer* self = ((TriMeshRenderer*)this); // Yep, we ignore the const statement, but this is an edge-case...
		self->OnParentChanged() -= Callback(&TriMeshRenderer::RecreateOnParentChanged, self);
		self->OnDestroyed() -= Callback(&TriMeshRenderer::RecreateWhenDestroyed, self);
		self->m_mesh = nullptr;
		self->SetMaterial(nullptr);
		self->OnTriMeshRendererDirty();
		Object::OnOutOfScope();
	}

	void TriMeshRenderer::RecreateOnParentChanged(const Component*) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		OnTriMeshRendererDirty();
	}

	void TriMeshRenderer::RecreateWhenDestroyed(Component*) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		m_mesh = nullptr;
		SetMaterial(nullptr);
		OnTriMeshRendererDirty();
	}

	void TriMeshRenderer::RecreateOnMaterialInstanceInvalidated(const Jimara::Material* material) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		if (material != nullptr) {
			if (material == m_material) {
				Material::Reader reader(material);
				m_materialInstance = reader.SharedInstance();
			}
		}
		OnTriMeshRendererDirty();
	}
}
