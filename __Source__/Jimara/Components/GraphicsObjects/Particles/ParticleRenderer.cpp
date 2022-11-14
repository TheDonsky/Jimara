#include "ParticleRenderer.h"
#include "../../../Graphics/Data/GraphicsMesh.h"
#include "../../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../../Environment/Rendering/SceneObjects/GraphicsObjectDescriptor.h"


namespace Jimara {
	struct ParticleRenderer::Helpers {
		class MeshBuffers : public virtual Graphics::VertexBuffer {
		private:
			const Reference<Graphics::GraphicsMesh> m_graphicsMesh;
			Graphics::ArrayBufferReference<MeshVertex> m_vertices;
			Graphics::ArrayBufferReference<uint32_t> m_indices;
			std::atomic<bool> m_dirty;

			inline void OnMeshDirty(Graphics::GraphicsMesh*) { m_dirty = true; }

		public:
			inline void Update() {
				if (!m_dirty) return;
				m_graphicsMesh->GetBuffers(m_vertices, m_indices);
				m_dirty = false;
			}

			inline MeshBuffers(const TriMeshRenderer::Configuration& desc)
				: m_graphicsMesh(Graphics::GraphicsMesh::Cached(desc.context->Graphics()->Device(), desc.mesh, desc.geometryType))
				, m_dirty(true) {
				m_graphicsMesh->OnInvalidate() += Callback<Graphics::GraphicsMesh*>(&MeshBuffers::OnMeshDirty, this);
				Update();
			}

			inline virtual ~MeshBuffers() {
				m_graphicsMesh->OnInvalidate() -= Callback<Graphics::GraphicsMesh*>(&MeshBuffers::OnMeshDirty, this);
			}

			inline virtual size_t AttributeCount()const override { return 3; }

			inline virtual AttributeInfo Attribute(size_t index)const override {
				static const AttributeInfo INFOS[] = {
					{ AttributeInfo::Type::FLOAT3, 0, offsetof(MeshVertex, position) },
					{ AttributeInfo::Type::FLOAT3, 1, offsetof(MeshVertex, normal) },
					{ AttributeInfo::Type::FLOAT2, 2, offsetof(MeshVertex, uv) },
				};
				return INFOS[index];
			}

			inline virtual size_t BufferElemSize()const override { return sizeof(MeshVertex); }

			inline virtual Reference<Graphics::ArrayBuffer> Buffer() override { return m_vertices; }

			inline Graphics::ArrayBufferReference<uint32_t> IndexBuffer()const { return m_indices; }
		};

		class PipelineDescriptor 
			: public virtual GraphicsObjectDescriptor
			, public virtual ObjectCache<TriMeshRenderer::Configuration>::StoredObject {
		private:
			const TriMeshRenderer::Configuration m_desc;
			const Reference<GraphicsObjectDescriptor::Set> m_graphicsObjectSet;
			const bool m_isInstanced;
			Material::CachedInstance m_cachedMaterialInstance;
			const Reference<MeshBuffers> m_meshBuffers;

		public:
			inline PipelineDescriptor(const TriMeshRenderer::Configuration& desc, bool isInstanced)
				: GraphicsObjectDescriptor(desc.material->Shader(), desc.layer, desc.geometryType)
				, m_desc(desc), m_isInstanced(isInstanced)
				, m_graphicsObjectSet(GraphicsObjectDescriptor::Set::GetInstance(desc.context))
				, m_cachedMaterialInstance(desc.material)
				, m_meshBuffers(Object::Instantiate<MeshBuffers>(desc)) {}

			inline virtual ~PipelineDescriptor() {}

			inline const TriMeshRenderer::Configuration& Descriptor()const { return m_desc; }
			inline const bool IsInstanced()const { return m_isInstanced; }


			/** ShaderResourceBindingSet: */

			inline virtual Reference<const Graphics::ShaderResourceBindings::ConstantBufferBinding> FindConstantBufferBinding(const std::string& name)const override {
				// Note: Maybe index would make this bit faster, but binding count is expected to be low and this function is invoked only once per resource per pipeline creation...
				for (size_t i = 0; i < m_cachedMaterialInstance.ConstantBufferCount(); i++)
					if (m_cachedMaterialInstance.ConstantBufferName(i) == name) return m_cachedMaterialInstance.ConstantBuffer(i);
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::StructuredBufferBinding> FindStructuredBufferBinding(const std::string& name)const override {
				// Note: Maybe index would make this bit faster, but binding count is expected to be low and this function is invoked only once per resource per pipeline creation...
				for (size_t i = 0; i < m_cachedMaterialInstance.StructuredBufferCount(); i++)
					if (m_cachedMaterialInstance.StructuredBufferName(i) == name) return m_cachedMaterialInstance.StructuredBuffer(i);
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureSamplerBinding> FindTextureSamplerBinding(const std::string& name)const override {
				// Note: Maybe index would make this bit faster, but binding count is expected to be low and this function is invoked only once per resource per pipeline creation...
				for (size_t i = 0; i < m_cachedMaterialInstance.TextureSamplerCount(); i++)
					if (m_cachedMaterialInstance.TextureSamplerName(i) == name) return m_cachedMaterialInstance.TextureSampler(i);
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::TextureViewBinding> FindTextureViewBinding(const std::string&)const override {
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessStructuredBufferSetBinding> FindBindlessStructuredBufferSetBinding(const std::string&)const override {
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureSamplerSetBinding> FindBindlessTextureSamplerSetBinding(const std::string&)const override {
				return nullptr;
			}

			inline virtual Reference<const Graphics::ShaderResourceBindings::BindlessTextureViewSetBinding> FindBindlessTextureViewSetBinding(const std::string&)const override {
				return nullptr;
			}


			/** GraphicsObjectDescriptor */

			inline virtual AABB Bounds()const override {
				// __TODO__: Implement this crap!
				return AABB();
			}

			inline virtual size_t VertexBufferCount()const override { return 1; }
			inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t)const override { return m_meshBuffers; }

			inline virtual size_t InstanceBufferCount()const override { return 1; }
			inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index)const override { return nullptr; /* __TODO__: Implement this crap! */ }

			inline virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer()const override { return m_meshBuffers->IndexBuffer(); }

			inline virtual size_t IndexCount()const override { return m_meshBuffers->IndexBuffer()->ObjectCount(); }

			inline virtual size_t InstanceCount()const override { return 0u; /* __TODO__: Implement this crap! */ }

			inline virtual Reference<Component> GetComponent(size_t instanceId, size_t)const override {
				/* __TODO__: Implement this crap! */
				return nullptr;
			}
		};

		class PipelineDescriptorInstancer : public virtual ObjectCache<TriMeshRenderer::Configuration> {
		public:
			inline static Reference<PipelineDescriptor> GetDescriptor(const TriMeshRenderer::Configuration& desc) {
				static PipelineDescriptorInstancer instance;
				return instance.GetCachedOrCreate(desc, false,
					[&]() -> Reference<PipelineDescriptor> { return Object::Instantiate<PipelineDescriptor>(desc, true); });
			}
		};
	};

	ParticleRenderer::ParticleRenderer(Component* parent, const std::string_view& name, size_t particleBudget)
		: Component(parent, name) {
		// __TODO__: Implement this crap!
		SetParticleBudget(particleBudget);
	}

	ParticleRenderer::~ParticleRenderer() {
		SetParticleBudget(0u);
		// __TODO__: Implement this crap!
	}

	size_t ParticleRenderer::ParticleBudget()const { return (m_buffers == nullptr) ? 0u : m_buffers->ParticleBudget(); }

	void ParticleRenderer::SetParticleBudget(size_t budget) {
		if (budget == ParticleBudget()) return;
		{
			m_buffers = nullptr;
			// __TODO__: Destroy all underlying tasks (maybe keep the old particles somehow)!
		}
		if (budget > 0u) {
			m_buffers = new ParticleBuffers(Context(), budget);
			// __TODO__: Create tasks!
		}
	}

	void ParticleRenderer::GetFields(Callback<Serialization::SerializedObject> recordElement) {
		TriMeshRenderer::GetFields(recordElement);
		JIMARA_SERIALIZE_FIELDS(this, recordElement) {
			JIMARA_SERIALIZE_FIELD_GET_SET(ParticleBudget, SetParticleBudget, "Particle Budget", "Maximal number of particles within the system");

			// __TODO__: Implement this crap!
		};
	}

	void ParticleRenderer::OnTriMeshRendererDirty() {
		const bool activeInHierarchy = ActiveInHeirarchy();
		const TriMeshRenderer::Configuration desc(this);
		{
			Helpers::PipelineDescriptor* currentPipelineDescriptor = dynamic_cast<Helpers::PipelineDescriptor*>(m_pipelineDescriptor.operator->());
			if (currentPipelineDescriptor != nullptr && (
				(!activeInHierarchy) || 
				(currentPipelineDescriptor->IsInstanced() != IsInstanced()) || 
				(currentPipelineDescriptor->Descriptor() != desc))) {
				{
					// __TODO__: Implement this crap!
					//Helpers::PipelineDescriptor::Writer writer(descriptor);
					//writer.RemoveComponent(this);
				}
				m_pipelineDescriptor = nullptr;
			}
		}
		if (activeInHierarchy && m_pipelineDescriptor == nullptr && Mesh() != nullptr && MaterialInstance() != nullptr) {
			Reference<Helpers::PipelineDescriptor> descriptor;
			if (IsInstanced()) descriptor = Helpers::PipelineDescriptorInstancer::GetDescriptor(desc);
			else descriptor = Object::Instantiate<Helpers::PipelineDescriptor>(desc, false);
			{
				// __TODO__: Implement this crap!
				//Helpers::PipelineDescriptor::Writer writer(descriptor);
				//writer.AddComponent(this);
			}
			m_pipelineDescriptor = descriptor;
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleRenderer>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<ParticleRenderer> serializer("Jimara/Graphics/Particles/ParticleRenderer", "Particle Renderer");
		report(&serializer);
	}
}
