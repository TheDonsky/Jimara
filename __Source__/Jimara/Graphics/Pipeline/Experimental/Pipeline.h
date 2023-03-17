#pragma once
#include "../Shader.h"
#include "../../Data/ShaderBinaries/ShaderResourceBindings.h"


namespace Jimara {
	namespace Graphics {
	namespace Experimental {
		template<typename ResourceType>
		using ResourceBinding = ShaderResourceBindings::ShaderBinding<ResourceType>;

		using InFlightBufferInfo = Graphics::Pipeline::CommandBufferInfo;

		class JIMARA_API Pipeline : public virtual Object {
		public:
			virtual void BindingSetCount()const = 0;

			virtual void Execute(CommandBuffer* commandBuffer) = 0;
		};

		class JIMARA_API GraphicsPipeline : public virtual Pipeline {
		public:
			struct VertexInputInfo;
			struct Descriptor;
		};

		struct JIMARA_API GraphicsPipeline::VertexInputInfo final {
			enum class JIMARA_API InputRate : uint8_t {
				VERTEX = 0,
				INSTANCE = 1
			};

			using AttributeType = Graphics::VertexBuffer::AttributeInfo::Type;

			struct JIMARA_API LocationInfo final {
				size_t location = 0u;
				size_t bufferOffset = 0u;
				AttributeType attributeType = AttributeType::TYPE_COUNT;
			};

			InputRate inputRate = InputRate::VERTEX;
			Stacktor<LocationInfo, 4u> locations;
		};

		struct JIMARA_API GraphicsPipeline::Descriptor final {
			Reference<Shader> vertexShader;
			Reference<Shader> fragmentShader;
			Stacktor<VertexInputInfo, 4u> vertexBuffers;
		};

		class JIMARA_API ComputePipeline : public virtual Pipeline {
		public:
			virtual void SetBlockCount(const Size3& workgroupCount) = 0;
		};

		class JIMARA_API BindingSet : public virtual Object {
		public:
			struct BindingDescriptor;
			struct Descriptor;

			virtual void Update(size_t inFlightCommandBufferIndex) = 0;

			virtual void Bind(InFlightBufferInfo inFlightBuffer) = 0;

		private:
			template<typename ResourceType>
			inline static Reference<ResourceType> FailToFind(BindingDescriptor) { return nullptr; }
		};

		struct JIMARA_API BindingSet::BindingDescriptor final {
			std::string_view bindingName;
			size_t setBindingIndex = 0u;
		};

		struct JIMARA_API BindingSet::Descriptor final {
			Reference<Pipeline> pipeline;
			size_t bindingSetId = 0u;

			Function<Reference<Buffer>, BindingDescriptor> findConstantBuffer = Function(BindingSet::FailToFind<Buffer>);
			Function<Reference<ArrayBuffer>, BindingDescriptor> findStructuredBuffer = Function(BindingSet::FailToFind<ArrayBuffer>);
			Function<Reference<TextureSampler>, BindingDescriptor> findTextureSampler = Function(BindingSet::FailToFind<TextureSampler>);
			Function<Reference<TextureView>, BindingDescriptor> findTextureView = Function(BindingSet::FailToFind<TextureView>);
			
			Function<Reference<BindlessSet<ArrayBuffer>::Instance>, BindingDescriptor> findBindlessStructuredBuffers =
				Function(BindingSet::FailToFind<BindlessSet<ArrayBuffer>::Instance>);
			Function<Reference<BindlessSet<TextureSampler>::Instance>, BindingDescriptor> findBindlessTextureSamplers =
				Function(BindingSet::FailToFind<BindlessSet<TextureSampler>::Instance>);
		};

		class JIMARA_API BindingPool : public virtual Object {
		public:
			virtual Reference<BindingSet> AllocateBindingSet(const BindingSet::Descriptor& descriptor) = 0;

			virtual void UpdateAllBindingSets(size_t inFlightCommandBufferIndex) = 0;
		};
	}
	}
}
