#pragma once
#include "Pipeline.h"
#include "../ComputePipeline.h"
#include "../GraphicsPipeline.h"


namespace Jimara {
	namespace Graphics {
	namespace Experimental {
		class JIMARA_API LegacyPipeline : public virtual Graphics::Pipeline {
		public:
			static Reference<LegacyPipeline> Create(
				GraphicsDevice* device, size_t maxInFlightCommandBuffers,
				Experimental::Pipeline* pipeline, const Graphics::PipelineDescriptor* descriptor);

			virtual ~LegacyPipeline();

			/// <summary>
			/// Executes pipeline on the command buffer
			/// </summary>
			/// <param name="bufferInfo"> Command buffer and it's index </param>
			virtual void Execute(const CommandBufferInfo& bufferInfo) override;

		private:
			template<typename ResourceType>
			class BindingMapping : public virtual ResourceBinding<ResourceType> {
			public:
				inline BindingMapping(size_t bindingIndex) : m_bindingIndex(bindingIndex) {}
				inline virtual ~BindingMapping() {}
				inline size_t BindingIndex()const { return m_bindingIndex; }
			private:
				size_t m_bindingIndex;
			};

			template<typename ResourceType>
			using BindingMappings = Stacktor<Reference<BindingMapping<ResourceType>>, 4u>;

			template<typename ResourceType>
			using BindlessSetBinding = Reference<ResourceBinding<typename BindlessSet<ResourceType>::Instance>>;
			
			struct BindingSetMappings {
				size_t bindingSetIndex = 0u;
				BindingMappings<Buffer> constantBuffers;
				BindingMappings<ArrayBuffer> structuredBuffers;
				BindingMappings<TextureSampler> textureSamplers;
				BindingMappings<TextureView> textureViews;
				BindlessSetBinding<ArrayBuffer> bindlessStructuredBuffers;
				BindlessSetBinding<TextureSampler> bindlessTextureSamplers;
				Reference<BindingSet> bindingSet;
			};

			using PipelineBindings = Stacktor<BindingSetMappings, 4u>;

			struct PipelineData {
				Reference<GraphicsDevice> device;
				Reference<const Graphics::PipelineDescriptor> descriptor;
				Reference<BindingPool> bindingPool;
				PipelineBindings pipelineBindings;
			};
			const PipelineData m_data;

			LegacyPipeline(PipelineData&& data);
		};

		class JIMARA_API LegacyComputePipeline : public virtual Graphics::ComputePipeline {
		public:
			static Reference<LegacyComputePipeline> Create(
				GraphicsDevice* device, size_t maxInFlightCommandBuffers,
				const Graphics::ComputePipeline::Descriptor* descriptor);

			virtual ~LegacyComputePipeline();

			/// <summary>
			/// Executes pipeline on the command buffer
			/// </summary>
			/// <param name="bufferInfo"> Command buffer and it's index </param>
			virtual void Execute(const CommandBufferInfo& bufferInfo) override;

		private:
			const Reference<const Graphics::ComputePipeline::Descriptor> m_descriptor;
			const Reference<Experimental::ComputePipeline> m_computePipeline;
			const Reference<LegacyPipeline> m_bindingSets;

			LegacyComputePipeline(
				const Graphics::ComputePipeline::Descriptor* descriptor,
				Experimental::ComputePipeline* pipeline, LegacyPipeline* bindingSets);
		};


		class JIMARA_API LegacyGraphicsPipeline : public virtual Graphics::GraphicsPipeline {
		public:
			static Reference<LegacyGraphicsPipeline> Create(
				RenderPass* renderPass, size_t maxInFlightCommandBuffers,
				Graphics::GraphicsPipeline::Descriptor* descriptor);

			virtual ~LegacyGraphicsPipeline();

			/// <summary>
			/// Executes pipeline on the command buffer
			/// </summary>
			/// <param name="bufferInfo"> Command buffer and it's index </param>
			virtual void Execute(const CommandBufferInfo& bufferInfo) override;

		private:
			const Reference<Graphics::GraphicsPipeline::Descriptor> m_descriptor;
			const Reference<Experimental::GraphicsPipeline> m_graphicsPipeline;
			const Reference<Experimental::VertexInput> m_vertexInput;
			const Reference<LegacyPipeline> m_bindingSets;
			using VertexBuffers = Stacktor<Reference<ResourceBinding<ArrayBuffer>>, 4u>;
			const VertexBuffers m_vertexBuffers;
			const Reference<ResourceBinding<ArrayBuffer>> m_indexBuffer;

			LegacyGraphicsPipeline(
				Graphics::GraphicsPipeline::Descriptor* descriptor,
				Experimental::GraphicsPipeline* graphicsPipeline,
				Experimental::VertexInput* vertexInput,
				LegacyPipeline* bindingSets,
				VertexBuffers&& vertexBuffers,
				ResourceBinding<ArrayBuffer>* indexBuffer);
		};
	}
	}
}
