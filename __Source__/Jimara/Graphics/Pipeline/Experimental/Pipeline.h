#pragma once
#include "../../Data/ShaderBinaries/ShaderResourceBindings.h"


namespace Jimara {
	namespace Graphics {
	namespace Experimental {
		/// <summary>
		/// Binding sets access the dynamic resources through ResourceBinding objects.
		/// You may consider them as pointers of smart pointers.
		/// <para/> Note: ResourceBinding objects are not thread-safe by design and one should be careful 
		///		about when the bound objects are changed.
		/// <typeparam name="ResourceType"> Type of a bound resource </typeparam>
		template<typename ResourceType>
		using ResourceBinding = ShaderResourceBindings::ShaderBinding<ResourceType>;

		/// <summary> Command buffer and in-flight buffer index </summary>
		using InFlightBufferInfo = Graphics::Pipeline::CommandBufferInfo;

		/// <summary>
		/// Pipeline objects are compiled shaders with well-defined input layouts ready to execute on GPU
		/// <para/> Note: Pipelines, generally speaking, will be cached and only one will be created per-configuration.
		/// </summary>
		class JIMARA_API Pipeline : public virtual Object {
		public:
			/// <summary>
			/// Number of bound descriptor sets used by pipeline during execution.
			/// <para/> Note: Compatible binding sets are allocated through BindingPool's interface 
			///		and they should be updated and bound manually.
			/// </summary>
			virtual size_t BindingSetCount()const = 0;
		};

		/// <summary>
		/// Vertex & index buffer input for a graphics pipeline
		/// </summary>
		class JIMARA_API VertexInput : public virtual Object {
		public:
			/// <summary>
			/// Binds vertex buffers to a command buffer
			/// <para/> Note: This should be executed before a corresponding draw call.
			/// </summary>
			/// <param name="commandBuffer"> Command buffer to record bind command to </param>
			virtual void Bind(CommandBuffer* commandBuffer) = 0;
		};

		/// <summary>
		/// Pipeline for drawing graphics objects
		/// <para/> Notes: 
		///		<para/> 0. Graphics pipelines require shader binaries for different stages, 
		///			regular binding sets and vertex input to execute;
		///		<para/> 1. Graphics pipelines are retrieved/created through RenderPass interface.
		/// </summary>
		class JIMARA_API GraphicsPipeline : public virtual Pipeline {
		public:
			/// <summary> Basic information about single vertex buffer </summary>
			struct VertexInputInfo;

			/// <summary> Graphics pipeline descriptor </summary>
			struct Descriptor;

			/// <summary> Blending mode </summary>
			enum class BlendMode : uint8_t;

			/// <summary> Geometry type </summary>
			using IndexType = Graphics::GraphicsPipeline::IndexType;

			/// <summary>
			/// Creates compatible vertex input
			/// </summary>
			/// <param name="vertexBuffers"> Vertex buffer bindings (array size should be the same as the vertexInput list within the descriptor) </param>
			/// <param name="indexBuffer"> 
			///		Index buffer binding 
			///		(bound objects have to be array buffers of uint32_t or uint16_t types; nullptr means indices from 0 to vertexId) 
			/// </param>
			/// <returns> New instance of a vertex input </returns>
			virtual Reference<VertexInput> CreateVertexInput(
				const ResourceBinding<Graphics::ArrayBuffer>* const* vertexBuffers,
				const ResourceBinding<Graphics::ArrayBuffer>* indexBuffer) = 0;

			/// <summary>
			/// Draws bound geometry using the graphics pipeline
			/// </summary>
			/// <param name="commandBuffer"> Command buffer to draw on (has to be invoked within a render pass) </param>
			/// <param name="indexCount"> Number of indices to use from the bound VertexInput's index buffer </param>
			/// <param name="instanceCount"> Number of instances to draw </param>
			virtual void Draw(CommandBuffer* commandBuffer, size_t indexCount, size_t instanceCount) = 0;

			/// <summary>
			/// Draws bound geometry using an indirect draw buffer
			/// </summary>
			/// <param name="commandBuffer"> Command buffer to draw on (has to be invoked within a render pass) </param>
			/// <param name="indirectBuffer"> Draw calls (can be uploaded to a buffer from CPU or built on GPU) </param>
			/// <param name="drawCount"> Number of draw calls (if one wishes to go below indirectBuffer->ObjectCount() feel free to input any number) </param>
			virtual void DrawIndirect(CommandBuffer* commandBuffer, IndirectDrawBuffer* indirectBuffer, size_t drawCount) = 0;
		};

		/// <summary> Basic information about single vertex buffer </summary>
		struct JIMARA_API GraphicsPipeline::VertexInputInfo final {
			/// <summary> Configuration variable for distinguishing buffers that are indexed per index and ones that are per-instance </summary>
			enum class JIMARA_API InputRate : uint8_t {
				/// <summary> Buffer will be mapped per-index during the graphics pipeline execution </summary>
				VERTEX = 0,

				/// <summary> Buffer will be mapped per-instance during the graphics pipeline execution </summary>
				INSTANCE = 1
			};

			/// <summary> Information about a single layout location </summary>
			struct JIMARA_API LocationInfo final {
				/// <summary> Optional location index (if provided, feel free to ignore the name, otherwise this is not mandatory) </summary>
				std::optional<size_t> location;

				/// <summary> Variable name (can be empty, but in that case location index has to be filled in) </summary>
				std::string_view name;

				/// <summary> Offset from buffer element's start in bytes </summary>
				size_t bufferElementOffset = 0u;
			};

			/// <summary> Configuration variable for distinguishing buffers that are indexed per index and ones that are per-instance </summary>
			InputRate inputRate = InputRate::VERTEX;

			/// <summary> Element size/stride of the corresponding bound vertex buffer </summary>
			size_t bufferElementSize = 0u;

			/// <summary> Locations, extracted from the bound vertex buffer </summary>
			Stacktor<LocationInfo, 4u> locations;
		};


		/// <summary> Blending mode </summary>
		enum class JIMARA_API GraphicsPipeline::BlendMode : uint8_t {
			/// <summary> Opaque/Cutout </summary>
			REPLACE = 0,

			/// <summary> Transparent overlay </summary>
			ALPHA_BLEND = 1,

			/// <summary> Additive transparent </summary>
			ADDITIVE = 2
		};

		/// <summary> Graphics pipeline descriptor </summary>
		struct JIMARA_API GraphicsPipeline::Descriptor final {
			/// <summary> Vertex shader binary </summary>
			Reference<const SPIRV_Binary> vertexShader;

			/// <summary> Fragment shader binary </summary>
			Reference<const SPIRV_Binary> fragmentShader;

			/// <summary> Blending mode </summary>
			BlendMode blendMode = BlendMode::REPLACE;

			/// <summary> geometry type </summary>
			IndexType indexType = IndexType::TRIANGLE;

			/// <summary> Vertex buffer layout </summary>
			Stacktor<VertexInputInfo, 4u> vertexInput;
		};

		/// <summary>
		/// Pipeline for general purpose computations that can be performed on a GPU
		/// <para/> Note: Compute pipelines are created/retrieved through graphics device 
		///		and do not require any description besides the corresponding shader bytecode.
		/// </summary>
		class JIMARA_API ComputePipeline : public virtual Pipeline {
		public:
			/// <summary>
			/// Runs compute kernel through a command buffer
			/// </summary>
			/// <param name="commandBuffer"> Command buffer to record dispatches to </param>
			/// <param name="workGroupCount"> Number of thead blocks to execute </param>
			virtual void Dispatch(CommandBuffer* commandBuffer, const Size3& workGroupCount) = 0;
		};

		/// <summary>
		/// Shaders within the Pipelines get their input through compatible BindingSet instances previously bound to command buffers.
		/// <para/> Notes:
		///		<para/> 0. Binding sets from different pipelines are compatible if and only if they have same set indices and
		///			all descriptor sets including that index are identical between the shaders used for those pipelines;
		///		<para/> 1. Compute and Graphics pipelines may bind their binding sets differently from one another,
		///			and because of that, binding sets allocated for one will not work for the other by default.
		///		<para/> 2. BindingSet objects are allocated through BindingPools using descriptor objects.
		/// </summary>
		class JIMARA_API BindingSet : public virtual Object {
		public:
			/// <summary> Descriptor of a single binding within a set </summary>
			struct BindingDescriptor;

			/// <summary> Descriptor for a BindingSet object allocation </summary>
			struct Descriptor;

			/// <summary>
			/// Stores currently bound resources from the user-provided resource bindings and updates underlying API objects.
			/// <para/> Notes: 
			///		<para/> 0. General usage would be as follows:
			///			<para/> InFlightBufferInfo info = ...
			///			<para/>	bindingSet->Update(info.inFlightBufferId);
			///			<para/> bindingSet->Bind(info);
			///		<para/> 1. If there are many binding sets from the same descriptor pool, 
			///			it will be more optimal to use BindingPool::UpdateAllBindingSets() instead.
			/// </summary>
			/// <param name="inFlightCommandBufferIndex"> Index of an in-flight command buffer for duture binding </param>
			virtual void Update(size_t inFlightCommandBufferIndex) = 0;

			/// <summary>
			/// Binds descriptor set for future pipeline executions.
			/// <para/> Note: All relevant binding sets should be bound using this call before the dispatch or a draw call.
			/// </summary>
			/// <param name="inFlightBuffer"> Command buffer and in-flight index </param>
			virtual void Bind(InFlightBufferInfo inFlightBuffer) = 0;
		};

		/// <summary> Descriptor of a single binding within a set </summary>
		struct JIMARA_API BindingSet::BindingDescriptor final {
			/// <summary> Name of the binding </summary>
			std::string_view bindingName;

			/// <summary> Binding layout index within the set </summary>
			size_t setBindingIndex = 0u;
		};

		/// <summary> Descriptor for a BindingSet object allocation </summary>
		struct JIMARA_API BindingSet::Descriptor final {
			/// <summary> Pipeline object </summary>
			Reference<const Pipeline> pipeline;

			/// <summary> Binding set index (should be within [0 - pipeline->BindingSetCount()) range) </summary>
			size_t bindingSetId = 0u;

			/// <summary>
			/// During set creation, individual resource bindings are mapped using binding search functions;
			/// This is a basic type definition for them
			/// </summary>
			/// <typeparam name="ResourceType"> Type of a bound resource </typeparam>
			template<typename ResourceType>
			using BindingSearchFn = Function<Reference<const ResourceBinding<ResourceType>>, BindingDescriptor>;

			/// <summary>
			/// By default, resource search functions fail automatically; This funtion is their default value
			/// </summary>
			/// <param name=""> Ignored </param>
			/// <typeparam name="ResourceType"> Type of a bound resource </typeparam>
			template<typename ResourceType>
			inline static Reference<const ResourceBinding<ResourceType>> FailToFind(BindingDescriptor) { return nullptr; }

			/// <summary> Should find corresponding resource binding objects for constant buffers </summary>
			BindingSearchFn<Buffer> findConstantBuffer = Function(BindingSet::Descriptor::FailToFind<Buffer>);

			/// <summary> Should find corresponding resource binding objects for array buffers </summary>
			BindingSearchFn<ArrayBuffer> findStructuredBuffer = Function(BindingSet::Descriptor::FailToFind<ArrayBuffer>);

			/// <summary> Should find corresponding resource binding objects for texture samplers </summary>
			BindingSearchFn<TextureSampler> findTextureSampler = Function(BindingSet::Descriptor::FailToFind<TextureSampler>);

			/// <summary> Should find corresponding resource binding objects for texture views </summary>
			BindingSearchFn<TextureView> findTextureView = Function(BindingSet::Descriptor::FailToFind<TextureView>);
			
			/// <summary> Should find corresponding resource binding objects for bindless structured buffers </summary>
			BindingSearchFn<BindlessSet<ArrayBuffer>::Instance> findBindlessStructuredBuffers =
				Function(BindingSet::Descriptor::FailToFind<BindlessSet<ArrayBuffer>::Instance>);

			/// <summary> Should find corresponding resource binding objects for bindless texture samplers </summary>
			BindingSearchFn<BindlessSet<TextureSampler>::Instance> findBindlessTextureSamplers =
				Function(BindingSet::Descriptor::FailToFind<BindlessSet<TextureSampler>::Instance>);
		};

		/// <summary>
		/// Resource pool for binding set allocation
		/// </summary>
		class JIMARA_API BindingPool : public virtual Object {
		public:
			/// <summary>
			/// Creates/Allocated new binding set instance
			/// </summary>
			/// <param name="descriptor"> Binding set descriptor </param>
			/// <returns> New instance of a binding set </returns>
			virtual Reference<BindingSet> AllocateBindingSet(const BindingSet::Descriptor& descriptor) = 0;

			/// <summary>
			/// Equivalent of invoking BindingSet::Update() on all binding sets allocated from this pool.
			/// <para/> Notes: 
			///		<para/> 0. UpdateAllBindingSets is usually much faster than invoking BindingSet::Update() on all sets;
			///		<para/> 1. Using muptiple threads to invoke BindingSet::Update() will not give better performance, 
			///			since BindingPool objects are expected to be internally synchronized.
			/// </summary>
			/// <param name="inFlightCommandBufferIndex"> Index of an in-flight command buffer for duture binding </param>
			virtual void UpdateAllBindingSets(size_t inFlightCommandBufferIndex) = 0;
		};



		class JIMARA_API DeviceExt : public virtual GraphicsDevice {
		public:
			virtual Reference<ComputePipeline> GetComputePipeline(const SPIRV_Binary* computeShader) = 0;

			virtual Reference<BindingPool> CreateBindingPool(size_t inFlightCommandBufferCount) = 0;
		};

		class JIMARA_API RenderPassExt : public virtual RenderPass {
		public:
			virtual Reference<GraphicsPipeline> GetGraphicsPipeline(const GraphicsPipeline::Descriptor& descriptor) = 0;
		};
	}
	}
}
