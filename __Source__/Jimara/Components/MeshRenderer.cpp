#include "MeshRenderer.h"
#include "Transform.h"

namespace Jimara {
	namespace {
		struct InstancedBatchDesc {
			Reference<SceneContext> context;
			Reference<const TriMesh> mesh;
			Reference<const Material> material;

			inline InstancedBatchDesc(SceneContext* ctx = nullptr, const TriMesh* geometry = nullptr, const Material* mat = nullptr)
				: context(ctx), mesh(geometry), material(mat) {}

			inline bool operator<(const InstancedBatchDesc& desc)const {
				if (context < desc.context) return true;
				else if (context > desc.context) return false;
				else if (mesh < desc.mesh) return true;
				else if (mesh > desc.mesh) return false;
				else return material < desc.material;
			}

			inline bool operator==(const InstancedBatchDesc& desc)const {
				return (context == desc.context) && (mesh == desc.mesh) && (material == desc.material);
			}
		};
	}
}

namespace std {
	template<>
	struct hash<Jimara::InstancedBatchDesc> {
		size_t operator()(const Jimara::InstancedBatchDesc& desc)const {
			size_t ctxHash = std::hash<Jimara::SceneContext*>()(desc.context);
			size_t meshHash = std::hash<const Jimara::TriMesh*>()(desc.mesh);
			size_t matHash = std::hash<const Jimara::Material*>()(desc.material);
			auto combine = [](size_t a, size_t b) {
				return a ^ (b + 0x9e3779b9 + (a << 6) + (a >> 2));
			};
			return combine(ctxHash, combine(meshHash, matHash));
		}
	};
}

namespace Jimara {
	namespace {
#pragma warning(disable: 4250)
		class MeshRenderPipelineDescriptor : public virtual ObjectCache<InstancedBatchDesc>::StoredObject, public virtual Graphics::GraphicsPipeline::Descriptor {
		private:
			const InstancedBatchDesc m_desc;

			std::mutex m_transformLock;
			std::unordered_map<const Transform*, size_t> m_transformIndices;
			std::vector<Reference<const Transform>> m_transforms;
			std::vector<Matrix4> m_transformBufferData;
			Graphics::ArrayBufferReference<Matrix4> m_instanceBuffer;

		public:
			inline MeshRenderPipelineDescriptor(const InstancedBatchDesc& desc) : m_desc(desc) {
				// __TODO__: Implement this crap!
			}

			virtual ~MeshRenderPipelineDescriptor() {
				// __TODO__: Implement this crap!
			}

			/** PipelineDescriptor: */

			inline virtual size_t BindingSetCount()const override {
				// __TODO__: Implement this crap!
				return 0;
			}

			inline virtual BindingSetDescriptor* BindingSet(size_t index)const override {
				// __TODO__: Implement this crap!
				return nullptr;
			}

			/** GraphicsPipeline::Descriptor: */

			inline virtual Reference<Graphics::Shader> VertexShader() override {
				// __TODO__: Implement this crap!
				return nullptr;
			}

			inline virtual Reference<Graphics::Shader> FragmentShader() override {
				// __TODO__: Implement this crap!
				return nullptr;
			}


			inline virtual size_t VertexBufferCount() override {
				// __TODO__: Implement this crap!
				return 0;
			}

			inline virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index) override {
				// __TODO__: Implement this crap!
				return nullptr;
			}


			inline virtual size_t InstanceBufferCount() override { return 1; }

			inline virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index) override {
				std::unique_lock<std::mutex> lock(m_transformLock);
				bool bufferDirty = (m_instanceBuffer == nullptr);
				size_t i = 0;
				if (bufferDirty) m_instanceBuffer = m_desc.context->GraphicsDevice()->CreateArrayBuffer<Matrix4>(m_transforms.size());
				else while (i < m_transforms.size()) {
					if (m_transforms[i]->WorldMatrix() != m_transformBufferData[i]) {
						bufferDirty = true;
						break;
					}
					else i++;
				}
				if (bufferDirty) {
					while (i < m_transforms.size()) {
						m_transformBufferData[i] = m_transforms[i]->WorldMatrix();
						i++;
					}
					memcpy(m_instanceBuffer.Map(), m_transformBufferData.data(), m_transforms.size() * sizeof(Matrix4));
					m_instanceBuffer->Unmap(true);
				}
				return m_instanceBuffer;
			}


			inline virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer() override {
				// __TODO__: Implement this crap!
				return nullptr;
			}

			inline virtual size_t IndexCount() override {
				// __TODO__: Implement this crap!
				return 0;
			}

			inline virtual size_t InstanceCount() override { return m_transforms.size(); }


			/** Writer */
			class Writer : public virtual Graphics::PipelineDescriptor::WriteLock {
			private:
				MeshRenderPipelineDescriptor* const m_desc;

			public:
				inline Writer(MeshRenderPipelineDescriptor* desc) : Graphics::PipelineDescriptor::WriteLock(desc), m_desc(desc) {}

				void AddTransform(const Transform* transform) {
					if (transform == nullptr) return;
					std::unique_lock<std::mutex> lock(m_desc->m_transformLock);
					if (m_desc->m_transformIndices.find(transform) != m_desc->m_transformIndices.end()) return;
					m_desc->m_transformIndices[transform] = m_desc->m_transforms.size();
					m_desc->m_transforms.push_back(transform);
					while (m_desc->m_transformBufferData.size() < m_desc->m_transforms.size())
						m_desc->m_transformBufferData.push_back(Matrix4(0.0f));
					m_desc->m_instanceBuffer = nullptr;
				}

				void RemoveTransform(const Transform* transform) {
					if (transform == nullptr) return;
					std::unique_lock<std::mutex> lock(m_desc->m_transformLock);
					std::unordered_map<const Transform*, size_t>::iterator it = m_desc->m_transformIndices.find(transform);
					if (it == m_desc->m_transformIndices.end()) return;
					const size_t lastIndex = m_desc->m_transforms.size() - 1;
					const size_t index = it->second;
					m_desc->m_transformIndices.erase(it);
					if (index < lastIndex) {
						const Transform* last = m_desc->m_transforms[lastIndex];
						m_desc->m_transforms[index] = last;
						m_desc->m_transformIndices[last] = index;
					}
					m_desc->m_transforms.pop_back();
					m_desc->m_instanceBuffer = nullptr;
				}
			};

			/** Instancer: */

			class Instancer : ObjectCache<InstancedBatchDesc> {
			private:
				struct Allocator {
					const InstancedBatchDesc* desc;
					inline Allocator(const InstancedBatchDesc& d) : desc(&d) {}
					inline Reference<MeshRenderPipelineDescriptor> operator()() { return Object::Instantiate<MeshRenderPipelineDescriptor>(*desc); }
				};

				inline static Instancer& Instance() {
					static Instancer instance;
					return instance;
				}

			public:
				inline static Reference<MeshRenderPipelineDescriptor> GetDescriptor(const InstancedBatchDesc& desc) {
					Allocator allocator(desc);
					return Instance().GetCachedOrCreate(desc, false, allocator);
				}
			};
		};
#pragma warning(default: 4250)
	}

	MeshRenderer::MeshRenderer(Component* parent, const std::string& name, const TriMesh* mesh, const Jimara::Material* material, bool instanced)
		: Component(parent, name), m_mesh(mesh), m_material(material), m_instanced(instanced), m_alive(true), m_descriptorTransform(nullptr) {
		RecreatePipelineDescriptor();
		OnParentChanged() += Callback(&MeshRenderer::RecreateOnParentChanged, this);
		OnDestroyed() += Callback(&MeshRenderer::RecreateWhenDestroyed, this);
	}

	MeshRenderer::~MeshRenderer() {
		OnParentChanged() -= Callback(&MeshRenderer::RecreateOnParentChanged, this);
		OnDestroyed() -= Callback(&MeshRenderer::RecreateWhenDestroyed, this);
		m_alive = false;
		RecreatePipelineDescriptor();
	}

	const TriMesh* MeshRenderer::Mesh()const { return m_mesh; }

	void MeshRenderer::SetMesh(const TriMesh* mesh) {
		if (mesh == m_mesh) return;
		m_mesh = mesh;
		RecreatePipelineDescriptor();
	}

	const Jimara::Material* MeshRenderer::Material()const { return m_material; }

	void MeshRenderer::SetMaterial(const Jimara::Material* material) {
		if (material == m_material) return;
		m_material = material;
		RecreatePipelineDescriptor();
	}

	bool MeshRenderer::IsInstanced()const { return m_instanced; }

	void MeshRenderer::RenderInstanced(bool instanced) {
		if (instanced == m_instanced) return;
		m_instanced = instanced;
		RecreatePipelineDescriptor();
	}


	void MeshRenderer::RecreatePipelineDescriptor() {
		if (m_pipelineDescriptor != nullptr) {
			MeshRenderPipelineDescriptor* descriptor = dynamic_cast<MeshRenderPipelineDescriptor*>(m_pipelineDescriptor.operator->());
			{
				MeshRenderPipelineDescriptor::Writer writer(descriptor);
				writer.RemoveTransform(m_descriptorTransform);
			}
			m_pipelineDescriptor = nullptr;
			m_descriptorTransform = nullptr;
		}
		if (m_alive && m_mesh != nullptr && m_material != nullptr) {
			m_descriptorTransform = Component::Transfrom();
			if (m_descriptorTransform == nullptr) return;
			const InstancedBatchDesc desc(Context(), m_mesh, m_material);
			Reference<MeshRenderPipelineDescriptor> descriptor;
			if (m_instanced) descriptor = MeshRenderPipelineDescriptor::Instancer::GetDescriptor(desc);
			else descriptor = Object::Instantiate<MeshRenderPipelineDescriptor>(desc);
			{
				MeshRenderPipelineDescriptor::Writer writer(descriptor);
				writer.AddTransform(m_descriptorTransform);
			}
			m_pipelineDescriptor = descriptor;
		}
	}

	void MeshRenderer::RecreateOnParentChanged(const Component*, Component*) {
		RecreatePipelineDescriptor();
	}

	void MeshRenderer::RecreateWhenDestroyed(const Component*) {
		m_alive = false;
		RecreatePipelineDescriptor();
	}
}
