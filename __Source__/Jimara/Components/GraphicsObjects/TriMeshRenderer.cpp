#include "TriMeshRenderer.h"
#include "../../Data/Materials/SampleDiffuse/SampleDiffuseShader.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	TriMeshRenderer::TriMeshRenderer() {
		OnParentChanged() += Callback(&TriMeshRenderer::RecreateOnParentChanged, this);
		OnDestroyed() += Callback(&TriMeshRenderer::RecreateWhenDestroyed, this);
	}

	TriMesh* TriMeshRenderer::Mesh()const { return m_mesh; }

	void TriMeshRenderer::SetMesh(TriMesh* mesh) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		if (Destroyed()) mesh = nullptr;
		if (mesh == m_mesh) return;
		m_mesh = mesh;
		ScheduleOnTriMeshRendererDirtyCall();
	}

	Jimara::Material* TriMeshRenderer::Material()const { return m_material; }

	void TriMeshRenderer::SetMaterial(Jimara::Material* material) {
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
		ScheduleOnTriMeshRendererDirtyCall();
	}

	const Jimara::Material::Instance* TriMeshRenderer::MaterialInstance() { 
		if (m_materialInstance == nullptr) 
			m_materialInstance = SampleDiffuseShader::MaterialInstance(Context()->Graphics()->Device());
		return m_materialInstance;
	}

	void TriMeshRenderer::SetMaterialInstance(const Jimara::Material::Instance* materialInstance) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		if (Destroyed()) materialInstance = nullptr;
		if (m_material != nullptr) SetMaterial(nullptr);
		else if (m_materialInstance == materialInstance) return;
		m_materialInstance = materialInstance;
		ScheduleOnTriMeshRendererDirtyCall();
	}

	GraphicsLayer TriMeshRenderer::Layer()const { return m_layer; }

	void TriMeshRenderer::SetLayer(GraphicsLayer layer) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		if (layer == m_layer) return;
		m_layer = layer;
		ScheduleOnTriMeshRendererDirtyCall();
	}

	bool TriMeshRenderer::IsInstanced()const { return m_instanced; }

	void TriMeshRenderer::RenderInstanced(bool instanced) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		if (instanced == m_instanced) return;
		m_instanced = instanced;
		ScheduleOnTriMeshRendererDirtyCall();
	}

	bool TriMeshRenderer::IsStatic()const { return m_isStatic; }

	void TriMeshRenderer::MarkStatic(bool isStatic) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		if (isStatic == m_isStatic) return;
		m_isStatic = isStatic;
		ScheduleOnTriMeshRendererDirtyCall();
	}

	Graphics::GraphicsPipeline::IndexType TriMeshRenderer::GeometryType()const { return m_geometryType; }

	void TriMeshRenderer::SetGeometryType(Graphics::GraphicsPipeline::IndexType geometryType) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		if (geometryType == m_geometryType) return;
		m_geometryType = geometryType;
		ScheduleOnTriMeshRendererDirtyCall();
	}

	void TriMeshRenderer::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		Component::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement, {
			JIMARA_SERIALIZE_FIELD_GET_SET(Mesh, SetMesh, "Mesh", "Mesh to render");
			JIMARA_SERIALIZE_FIELD_GET_SET(Material, SetMaterial, "Material", "Material to render with");
			JIMARA_SERIALIZE_FIELD_GET_SET(Layer, SetLayer, "Layer", "Graphics object layer (for renderer filtering)");
			JIMARA_SERIALIZE_FIELD_GET_SET(IsInstanced, RenderInstanced, "Instanced", "Set to true, if the mesh is supposed to be instanced");
			JIMARA_SERIALIZE_FIELD_GET_SET(IsStatic, MarkStatic, "Static", "If true, the renderer assumes the mesh transform stays constant and saves some CPU cycles doing that");
			JIMARA_SERIALIZE_FIELD_GET_SET(GeometryType, SetGeometryType, "Geometry Type", "Tells, how the mesh is supposed to be rendered (TRIANGLE/EDGE)",
				Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<Graphics::GraphicsPipeline::IndexType>>>(false,
					"TRIANGLE", Graphics::GraphicsPipeline::IndexType::TRIANGLE,
					"EDGE", Graphics::GraphicsPipeline::IndexType::EDGE));
			});
	}

	void TriMeshRenderer::OnComponentInitialized() {
		ScheduleOnTriMeshRendererDirtyCall();
	}

	void TriMeshRenderer::OnComponentEnabled() {
		ScheduleOnTriMeshRendererDirtyCall();
	}

	void TriMeshRenderer::OnComponentDisabled() {
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

	void TriMeshRenderer::ScheduleOnTriMeshRendererDirtyCall() {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		if (m_dirty.load()) return;
		void(*invokeOnTriMeshRendererDirty)(Object*) = [](Object* selfPtr) {
			TriMeshRenderer* self = dynamic_cast<TriMeshRenderer*>(selfPtr);
			self->m_dirty = false;
			self->OnTriMeshRendererDirty();
		};
		Context()->ExecuteAfterUpdate(Callback<Object*>(invokeOnTriMeshRendererDirty), this);
		m_dirty = true;
	}

	void TriMeshRenderer::RecreateOnParentChanged(ParentChangeInfo) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		ScheduleOnTriMeshRendererDirtyCall();
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

	template<> void TypeIdDetails::GetTypeAttributesOf<TriMeshRenderer>(const Callback<const Object*>& report) {}
}
