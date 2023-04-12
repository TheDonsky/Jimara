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
	};


	/// <summary> Per-viewport graphics object </summary>
	class JIMARA_API GraphicsObjectDescriptor::ViewportData : public virtual Object, public virtual Graphics::ShaderResourceBindings::ShaderResourceBindingSet {
	private:
		// Shader class (Because of some dependencies, this can not change, threfore we have it kind of hard coded here)
		const Reference<const Graphics::ShaderClass> m_shaderClass;

		// Graphics layer (Because of some dependencies, this can not change, threfore we have it kind of hard coded here)
		const Jimara::Layer m_layer;

		// Type of the geometry primitives or index interpretation(TRIANGLE(filled; multiples of 3) or EDGE(wireframe; pairs of 2))
		const Graphics::GraphicsPipeline::IndexType m_geometryType;

		// Blending mode
		const Graphics::Experimental::GraphicsPipeline::BlendMode m_blendMode;

	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="shaderClass"> Shader class (Because of some dependencies, this can not change, threfore we have it kind of hard coded here) </param>
		/// <param name="layer"> Graphics layer for filtering (Because of some dependencies, this can not change, threfore we have it kind of hard coded here) </param>
		/// <param name="geometryType"> Type of the geometry primitives or index interpretation (TRIANGLE(filled; multiples of 3) or EDGE(wireframe; pairs of 2)) </param>
		inline ViewportData(
			const Graphics::ShaderClass* shaderClass, Jimara::Layer layer,
			Graphics::GraphicsPipeline::IndexType geometryType,
			Graphics::Experimental::GraphicsPipeline::BlendMode blendMode)
			: m_shaderClass(shaderClass), m_layer(layer), m_geometryType(geometryType), m_blendMode(blendMode) {}

		/// <summary> Shader class to use for rendering </summary>
		inline const Graphics::ShaderClass* ShaderClass()const { return m_shaderClass; }

		/// <summary> Graphics layer for filtering </summary>
		inline Jimara::Layer Layer()const { return m_layer; }

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
	};
}
