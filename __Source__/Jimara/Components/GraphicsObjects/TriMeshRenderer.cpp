#include "TriMeshRenderer.h"
#include "../../Data/Materials/SampleDiffuse/SampleDiffuseShader.h"
#include "../../Data/Serialization/Attributes/EnumAttribute.h"


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

	namespace {
		class TriMeshRendererSerializer : public virtual Serialization::SerializerList::From<TriMeshRenderer> {
		public:
			inline TriMeshRendererSerializer() : ItemSerializer("Jimara/TriMeshRenderer", "Triangle Mesh Renderer") {}

			inline static const TriMeshRendererSerializer* Instance() {
				static const TriMeshRendererSerializer instance;
				return &instance;
			}

			virtual void GetFields(const Callback<Serialization::SerializedObject>& recordElement, TriMeshRenderer* target)const final override {
				TypeId::Of<Component>().FindAttributeOfType<ComponentSerializer>()->GetFields(recordElement, target);
				{
					static const Reference<const Serialization::ItemSerializer::Of<TriMeshRenderer>> serializer =
						Serialization::ValueSerializer<TriMesh*>::Create<TriMeshRenderer>(
							"Mesh", "Mesh to render",
							Function<TriMesh*, TriMeshRenderer*>([](TriMeshRenderer* renderer) -> TriMesh* { return renderer->Mesh(); }),
							Callback<TriMesh* const&, TriMeshRenderer*>([](TriMesh* const& value, TriMeshRenderer* renderer) { renderer->SetMesh(value); }));
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const Serialization::ItemSerializer::Of<TriMeshRenderer>> serializer =
						Serialization::ValueSerializer<Material*>::Create<TriMeshRenderer>(
							"Material", "Material to render with",
							Function<Material*, TriMeshRenderer*>([](TriMeshRenderer* renderer) -> Material* { return renderer->Material(); }),
							Callback<Material* const&, TriMeshRenderer*>([](Material* const& value, TriMeshRenderer* renderer) { renderer->SetMaterial(value); }));
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const Serialization::ItemSerializer::Of<TriMeshRenderer>> serializer =
						Serialization::ValueSerializer<GraphicsLayer>::For<TriMeshRenderer>(
							"Layer", "Graphics object layer (for renderer filtering)",
							[](TriMeshRenderer* renderer) -> GraphicsLayer { return renderer->Layer(); },
							[](GraphicsLayer const& value, TriMeshRenderer* renderer) { renderer->SetLayer(value); });
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const Serialization::ItemSerializer::Of<TriMeshRenderer>> serializer =
						Serialization::ValueSerializer<bool>::For<TriMeshRenderer>(
							"Instanced", "Set to true, if the mesh is supposed to be instanced",
							[](TriMeshRenderer* renderer) -> bool { return renderer->IsInstanced(); },
							[](bool const& value, TriMeshRenderer* renderer) { renderer->RenderInstanced(value); });
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const Serialization::ItemSerializer::Of<TriMeshRenderer>> serializer =
						Serialization::ValueSerializer<bool>::For<TriMeshRenderer>(
							"Static", "If true, the renderer assumes the mesh transform stays constant and saves some CPU cycles doing that",
							[](TriMeshRenderer* renderer) -> bool { return renderer->IsStatic(); },
							[](bool const& value, TriMeshRenderer* renderer) { renderer->MarkStatic(value); });
					recordElement(serializer->Serialize(target));
				}
				{
					static const Reference<const Serialization::ItemSerializer::Of<TriMeshRenderer>> serializer =
						Serialization::ValueSerializer<uint8_t>::For<TriMeshRenderer>(
							"Geometry Type", "Tells, how the mesh is supposed to be rendered (TRIANGLE/EDGE)",
							[](TriMeshRenderer* renderer) -> uint8_t { return static_cast<uint8_t>(renderer->GeometryType()); },
							[](uint8_t const& value, TriMeshRenderer* renderer) { renderer->SetGeometryType(static_cast<Graphics::GraphicsPipeline::IndexType>(value)); },
							{ Object::Instantiate<Serialization::EnumAttribute<uint8_t>>(std::vector<Serialization::EnumAttribute<uint8_t>::Choice>({
								Serialization::EnumAttribute<uint8_t>::Choice("TRIANGLE", static_cast<uint8_t>(Graphics::GraphicsPipeline::IndexType::TRIANGLE)),
								Serialization::EnumAttribute<uint8_t>::Choice("EDGE", static_cast<uint8_t>(Graphics::GraphicsPipeline::IndexType::EDGE))
								}), false)
							});
					recordElement(serializer->Serialize(target));
				}
			}
		};
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<TriMeshRenderer>(const Callback<const Object*>& report) {
		report(TriMeshRendererSerializer::Instance());
	}
}
