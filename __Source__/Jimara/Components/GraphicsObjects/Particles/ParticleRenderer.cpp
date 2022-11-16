#include "ParticleRenderer.h"
#include "../../../Math/Random.h"
#include "../../../Graphics/Data/GraphicsMesh.h"
#include "../../../Data/Serialization/Helpers/SerializerMacros.h"
#include "../../../Environment/Rendering/SceneObjects/GraphicsObjectDescriptor.h"
#include "../../../Environment/Rendering/Particles/ParticleState.h"


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
				m_dirty = false;
				m_graphicsMesh->GetBuffers(m_vertices, m_indices);
			}

			inline MeshBuffers(const TriMeshRenderer::Configuration& desc)
				: m_graphicsMesh(Graphics::GraphicsMesh::Cached(desc.context->Graphics()->Device(), desc.mesh, desc.geometryType))
				, m_dirty(true) {
				m_graphicsMesh->GetBuffers(m_vertices, m_indices);
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

		class TransformBuffers : public virtual Graphics::InstanceBuffer {
		private:
			size_t m_instanceCount = 0u;
			Graphics::ArrayBufferReference<Matrix4> m_instanceBuffer;
			const ParticleRenderer* const* m_renderers = nullptr;
			size_t m_rendererCount = 0u;

		public:
			inline void SetRenderers(const ParticleRenderer* const* renderers, size_t rendererCount) {
				m_renderers = renderers;
				m_rendererCount = rendererCount;
			}

			inline void Update() {
				m_instanceCount = 0u;
				for (size_t i = 0; i < m_rendererCount; i++)
					m_instanceCount += m_renderers[i]->ParticleBudget();

				if (m_instanceBuffer == nullptr || m_instanceBuffer->ObjectCount() < m_instanceCount) {
					m_instanceBuffer = m_renderers[0]->Context()->Graphics()->Device()->CreateArrayBuffer<Matrix4>(m_instanceCount);
					if (m_instanceBuffer == nullptr) {
						m_renderers[0]->Context()->Log()->Fatal(
							"ParticleRenderer::Helpers::Update Failed to allocate instance transform buffer! [File: '", __FILE__, "'; Line: ", __LINE__);
						m_instanceCount = 0u;
						return;
					}

					// __TODO__: Remove this; this is temporary....
					Matrix4* ptr = m_instanceBuffer.Map();
					for (size_t i = 0; i < m_rendererCount; i++) {
						const size_t particleBudget = m_renderers[i]->ParticleBudget();
						const Transform* transform = m_renderers[i]->GetTransfrom();
						const Matrix4 mat = (transform == nullptr) ? Math::Identity() : transform->WorldMatrix();
						for (size_t j = 0; j < particleBudget; j++) {
							(*ptr) = mat;
							(*ptr)[3] += Vector4(Random::PointOnSphere(), 0.0f);
							ptr++;
						}
					}
					m_instanceBuffer->Unmap(true);
				}

				// __TODO__: Implement this crap!
			}

			inline virtual size_t AttributeCount()const override { return 1; }

			inline virtual Graphics::InstanceBuffer::AttributeInfo Attribute(size_t)const {
				return { Graphics::InstanceBuffer::AttributeInfo::Type::MAT_4X4, 3, 0 };
			}

			inline virtual size_t BufferElemSize()const override { return sizeof(Matrix4); }

			inline virtual Reference<Graphics::ArrayBuffer> Buffer() override { return m_instanceBuffer; }

			inline size_t InstanceCount()const { return m_instanceCount; }
		};


#pragma warning(disable: 4250)
		class PipelineDescriptor 
			: public virtual GraphicsObjectDescriptor
			, public virtual ObjectCache<TriMeshRenderer::Configuration>::StoredObject
			, public virtual JobSystem::Job {
		private:
			const TriMeshRenderer::Configuration m_desc;
			const Reference<GraphicsObjectDescriptor::Set> m_graphicsObjectSet;
			const bool m_isInstanced;
			Material::CachedInstance m_cachedMaterialInstance;
			const Reference<MeshBuffers> m_meshBuffers;
			const Reference<TransformBuffers> m_transformBuffers;
			
			std::mutex m_lock;
			Reference<GraphicsObjectDescriptor::Set::ItemOwner> m_owner;
			std::unordered_set<ParticleRenderer*> m_renderers;
			std::vector<ParticleRenderer*> m_rendererList;

		public:
			inline PipelineDescriptor(const TriMeshRenderer::Configuration& desc, bool isInstanced)
				: GraphicsObjectDescriptor(desc.material->Shader(), desc.layer, desc.geometryType)
				, m_desc(desc), m_isInstanced(isInstanced)
				, m_graphicsObjectSet(GraphicsObjectDescriptor::Set::GetInstance(desc.context))
				, m_cachedMaterialInstance(desc.material)
				, m_meshBuffers(Object::Instantiate<MeshBuffers>(desc))
				, m_transformBuffers(Object::Instantiate<TransformBuffers>()) {}

			inline virtual ~PipelineDescriptor() {}

			inline const TriMeshRenderer::Configuration& Descriptor()const { return m_desc; }
			inline const bool IsInstanced()const { return m_isInstanced; }

			inline void AddRenderer(ParticleRenderer* renderer) {
				if (renderer == nullptr) return;
				std::unique_lock<std::mutex> lock(m_lock);
				if (m_renderers.find(renderer) != m_renderers.end()) return;
				m_renderers.insert(renderer);
				m_rendererList.clear();
				if (m_renderers.size() == 1u) {
					if (m_owner != nullptr)
						m_desc.context->Log()->Fatal(
							"ParticleRenderer::Helpers::PipelineDescriptor::AddRenderer - m_owner expected to be nullptr! [File: '", __FILE__, "'; Line: ", __LINE__);
					m_owner = Object::Instantiate<GraphicsObjectDescriptor::Set::ItemOwner>(this);
					m_graphicsObjectSet->Add(m_owner);
					m_desc.context->Graphics()->SynchPointJobs().Add(this);
				}
			}

			inline void RemoveRenderer(ParticleRenderer* renderer) {
				if (renderer == nullptr) return;
				decltype(m_renderers)::iterator it = m_renderers.find(renderer);
				if (it == m_renderers.end()) return;
				m_renderers.erase(it);
				m_rendererList.clear();
				if (m_renderers.size() <= 0u) {
					if (m_owner == nullptr)
						m_desc.context->Log()->Fatal(
							"ParticleRenderer::Helpers::PipelineDescriptor::RemoveRenderer - m_owner expected to be non-nullptr! [File: '", __FILE__, "'; Line: ", __LINE__);
					m_desc.context->Graphics()->SynchPointJobs().Remove(this);
					m_graphicsObjectSet->Remove(m_owner);
					m_owner = nullptr;
				}
			}

#pragma region __ShaderResourceBindingSet__
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
#pragma endregion


#pragma region __GraphicsObjectDescriptor__
			/** GraphicsObjectDescriptor */

			inline virtual AABB Bounds()const override {
				// __TODO__: Implement this crap!
				return AABB();
			}

			inline virtual size_t VertexBufferCount()const override { return 1; }
			inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t)const override { return m_meshBuffers; }

			inline virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer()const override { return m_meshBuffers->IndexBuffer(); }
			inline virtual size_t IndexCount()const override { return m_meshBuffers->IndexBuffer()->ObjectCount(); }

			inline virtual size_t InstanceBufferCount()const override { return 1; }
			inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index)const override { return m_transformBuffers; }
			inline virtual size_t InstanceCount()const override { return m_transformBuffers->InstanceCount(); }

			inline virtual Reference<Component> GetComponent(size_t instanceId, size_t)const override {
				/* __TODO__: Implement this crap! */
				return nullptr;
			}
#pragma endregion


		protected:
#pragma region __JobSystem_Job__
			virtual void Execute()final override {
				std::unique_lock<std::mutex> lock(m_lock);
				if (m_renderers.size() != m_rendererList.size()) {
					m_rendererList.clear();
					for (auto it = m_renderers.begin(); it != m_renderers.end(); ++it)
						m_rendererList.push_back(*it);
					m_transformBuffers->SetRenderers(m_rendererList.data(), m_rendererList.size());
				}

				m_cachedMaterialInstance.Update();
				m_meshBuffers->Update();
				m_transformBuffers->Update();
			}
			virtual void CollectDependencies(Callback<Job*>)override {}
#pragma endregion
		};
#pragma warning(default: 4250)

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
		if (Destroyed()) 
			budget = 0u;
		if (budget == ParticleBudget()) return;
		{
			m_buffers = nullptr;
			m_particleStateBuffer = nullptr;
			// __TODO__: Destroy all underlying tasks (maybe keep the old particles somehow)!
		}
		if (budget > 0u) {
			m_buffers = Object::Instantiate<ParticleBuffers>(Context(), budget);
			m_particleStateBuffer = m_buffers->GetBuffer(ParticleState::BufferId());
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
		SetParticleBudget(ParticleBudget());
		const bool activeInHierarchy = ActiveInHeirarchy();
		const TriMeshRenderer::Configuration desc(this);
		{
			Helpers::PipelineDescriptor* currentPipelineDescriptor = dynamic_cast<Helpers::PipelineDescriptor*>(m_pipelineDescriptor.operator->());
			if (currentPipelineDescriptor != nullptr && (
				(!activeInHierarchy) || 
				(currentPipelineDescriptor->IsInstanced() != IsInstanced()) || 
				(currentPipelineDescriptor->Descriptor() != desc))) {
				currentPipelineDescriptor->RemoveRenderer(this);
				m_pipelineDescriptor = nullptr;
			}
		}
		if (activeInHierarchy && m_pipelineDescriptor == nullptr && Mesh() != nullptr && MaterialInstance() != nullptr) {
			Reference<Helpers::PipelineDescriptor> descriptor;
			if (IsInstanced()) descriptor = Helpers::PipelineDescriptorInstancer::GetDescriptor(desc);
			else descriptor = Object::Instantiate<Helpers::PipelineDescriptor>(desc, false);
			descriptor->AddRenderer(this);
			m_pipelineDescriptor = descriptor;
		}
	}

	template<> void TypeIdDetails::GetTypeAttributesOf<ParticleRenderer>(const Callback<const Object*>& report) {
		static const ComponentSerializer::Of<ParticleRenderer> serializer("Jimara/Graphics/Particles/ParticleRenderer", "Particle Renderer");
		report(&serializer);
	}
}
