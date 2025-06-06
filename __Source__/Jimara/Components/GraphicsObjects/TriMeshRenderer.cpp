#include "TriMeshRenderer.h"
#include "../../Data/Materials/SampleDiffuse/SampleDiffuseShader.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"
#include "../../Data/Serialization/Helpers/SerializerMacros.h"


namespace Jimara {
	TriMeshRenderer::TriMeshRenderer() {
		m_flags = Flags::INSTANCED | Flags::CAST_SHADOWS;
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
			m_materialInstance = SampleDiffuseShader::MaterialInstance(Context());
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

	Layer TriMeshRenderer::Layer()const { return m_layer; }

	void TriMeshRenderer::SetLayer(Jimara::Layer layer) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		if (layer == m_layer) return;
		m_layer = layer;
		ScheduleOnTriMeshRendererDirtyCall();
	}

	TriMeshRenderer::Flags TriMeshRenderer::RendererFlags()const { return m_flags; }

	void TriMeshRenderer::SetRendererFlags(Flags flags) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		if (m_flags == flags)
			return;
		m_flags = flags;
		ScheduleOnTriMeshRendererDirtyCall();
	}

	bool TriMeshRenderer::HasRendererFlags(Flags flags)const { return (m_flags & flags) == flags; }

	void TriMeshRenderer::SetRendererFlags(Flags flags, bool value) {
		std::unique_lock<std::recursive_mutex> lock(Context()->UpdateLock());
		Flags resultFlags = value
			? (m_flags | flags)
			: (m_flags & (~(flags)));
		if (m_flags == resultFlags)
			return;
		m_flags = resultFlags;
		ScheduleOnTriMeshRendererDirtyCall();
	}

	bool TriMeshRenderer::IsInstanced()const { 
		return HasRendererFlags(Flags::INSTANCED); 
	}

	void TriMeshRenderer::RenderInstanced(bool instanced) {
		SetRendererFlags(Flags::INSTANCED, instanced);
	}

	bool TriMeshRenderer::IsStatic()const { 
		return HasRendererFlags(Flags::STATIC); 
	}

	void TriMeshRenderer::MarkStatic(bool isStatic) {
		SetRendererFlags(Flags::STATIC, isStatic);
	}

	bool TriMeshRenderer::CastsShadows()const {
		return HasRendererFlags(Flags::CAST_SHADOWS);
	}

	void TriMeshRenderer::CastShadows(bool castShadows) {
		SetRendererFlags(Flags::CAST_SHADOWS, castShadows);
	}

	const Object* TriMeshRenderer::GeometryTypeEnumerationAttribute() {
		static const Reference<Object> attribute =
			Object::Instantiate<Serialization::EnumAttribute<std::underlying_type_t<Graphics::GraphicsPipeline::IndexType>>>(false,
				"TRIANGLE", Graphics::GraphicsPipeline::IndexType::TRIANGLE,
				"EDGE", Graphics::GraphicsPipeline::IndexType::EDGE);
		return attribute;
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
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(Mesh, SetMesh, "Mesh", "Mesh to render");
			JIMARA_SERIALIZE_FIELD_GET_SET(Material, SetMaterial, "Material", "Material to render with");
			JIMARA_SERIALIZE_FIELD_GET_SET(Layer, SetLayer, "Layer", "Graphics object layer (for renderer filtering)", Layers::LayerAttribute::Instance());
			JIMARA_SERIALIZE_FIELD_GET_SET(IsInstanced, RenderInstanced, "Instanced", "Set to true, if the mesh is supposed to be instanced");
			JIMARA_SERIALIZE_FIELD_GET_SET(IsStatic, MarkStatic, "Static", "If true, the renderer assumes the mesh transform stays constant and saves some CPU cycles doing that");
			JIMARA_SERIALIZE_FIELD_GET_SET(CastsShadows, CastShadows, "Cast Shadows", "If set, the renderer will cast shadows");
			JIMARA_SERIALIZE_FIELD_GET_SET(GeometryType, SetGeometryType,
				"Geometry Type", "Tells, how the mesh is supposed to be rendered (TRIANGLE/EDGE)", GeometryTypeEnumerationAttribute());
		};
	}

	void TriMeshRenderer::GetSerializedActions(Callback<Serialization::SerializedCallback> report) {
		Component::GetSerializedActions(report);

		// Mesh:
		{
			static const auto serializer = Serialization::DefaultSerializer<Reference<TriMesh>>::Create("Mesh", "Mesh to render");
			report(Serialization::SerializedCallback::Create<TriMesh*>::From("SetMesh", Callback<TriMesh*>(&TriMeshRenderer::SetMesh, this), serializer));
		}

		// Material:
		{
			static const auto serializer = Serialization::DefaultSerializer<Reference<Jimara::Material>>::Create(
				"Material", "Material to render the mesh with");
			report(Serialization::SerializedCallback::Create<Jimara::Material*>::From(
				"SetMaterial", Callback<Jimara::Material*>(&TriMeshRenderer::SetMaterial, this), serializer));
		}

		// Layer:
		{
			static const auto serializer = Serialization::DefaultSerializer<Jimara::Layer>::Create("Layer", "Graphics object layer (for renderer filtering)");
			report(Serialization::SerializedCallback::Create<Jimara::Layer>::From("SetLayer", Callback<Jimara::Layer>(&TriMeshRenderer::SetLayer, this), serializer));
		}

		// Instancing flag:
		{
			static const auto serializer = Serialization::DefaultSerializer<bool>::Create("Instanced", "Set to true, if the mesh is supposed to be instanced");
			report(Serialization::SerializedCallback::Create<bool>::From("RenderInstanced", Callback<bool>(&TriMeshRenderer::RenderInstanced, this), serializer));
		}

		// Static flag:
		{
			static const auto serializer = Serialization::DefaultSerializer<bool>
				::Create("Static", "If true, the renderer assumes the mesh transform stays constant and saves some CPU cycles doing that");
			report(Serialization::SerializedCallback::Create<bool>::From("MarkStatic", Callback<bool>(&TriMeshRenderer::MarkStatic, this), serializer));
		}

		// Shadow-casting flag:
		{
			static const auto serializer = Serialization::DefaultSerializer<bool>::Create("Cast Shadows", "If set, the renderer will cast shadows");
			report(Serialization::SerializedCallback::Create<bool>::From("CastShadows", Callback<bool>(&TriMeshRenderer::CastShadows, this), serializer));
		}

		// Set geometry type:
		{
			static const auto serializer = Serialization::DefaultSerializer<std::underlying_type_t<Graphics::GraphicsPipeline::IndexType>>::Create(
				"Geometry Type", "Tells, how the mesh is supposed to be rendered (TRIANGLE/EDGE)",
				std::vector<Reference<const Object>>{GeometryTypeEnumerationAttribute()});
			report(Serialization::SerializedCallback::Create<Graphics::GraphicsPipeline::IndexType>::From(
				"SetGeometryType", Callback<Graphics::GraphicsPipeline::IndexType>(&TriMeshRenderer::SetGeometryType, this), serializer));
		}
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
		if (m_dirty) return;
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

	TriMeshRenderer::Configuration::Configuration(TriMeshRenderer* renderer) {
		if (renderer == nullptr) return;
		context = renderer->Context();
		mesh = renderer->Mesh();
		material = renderer->MaterialInstance();
		layer = renderer->Layer();
		flags = renderer->RendererFlags();
		geometryType = renderer->GeometryType();
	}

	bool TriMeshRenderer::Configuration::operator<(const Configuration& configuration)const {
		if (context < configuration.context) return true;
		else if (context > configuration.context) return false;
		else if (mesh < configuration.mesh) return true;
		else if (mesh > configuration.mesh) return false;
		else if (material < configuration.material) return true;
		else if (material > configuration.material) return false;
		else if (layer < configuration.layer) return true;
		else if (layer > configuration.layer) return false;
		else if (flags < configuration.flags) return true;
		else if (flags > configuration.flags) return false;
		else return geometryType < configuration.geometryType;
	}

	bool TriMeshRenderer::Configuration::operator==(const Configuration& configuration)const {
		return (context == configuration.context)
			&& (mesh == configuration.mesh)
			&& (material == configuration.material)
			&& (layer == configuration.layer)
			&& (flags == configuration.flags)
			&& (geometryType == configuration.geometryType);
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<TriMeshRenderer>(const Callback<const Object*>& report) {}
}

namespace std {
	size_t hash<Jimara::TriMeshRenderer::Configuration>::operator()(const Jimara::TriMeshRenderer::Configuration& configuration)const {
		size_t ctxHash = std::hash<Jimara::SceneContext*>()(configuration.context);
		size_t meshHash = std::hash<const Jimara::TriMesh*>()(configuration.mesh);
		size_t matHash = std::hash<const Jimara::Material::Instance*>()(configuration.material);
		size_t layerHash = std::hash<Jimara::Layer>()(configuration.layer);
		size_t staticHash = std::hash<std::underlying_type_t<Jimara::TriMeshRenderer::Flags>>()(
			static_cast<std::underlying_type_t<Jimara::TriMeshRenderer::Flags>>(configuration.flags));
		size_t geometryTypeHash = std::hash<uint8_t>()(static_cast<uint8_t>(configuration.geometryType));
		return Jimara::MergeHashes(
			Jimara::MergeHashes(ctxHash, Jimara::MergeHashes(meshHash, matHash)),
			Jimara::MergeHashes(layerHash, Jimara::MergeHashes(staticHash, geometryTypeHash)));
	}
}
