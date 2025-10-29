#pragma once
#include "../../../../Data/Materials/Material.h"
#include "../../../Scene/SceneObjectCollection.h"
#include "../../../Layers.h"
#include "../../ViewportDescriptor.h"


namespace Jimara {
	/// <summary>
	/// Simple descriptor of a graphics scene object
	/// </summary>
	class JIMARA_API GraphicsObjectDescriptor : public virtual Object {
	public:
		/// <summary> Flags for geometry </summary>
		enum class GeometryFlags : uint32_t;

		/// <summary> Per-vertex buffer information </summary>
		struct PerVertexBufferData;

		/// <summary> Index buffer information </summary>
		struct IndexBuffer;

		/// <summary> Per-instance buffer information </summary>
		struct PerInstanceBufferData;

		/// <summary> Information about active/live instances </summary>
		struct InstanceInfo;

		/// <summary> Details about rendered geometry </summary>
		struct GeometryDescriptor;

		/// <summary> Per-viewport graphics object </summary>
		class ViewportData;

		/// <summary> Information about vertex input buffer </summary>
		struct VertexBufferInfo;

		/// <summary> Vertex input for pipelines </summary>
		struct VertexInputInfo;

		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="shader"> Lit-shader (Because of some dependencies, this can not change, threfore we have it kind of hard coded here) </param>
		/// <param name="layer"> Graphics layer for filtering (Because of some dependencies, this can not change, threfore we have it kind of hard coded here) </param>
		inline GraphicsObjectDescriptor(const Material::LitShader* shader, Jimara::Layer layer) 
			: m_shader(shader), m_layer(layer) {}

		/// <summary> Virtual destructor </summary>
		inline virtual ~GraphicsObjectDescriptor() {}

		/// <summary> Lit-shader to use for rendering </summary>
		inline const Material::LitShader* Shader()const { return m_shader; }

		/// <summary> Graphics layer for filtering </summary>
		inline Jimara::Layer Layer()const { return m_layer; }

		/// <summary>
		/// Retrieves frustrum-specific object descriptor
		/// <para/> If nullptr is returned, that means that the object should not be rendered for a specific frustrum.
		/// </summary>
		/// <param name="frustrum"> "Target frustrum" (can be nullptr and that specific case means the "default" descriptor, whatever that means for each object type) </param>
		/// <returns> Per-frustrum object descriptor </returns>
		virtual Reference<const ViewportData> GetViewportData(const RendererFrustrumDescriptor* frustrum) = 0;

		/// <summary>
		/// SceneObjectCollection<GraphicsObjectDescriptor> will flush on Scene::GraphicsContext::OnGraphicsSynch
		/// </summary>
		/// <param name="context"> Scene context </param>
		/// <returns> Scene::GraphicsContext::OnGraphicsSynch </returns>
		inline static Event<>& OnFlushSceneObjectCollections(SceneContext* context) { return context->Graphics()->OnGraphicsSynch(); }

		/// <summary> Set of all GraphicsObjectDescriptors tied to a scene </summary>
		typedef SceneObjectCollection<GraphicsObjectDescriptor> Set;


	private:
		// Lit-Shader (Because of some dependencies, this can not change, threfore we have it kind of hard coded here)
		const Reference<const Material::LitShader> m_shader;

		// Layer
		const Jimara::Layer m_layer;
	};


	/// <summary> Information about vertex input buffer </summary>
	struct JIMARA_API GraphicsObjectDescriptor::VertexBufferInfo final {
		/// <summary> Basic information about layout </summary>
		Graphics::GraphicsPipeline::VertexInputInfo layout;

		/// <summary> Vertex buffer binding </summary>
		Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> binding;
	};

	/// <summary> Vertex input for pipelines </summary>
	struct JIMARA_API GraphicsObjectDescriptor::VertexInputInfo final {
		/// <summary> Vertex buffers </summary>
		Stacktor<VertexBufferInfo, 4u> vertexBuffers;

		/// <summary> Index buffer binding </summary>
		Reference<const Graphics::ResourceBinding<Graphics::ArrayBuffer>> indexBuffer;
	};



	/// <summary> Flags for geometry </summary>
	enum class GraphicsObjectDescriptor::GeometryFlags : uint32_t {
		/// <summary> Empty bitmask </summary>
		NONE = 0,

		/// <summary> If set, this flag tells the renderers that unless vertex position buffer changes, the content will stay constant </summary>
		VERTEX_POSITION_CONSTANT = (1 << 0),

		/// <summary> If set, this flag tells the renderers that unless instance transform buffer changes, the content will stay constant </summary>
		INSTANCE_TRANSFORM_CONSTANT = (1 << 0)
	};

	// Define boolean operations for GeometryFlags:
	JIMARA_DEFINE_ENUMERATION_BOOLEAN_OPERATIONS(GraphicsObjectDescriptor::GeometryFlags);

	/// <summary> Per-vertex buffer information </summary>
	struct GraphicsObjectDescriptor::PerVertexBufferData {
		/// <summary> Field buffer </summary>
		Reference<Graphics::ArrayBuffer> buffer;

		/// <summary> Offset (in bytes) to the first relevant entry within the buffer origin </summary>
		uint32_t bufferOffset = 0u;

		/// <summary> Stride (in bytes) between values for any given instance </summary>
		uint32_t perVertexStride = 0u;

		/// <summary> Stride (in bytes) between buffer chunks of individual instances </summary>
		uint32_t perInstanceStride = 0u;

		/// <summary> Number of entries per-instance (can be rereved based on strides, but we do not enforce tight packing) </summary>
		uint32_t numEntriesPerInstance = 0u;
	};

	/// <summary> Index buffer information </summary>
	struct GraphicsObjectDescriptor::IndexBuffer {
		/// <summary> Index buffer (currently, only uint32_t-based index buffers are supported) </summary>
		Reference<Graphics::ArrayBuffer> buffer;

		/// <summary> 
		/// Offset (in bytes) of the first index within the buffer.
		/// <para/> Mainly to allow sharing the same buffer between multiple objects, where applicable.
		/// </summary>
		uint32_t baseIndexOffset = 0u;

		/// <summary> Index count per-instance </summary>
		uint32_t indexCount = 0u;
	};

	/// <summary> Per-instance buffer information </summary>
	struct GraphicsObjectDescriptor::PerInstanceBufferData {
		/// <summary> Field buffer </summary>
		Reference<Graphics::ArrayBuffer> buffer;

		/// <summary> Offset (in bytes) to the first relevant entry within the buffer origin </summary>
		uint32_t bufferOffset = 0u;

		/// <summary> Distance (in bytes) in-between entries </summary>
		uint32_t elemStride = 0u;
	};

	/// <summary> Information about active/live instances </summary>
	struct GraphicsObjectDescriptor::InstanceInfo {
		/// <summary> Number of instances ('live instances' can be overriden by a lower value via range buffer) </summary>
		uint32_t count = 0u;

		/// <summary> 
		/// Optional buffer, storing ranges of the instances that are considered 'active'/'live'/'visible' (ei need to be rendered).
		/// <para/> Ranges consist of two variables: "first instance index" and "instance count" (both uint32_t). 
		/// Each range with the two values will tell the system to render instances from "first instance index" to ("first instance index" + "instance count");
		/// <para/> There can be more live ranges than one; having said that, keeping just one might be optimal where possible, 
		/// because some renderers might have special optimizations for that case;
		/// <para/> If buffer is not specified, all instances will be considered as 'live'.
		/// </summary>
		Reference<Graphics::ArrayBuffer> liveInstanceRangeBuffer;

		/// <summary> Offset (in bytes) from the liveInstanceRangeBuffer origin to the first "first instance index" value </summary>
		uint32_t firstInstanceIndexOffset = 0u;

		/// <summary> Distance (in bytes) between "first instance index" values for each range entry </summary>
		uint32_t firstInstanceIndexStride = 0u;

		/// <summary> Offset (in bytes) from the liveInstanceRangeBuffer origin to the first "instance count" value </summary>
		uint32_t instanceCountOffset = 0u;

		/// <summary> Distance (in bytes) between "instance count" values for each range entry </summary>
		uint32_t instanceCountStride = 0u;

		/// <summary> 
		/// Total number of range entries within liveInstanceRangeBuffer.
		/// <para/> There can be more live ranges than one; having said that, keeping just one might be optimal where possible, 
		/// because some renderers might have special optimizations for that case;
		/// <para/> 0 count means none of the instances are 'live'. Having said that, the value is ignored if liveInstanceRangeBuffer is not provided.
		/// </summary>
		uint32_t liveInstanceRangeCount = 0u;
	};

	/// <summary> Details about rendered geometry </summary>
	struct GraphicsObjectDescriptor::GeometryDescriptor {
		/// <summary> Vertex position buffer (JM_VertexPosition; Always storing a Vector3/vec3 data) </summary>
		PerVertexBufferData vertexPositions;

		/// <summary> Index buffer </summary>
		IndexBuffer indexBuffer;

		/// <summary> Instance transform buffer (JM_ObjectTransform; Always storing a Matrix4/mat4 data) </summary>
		PerInstanceBufferData instanceTransforms;

		/// <summary> Instance information </summary>
		InstanceInfo instances;

		/// <summary> Flags for additional controls </summary>
		GeometryFlags flags = GeometryFlags::NONE;
	};



	/// <summary> Per-viewport graphics object </summary>
	class JIMARA_API GraphicsObjectDescriptor::ViewportData : public virtual Object {
	private:
		// Type of the geometry primitives or index interpretation(TRIANGLE(filled; multiples of 3) or EDGE(wireframe; pairs of 2))
		const Graphics::GraphicsPipeline::IndexType m_geometryType;

	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="geometryType"> Type of the geometry primitives or index interpretation (TRIANGLE(filled; multiples of 3) or EDGE(wireframe; pairs of 2)) </param>
		inline ViewportData(Graphics::GraphicsPipeline::IndexType geometryType) : m_geometryType(geometryType) {}

		/// <summary> Type of the geometry primitives or index interpretation (TRIANGLE(filled; multiples of 3) or EDGE(wireframe; pairs of 2)) </summary>
		inline Graphics::GraphicsPipeline::IndexType GeometryType()const { return m_geometryType; }

		/// <summary> 
		/// Should give access to the resource bindings needed for binding set creation; 
		/// <para/> Notes:
		///		<para/> 0. Whatever is returned from here, should be valid throught the lifecycle of the object, 
		///			since anyone can make a request and it's up to the caller when and how to use it;
		///		<para/> 1. There may be more than one calls to this function from multiple users and it is up to the 
		///			implementation to make sure the returned value stays consistent.
		/// </summary>
		virtual Graphics::BindingSet::BindingSearchFunctions BindingSearchFunctions()const = 0;

		/// <summary>
		/// Should give access to the vertex input information for pipeline and Graphics::VertexInput creation;
		/// <para/> Notes:
		///		<para/> 0. Whatever is returned from here, should be valid throught the lifecycle of the object, 
		///			since anyone can make a request and it's up to the caller when and how to use it;
		///		<para/> 1. There may be more than one calls to this function from multiple users and it is up to the 
		///			implementation to make sure the returned value stays consistent.
		/// </summary>
		virtual VertexInputInfo VertexInput()const = 0;

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
		/// Should fill-in the geometry descriptor for the renderers.
		/// <para/> Currently, we want this feature only for generating the acceleration structures for the RT renderers
		/// and not all renderer components are forced to support it. 
		/// Down the line, we might take-out the VertexInput info entirely and receive standard data from GeometryDescriptor.
		/// </summary>
		/// <param name="descriptor"> Descriptor to fill-in </param>
		virtual void GetGeometry(GeometryDescriptor& descriptor)const { descriptor = {}; }

		/// <summary>
		/// Drawing component reference by JM_ObjectIndex
		/// </summary>
		/// <param name="objectIndex"> Object index (same as JM_ObjectIndex vertex input) </param>
		/// <returns> Component reference </returns>
		virtual Reference<Component> GetComponent(size_t objectIndex)const = 0;
	};
}
