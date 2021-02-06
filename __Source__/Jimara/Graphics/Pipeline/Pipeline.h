#pragma once
#include <stdint.h>
namespace Jimara {
	namespace Graphics {
		class Pipeline;
		class PipelineDescriptor;

		/// <summary> Pipeline stages </summary>
		enum class PipelineStage : uint8_t {
			// No stage
			NONE = 0,

			// Compute shader
			COMPUTE = 1,

			// Vertex shader
			VERTEX = (1 << 1),

			// Fragment shader
			FRAGMENT = (1 << 2)
		};

		/// <summary> Pipeline stage bitmask </summary>
		typedef uint8_t PipelineStageMask;

		/// <summary> Casts stage to Stage to StageMask </summary>
		inline static PipelineStageMask StageMask(PipelineStage stage) {
			return static_cast<PipelineStageMask>(stage);
		}

		/// <summary>
		/// Makes a stage mask from stage list
		/// </summary>
		/// <typeparam name="...Stages"> Basically, some amount of repeating Stage keywords </typeparam>
		/// <param name="stage"> First stage </param>
		/// <param name="anotherStage"> Second stage </param>
		/// <param name="...rest"> Rest of the stages </param>
		/// <returns> stage | PipelineStageMask(rest...) </returns>
		template<typename... Stages>
		inline static PipelineStageMask StageMask(PipelineStage stage, PipelineStage anotherStage, Stages... rest) {
			return PipelineStageMask(stage) | StageMask(anotherStage, rest...);
		}
	}
}
#include "Shader.h"
#include "../Memory/Buffers.h"
#include "../Memory/Texture.h"


namespace Jimara {
	namespace Graphics {
		class PipelineDescriptor : public virtual Object {
		public:
			class BindingSetDescriptor : public virtual Object {
			public:
				struct BindingInfo {
					PipelineStageMask stages;
					uint32_t binding;
				};

				inline virtual ~BindingSetDescriptor() {}

				virtual bool SetByEnvironment()const = 0;


				virtual size_t ConstantBufferCount()const = 0;

				virtual BindingInfo ConstantBufferInfo(size_t index)const = 0;

				virtual Reference<Buffer> ConstantBuffer(size_t index) = 0;


				virtual size_t StructuredBufferCount()const = 0;

				virtual BindingInfo StructuredBufferInfo(size_t index)const = 0;

				virtual Reference<ArrayBuffer> StructuredBuffer(size_t index) = 0;


				virtual size_t TextureSamplerCount()const = 0;

				virtual BindingInfo TextureSamplerInfo(size_t index)const = 0;

				virtual Reference<TextureSampler> Sampler(size_t index) = 0;
			};

			virtual size_t BindingSetCount()const = 0;

			virtual BindingSetDescriptor* BindingSet(size_t index)const = 0;
		};

		class Pipeline : public virtual Object {
		public:
		};
	}
}
