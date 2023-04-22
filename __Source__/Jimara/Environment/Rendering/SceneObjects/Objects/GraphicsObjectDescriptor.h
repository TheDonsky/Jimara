#pragma once
#include "../../../../Data/Material.h"
#include "../../../Scene/SceneObjectCollection.h"
#include "../../../Layers.h"
#include "../../ViewportDescriptor.h"


namespace Jimara {
	/// <summary>
	/// Simple descriptor of a graphics scene object
	/// </summary>
	class JIMARA_API GraphicsObjectDescriptor : public virtual Object {
	public:
		/// <summary> Per-viewport graphics object </summary>
		class ViewportData;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="layer"> Graphics layer for filtering (Because of some dependencies, this can not change, threfore we have it kind of hard coded here) </param>
		inline GraphicsObjectDescriptor(Jimara::Layer layer) : m_layer(layer) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~GraphicsObjectDescriptor() {}

		/// <summary> Graphics layer for filtering </summary>
		inline Jimara::Layer Layer()const { return m_layer; }

		/// <summary>
		/// Retrieves viewport-specific object descriptor
		/// <para/> If nullptr is returned, that means that the object should not be rendered for a specific viewport.
		/// </summary>
		/// <param name="viewport"> "Target viewport" (can be nullptr and that specific case means the "default" descriptor, whatever that means for each object type) </param>
		/// <returns> Per-viewport object descriptor </returns>
		virtual Reference<const ViewportData> GetViewportData(const ViewportDescriptor* viewport) = 0;

		/// <summary>
		/// SceneObjectCollection<GraphicsObjectDescriptor> will flush on Scene::GraphicsContext::OnGraphicsSynch
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Scene::GraphicsContext::OnGraphicsSynch </returns>
		inline static Event<>& OnFlushSceneObjectCollections(SceneContext* context) { return context->Graphics()->OnGraphicsSynch(); }

		/// <summary> Set of all GraphicsObjectDescriptors tied to a scene </summary>
		typedef SceneObjectCollection<GraphicsObjectDescriptor> Set;


	private:
		// Layer
		const Jimara::Layer m_layer;
	};


	/// <summary> Per-viewport graphics object </summary>
	class JIMARA_API GraphicsObjectDescriptor::ViewportData : public virtual Object, public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
	private:
		// Scene context
		const Reference<SceneContext> m_context;

		// Shader class (Because of some dependencies, this can not change, threfore we have it kind of hard coded here)
		const Reference<const Graphics::ShaderClass> m_shaderClass;

		// Type of the geometry primitives or index interpretation(TRIANGLE(filled; multiples of 3) or EDGE(wireframe; pairs of 2))
		const Graphics::GraphicsPipeline::IndexType m_geometryType;

		// Blending mode
		const Graphics::Experimental::GraphicsPipeline::BlendMode m_blendMode;

	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="shaderClass"> Scene context </param>
		/// <param name="shaderClass"> Shader class (Because of some dependencies, this can not change, threfore we have it kind of hard coded here) </param>
		/// <param name="geometryType"> Type of the geometry primitives or index interpretation (TRIANGLE(filled; multiples of 3) or EDGE(wireframe; pairs of 2)) </param>
		inline ViewportData(
			SceneContext* context,
			const Graphics::ShaderClass* shaderClass,
			Graphics::GraphicsPipeline::IndexType geometryType,
			Graphics::Experimental::GraphicsPipeline::BlendMode blendMode)
			: m_context(context), m_shaderClass(shaderClass), m_geometryType(geometryType), m_blendMode(blendMode) {}

		/// <summary> Scene context </summary>
		inline SceneContext* Context()const { return m_context; }

		/// <summary> Shader class to use for rendering </summary>
		inline const Graphics::ShaderClass* ShaderClass()const { return m_shaderClass; }

		/// <summary> Type of the geometry primitives or index interpretation (TRIANGLE(filled; multiples of 3) or EDGE(wireframe; pairs of 2)) </summary>
		inline Graphics::GraphicsPipeline::IndexType GeometryType()const { return m_geometryType; }

		/// <summary> Blending mode </summary>
		const Graphics::Experimental::GraphicsPipeline::BlendMode BlendMode()const { return m_blendMode; }

		/// <summary> Boundaries, covering the entire volume of the scene object (useful for culling and sorting) </summary>
		virtual AABB Bounds()const = 0;


		/// <summary> Number of vertex buffers, used by the vertex shader (tied to material; should not change) </summary>
		virtual size_t VertexBufferCount()const = 0;

		/// <summary>
		/// Vertex buffer by index
		/// </summary>
		/// <param name="index"> Vertex buffer index </param>
		/// <returns> Index'th vertex buffer </returns>
		virtual Reference<Graphics::VertexBuffer> VertexBuffer(size_t index)const = 0;


		/// <summary> 
		/// Number of instance buffers, used by the vertex shader 
		/// (basically, vertex buffers that are delivered per-instance, instead of per vertex. tied to material; should not change) 
		/// </summary>
		virtual size_t InstanceBufferCount()const = 0;

		/// <summary>
		/// Instance buffer by index
		/// </summary>
		/// <param name="index"> Instance buffer index </param>
		/// <returns> Index'th instance buffer </returns>
		virtual Reference<Graphics::InstanceBuffer> InstanceBuffer(size_t index)const = 0;


		/// <summary> Index buffer </summary>
		virtual Graphics::ArrayBufferReference<uint32_t> IndexBuffer()const = 0;

		/// <summary>
		/// Indirect draw buffer
		/// <para/> Notes:
		///		<para/> 0. If not null, indirect index draw command will be used;
		///		<para/> 1. If provided, InstanceCount will be understood as indirect draw command count.
		/// </summary>
		virtual Graphics::IndirectDrawBufferReference IndirectBuffer()const { return nullptr; }

		/// <summary> Number of indices to use from index buffer (helps when we want to reuse the index buffer object even when we change geometry or something) </summary>
		virtual size_t IndexCount()const = 0;

		/// <summary> Number of instances to draw (by ignoring some of the instance buffer members, we can mostly vary instance count without any reallocation) </summary>
		virtual size_t InstanceCount()const = 0;

		/// <summary>
		/// Drawing component reference by instanceId and primitiveId
		/// </summary>
		/// <param name="instanceId"> Index of the instance [0 - InstanceCount) </param>
		/// <param name="primitiveId"> Index of a primitive (triangle, for example; or whatever fragment shader sees as gl_PrimitiveID) </param>
		/// <returns> Component reference </returns>
		virtual Reference<Component> GetComponent(size_t instanceId, size_t primitiveId)const = 0;

		/// <summary> Generated BindingSearchFunctions from the ViewportData </summary>
		inline Graphics::BindingSet::BindingSearchFunctions BindingSearchFunctions()const {
			Graphics::BindingSet::BindingSearchFunctions functions = {};
			{
				static Reference<const Graphics::ResourceBinding<Graphics::Buffer>>
					(*findFn)(const ViewportData*, const Graphics::BindingSet::BindingDescriptor&) =
					[](const ViewportData* self, const Graphics::BindingSet::BindingDescriptor& desc) {
					const auto binding = self->FindConstantBufferBinding(desc.name); 
					if (binding != nullptr || self->ShaderClass() == nullptr || self->m_context == nullptr) return binding;
					else return self->ShaderClass()->DefaultConstantBufferBinding(desc.name, self->m_context->Graphics()->Device());
				};
				functions.constantBuffer = Graphics::BindingSet::BindingSearchFn<Graphics::Buffer>(findFn, this);
			}
			{
				static Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>>
					(*findFn)(const ViewportData*, const Graphics::BindingSet::BindingDescriptor&) =
					[](const ViewportData* self, const Graphics::BindingSet::BindingDescriptor& desc) { 
					const auto binding = self->FindStructuredBufferBinding(desc.name);
					if (binding != nullptr || self->ShaderClass() == nullptr || self->m_context == nullptr) return binding;
					else return self->ShaderClass()->DefaultStructuredBufferBinding(desc.name, self->m_context->Graphics()->Device());
				};
				functions.structuredBuffer = Graphics::BindingSet::BindingSearchFn<Graphics::ArrayBuffer>(findFn, this);
			}
			{
				static Reference<const Graphics::ResourceBinding<Graphics::TextureSampler>>
					(*findFn)(const ViewportData*, const Graphics::BindingSet::BindingDescriptor&) =
					[](const ViewportData* self, const Graphics::BindingSet::BindingDescriptor& desc) { 
					const auto binding = self->FindTextureSamplerBinding(desc.name); 
					if (binding != nullptr || self->ShaderClass() == nullptr || self->m_context == nullptr) return binding;
					else return self->ShaderClass()->DefaultTextureSamplerBinding(desc.name, self->m_context->Graphics()->Device());
				};
				functions.textureSampler = Graphics::BindingSet::BindingSearchFn<Graphics::TextureSampler>(findFn, this);
			}
			{
				static Reference<const Graphics::ResourceBinding<Graphics::TextureView>>
					(*findFn)(const ViewportData*, const Graphics::BindingSet::BindingDescriptor&) =
					[](const ViewportData* self, const Graphics::BindingSet::BindingDescriptor& desc) { return self->FindTextureViewBinding(desc.name); };
				functions.textureView = Graphics::BindingSet::BindingSearchFn<Graphics::TextureView>(findFn, this);
			}
			{
				static Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>>
					(*findFn)(const ViewportData*, const Graphics::BindingSet::BindingDescriptor&) =
					[](const ViewportData* self, const Graphics::BindingSet::BindingDescriptor& desc) { return self->FindBindlessStructuredBufferSetBinding(desc.name); };
				functions.bindlessStructuredBuffers = Graphics::BindingSet::BindingSearchFn<Graphics::BindlessSet<Graphics::ArrayBuffer>::Instance>(findFn, this);
			}
			{
				static Reference<const Graphics::ResourceBinding<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>>
					(*findFn)(const ViewportData*, const Graphics::BindingSet::BindingDescriptor&) =
					[](const ViewportData* self, const Graphics::BindingSet::BindingDescriptor& desc) { return self->FindBindlessTextureSamplerSetBinding(desc.name); };
				functions.bindlessTextureSamplers = Graphics::BindingSet::BindingSearchFn<Graphics::BindlessSet<Graphics::TextureSampler>::Instance>(findFn, this);
			}
			return functions;
		}

		/// <summary> Generated vertex input layout from ViewportData </summary>
		inline Stacktor<Graphics::Experimental::GraphicsPipeline::VertexInputInfo, 4u> VertexInputInfo()const {
			Stacktor<Graphics::Experimental::GraphicsPipeline::VertexInputInfo, 4u> inputs;
			auto addVertexInputs = [&](size_t count, const auto& getBuffer, auto inputRate) {
				for (size_t i = 0u; i < count; i++) {
					inputs.Push({});
					Graphics::Experimental::GraphicsPipeline::VertexInputInfo& info = inputs[inputs.Size() - 1u];
					info.inputRate = inputRate;
					const auto vertexBuffer = getBuffer(i);
					if (vertexBuffer == nullptr) continue;
					info.bufferElementSize = vertexBuffer->BufferElemSize();
					for (size_t j = 0u; j < vertexBuffer->AttributeCount(); j++) {
						const auto attributeInfo = vertexBuffer->Attribute(j);
						Graphics::Experimental::GraphicsPipeline::VertexInputInfo::LocationInfo location = {};
						location.location = attributeInfo.location;
						location.bufferElementOffset = attributeInfo.offset;
						info.locations.Push(location);
					}
				}
			};
			addVertexInputs(VertexBufferCount(), [&](size_t index) { return VertexBuffer(index); },
				Graphics::Experimental::GraphicsPipeline::VertexInputInfo::InputRate::VERTEX);
			addVertexInputs(InstanceBufferCount(), [&](size_t index) { return InstanceBuffer(index); },
				Graphics::Experimental::GraphicsPipeline::VertexInputInfo::InputRate::INSTANCE);
			return inputs;
		}
	};
}
