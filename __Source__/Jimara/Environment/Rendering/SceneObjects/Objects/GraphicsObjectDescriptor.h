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
	class JIMARA_API GraphicsObjectDescriptor::ViewportData : public virtual Object {
	private:
		// Scene context
		const Reference<SceneContext> m_context;

		// Shader class (Because of some dependencies, this can not change, threfore we have it kind of hard coded here)
		const Reference<const Graphics::ShaderClass> m_shaderClass;

		// Type of the geometry primitives or index interpretation(TRIANGLE(filled; multiples of 3) or EDGE(wireframe; pairs of 2))
		const Graphics::GraphicsPipeline::IndexType m_geometryType;

		// Blending mode
		const Graphics::GraphicsPipeline::BlendMode m_blendMode;

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
			Graphics::GraphicsPipeline::BlendMode blendMode)
			: m_context(context), m_shaderClass(shaderClass), m_geometryType(geometryType), m_blendMode(blendMode) {}

		/// <summary> Scene context </summary>
		inline SceneContext* Context()const { return m_context; }

		/// <summary> Shader class to use for rendering </summary>
		inline const Graphics::ShaderClass* ShaderClass()const { return m_shaderClass; }

		/// <summary> Type of the geometry primitives or index interpretation (TRIANGLE(filled; multiples of 3) or EDGE(wireframe; pairs of 2)) </summary>
		inline Graphics::GraphicsPipeline::IndexType GeometryType()const { return m_geometryType; }

		/// <summary> Blending mode </summary>
		const Graphics::GraphicsPipeline::BlendMode BlendMode()const { return m_blendMode; }

		/// <summary> Boundaries, covering the entire volume of the scene object (useful for culling and sorting) </summary>
		virtual AABB Bounds()const = 0;


		/// <summary> Number of vertex buffers, used by the vertex shader (tied to material; should not change) </summary>
		virtual size_t VertexBufferCount()const = 0;

		/// <summary>
		/// Vertex buffer by index
		/// </summary>
		/// <param name="index"> Vertex buffer index </param>
		/// <returns> Index'th vertex buffer </returns>
		virtual Reference<Graphics::Legacy::VertexBuffer> VertexBuffer(size_t index)const = 0;


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
		virtual Reference<Graphics::Legacy::InstanceBuffer> InstanceBuffer(size_t index)const = 0;


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

		/// <summary> 
		/// Should give access to the resource bindings needed for binding set creation; 
		/// <para/> Notes:
		///		<para/> 0. Whatever is returned from here, should be valid throught the lifecycle of the object, 
		///			since anyone can make a request and it's up to the caller when and how to use it;
		///		<para/> 1. There may be more than one calls to this function from multiple users and it is up to the 
		///			implementation to make sure the returned value stays consistent.
		/// </summary>
		virtual Graphics::BindingSet::BindingSearchFunctions BindingSearchFunctions()const = 0;

		/// <summary> Generated vertex input layout from ViewportData </summary>
		inline Stacktor<Graphics::GraphicsPipeline::VertexInputInfo, 4u> VertexInputInfo()const {
			Stacktor<Graphics::GraphicsPipeline::VertexInputInfo, 4u> inputs;
			auto addVertexInputs = [&](size_t count, const auto& getBuffer, auto inputRate) {
				for (size_t i = 0u; i < count; i++) {
					inputs.Push({});
					Graphics::GraphicsPipeline::VertexInputInfo& info = inputs[inputs.Size() - 1u];
					info.inputRate = inputRate;
					const auto vertexBuffer = getBuffer(i);
					if (vertexBuffer == nullptr) continue;
					info.bufferElementSize = vertexBuffer->BufferElemSize();
					for (size_t j = 0u; j < vertexBuffer->AttributeCount(); j++) {
						const auto attributeInfo = vertexBuffer->Attribute(j);
						Graphics::GraphicsPipeline::VertexInputInfo::LocationInfo location = {};
						location.location = attributeInfo.location;
						location.bufferElementOffset = attributeInfo.offset;
						info.locations.Push(location);
					}
				}
			};
			addVertexInputs(VertexBufferCount(), [&](size_t index) { return VertexBuffer(index); },
				Graphics::GraphicsPipeline::VertexInputInfo::InputRate::VERTEX);
			addVertexInputs(InstanceBufferCount(), [&](size_t index) { return InstanceBuffer(index); },
				Graphics::GraphicsPipeline::VertexInputInfo::InputRate::INSTANCE);
			return inputs;
		}
	};
}
