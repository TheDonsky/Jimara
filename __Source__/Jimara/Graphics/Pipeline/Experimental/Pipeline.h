#pragma once
#include "../../Data/ShaderBinaries/ShaderResourceBindings.h"


namespace Jimara {
	namespace Graphics {
	namespace Experimental {
		template<typename ResourceType>
		using ResourceBinding = ShaderResourceBindings::ShaderBinding<ResourceType>;

		using InFlightBufferInfo = Graphics::Pipeline::CommandBufferInfo;

		class JIMARA_API Pipeline : public virtual Object {
		public:
			virtual size_t BindingSetCount()const = 0;
		};

		class VertexInput : public virtual Pipeline {
		public:
			virtual void Bind(CommandBuffer* commandBuffer) = 0;
		};

		class JIMARA_API GraphicsPipeline : public virtual Pipeline {
		public:
			struct VertexInputInfo;
			struct Descriptor;
			enum class BlendMode : uint8_t;
			using IndexType = Graphics::GraphicsPipeline::IndexType;

			virtual Reference<VertexInput> CreateVertexInput(
				const ResourceBinding<Graphics::ArrayBuffer>** vertexBuffers,
				const ResourceBinding<Graphics::ArrayBuffer>* indexBuffer) = 0;

			virtual void Draw(CommandBuffer* commandBuffer, size_t indexCount, size_t instanceCount) = 0;

			virtual void DrawIndirect(CommandBuffer* commandBuffer, IndirectDrawBuffer* indirectBuffer, size_t drawCount) = 0;
		};

		struct JIMARA_API GraphicsPipeline::VertexInputInfo final {
			enum class JIMARA_API InputRate : uint8_t {
				VERTEX = 0,
				INSTANCE = 1
			};

			using AttributeType = Graphics::VertexBuffer::AttributeInfo::Type;

			struct JIMARA_API LocationInfo final {
				std::optional<size_t> location;
				std::string_view name;
				size_t bufferOffset = 0u;
				AttributeType attributeType = AttributeType::TYPE_COUNT;
			};

			InputRate inputRate = InputRate::VERTEX;
			Stacktor<LocationInfo, 4u> locations;
		};

		enum class JIMARA_API GraphicsPipeline::BlendMode : uint8_t {
			REPLACE = 0,
			ALPHA_BLEND = 1,
			ADDITIVE = 2
		};

		struct JIMARA_API GraphicsPipeline::Descriptor final {
			Reference<const SPIRV_Binary> vertexShader;
			Reference<const SPIRV_Binary> fragmentShader;
			BlendMode blendMode = BlendMode::REPLACE;
			IndexType indexType = IndexType::TRIANGLE;
			Stacktor<VertexInputInfo, 4u> vertexInput;
		};

		class JIMARA_API ComputePipeline : public virtual Pipeline {
		public:
			virtual void Dispatch(CommandBuffer* commandBuffer, const Size3& workGroupCount) = 0;
		};

		class JIMARA_API BindingSet : public virtual Object {
		public:
			struct BindingDescriptor;
			struct Descriptor;

			virtual void Update(size_t inFlightCommandBufferIndex) = 0;

			virtual void Bind(InFlightBufferInfo inFlightBuffer) = 0;

			template<typename ResourceType>
			inline static Reference<const ResourceBinding<ResourceType>> FailToFind(BindingDescriptor) { return nullptr; }
		};

		struct JIMARA_API BindingSet::BindingDescriptor final {
			std::string_view bindingName;
			size_t setBindingIndex = 0u;
		};

		struct JIMARA_API BindingSet::Descriptor final {
			Reference<const Pipeline> pipeline;
			size_t bindingSetId = 0u;

			template<typename ResourceType>
			using BindingSearchFn = Function<Reference<const ResourceBinding<ResourceType>>, BindingDescriptor>;

			BindingSearchFn<Buffer> findConstantBuffer = Function(BindingSet::FailToFind<Buffer>);
			BindingSearchFn<ArrayBuffer> findStructuredBuffer = Function(BindingSet::FailToFind<ArrayBuffer>);
			BindingSearchFn<TextureSampler> findTextureSampler = Function(BindingSet::FailToFind<TextureSampler>);
			BindingSearchFn<TextureView> findTextureView = Function(BindingSet::FailToFind<TextureView>);
			
			BindingSearchFn<BindlessSet<ArrayBuffer>::Instance> findBindlessStructuredBuffers =
				Function(BindingSet::FailToFind<BindlessSet<ArrayBuffer>::Instance>);
			BindingSearchFn<BindlessSet<TextureSampler>::Instance> findBindlessTextureSamplers =
				Function(BindingSet::FailToFind<BindlessSet<TextureSampler>::Instance>);
		};

		class JIMARA_API BindingPool : public virtual Object {
		public:
			virtual Reference<BindingSet> AllocateBindingSet(const BindingSet::Descriptor& descriptor) = 0;

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
