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
#include "CommandBuffer.h"
#include "../Memory/Buffers.h"
#include "../Memory/Texture.h"
#include <shared_mutex>


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Pipeline binding descriptor interface
		/// </summary>
		class PipelineDescriptor : public virtual Object {
		private:
			// Lock for change synchronisation
			mutable std::shared_mutex m_rwLock;

		public:
			// Lock, the readers(pipelines) aquire, when reading the descriptor content bindings (pipeline shape is meant to be immutable, so this only applies to the buffers)
			class ReadLock : public virtual std::shared_lock<std::shared_mutex> {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="desc"> Pipeline descriptor </param>
				inline ReadLock(const PipelineDescriptor* desc) : std::shared_lock<std::shared_mutex>(desc->m_rwLock) {}

				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="desc"> Pipeline descriptor </param>
				inline ReadLock(const PipelineDescriptor& desc) : ReadLock(&desc) {}
			};

			// Lock, the writers have to aquire, when altering descriptor content bindings (pipeline shape is meant to be immutable, so this only applies to the buffers)
			class WriteLock : public virtual std::unique_lock<std::shared_mutex> {
			public:
				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="desc"> Pipeline descriptor </param>
				inline WriteLock(PipelineDescriptor* desc) : std::unique_lock<std::shared_mutex>(desc->m_rwLock) {}

				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="desc"> Pipeline descriptor </param>
				inline WriteLock(PipelineDescriptor& desc) : WriteLock(&desc) {}
			};

			/// <summary>
			/// Shader binding set descriptor
			/// </summary>
			class BindingSetDescriptor : public virtual Object {
			public:
				/// <summary>
				/// Information about a single shader binding
				/// </summary>
				struct BindingInfo {
					/// <summary> Pipeline stages, the binding ie visible in </summary>
					PipelineStageMask stages;

					/// <summary> Binding index </summary>
					uint32_t binding;
				};

				/// <summary> Virtual destructor </summary>
				inline virtual ~BindingSetDescriptor() {}


				/// <summary> 
				/// If true, the actual GPU resources provided by the descriptor will be considered as "Set by environment pipeline", 
				/// so the toolbox is going to ignore those and save some time (shader input description still matters, though).
				/// Note: Should stay the same throught the Object's lifecycle
				/// </summary>
				virtual bool SetByEnvironment()const = 0;


				/// <summary> Number of constant(uniform) buffers, available in the binding [Should stay the same throught the Object's lifecycle] </summary>
				virtual size_t ConstantBufferCount()const = 0;

				/// <summary>
				/// Constant buffer binding info by index [Should stay the same throught the Object's lifecycle]
				/// </summary>
				/// <param name="index"> Constant buffer binding index </param>
				/// <returns> Index'th constant buffer information </returns>
				virtual BindingInfo ConstantBufferInfo(size_t index)const = 0;

				/// <summary>
				/// Constant buffer by binding index
				/// </summary>
				/// <param name="index"> Constant buffer binding index </param>
				/// <returns> Index'th constant buffer </returns>
				virtual Reference<Buffer> ConstantBuffer(size_t index)const = 0;


				/// <summary> Number of structured(storage) buffers, available in the binding [Should stay the same throught the Object's lifecycle] </summary>
				virtual size_t StructuredBufferCount()const = 0;

				/// <summary>
				/// Structured buffer binding info by index [Should stay the same throught the Object's lifecycle]
				/// </summary>
				/// <param name="index"> Structured buffer binding index </param>
				/// <returns> Index'th structured buffer information </returns>
				virtual BindingInfo StructuredBufferInfo(size_t index)const = 0;

				/// <summary>
				/// Structured buffer by binding index
				/// </summary>
				/// <param name="index"> Structured buffer binding index </param>
				/// <returns> Index'th structured buffer </returns>
				virtual Reference<ArrayBuffer> StructuredBuffer(size_t index)const = 0;


				/// <summary> Number of texture samplers, available in the binding [Should stay the same throught the Object's lifecycle] </summary>
				virtual size_t TextureSamplerCount()const = 0;

				/// <summary>
				/// Texture sampler binding info by index [Should stay the same throught the Object's lifecycle]
				/// </summary>
				/// <param name="index"> Texture sampler binding index </param>
				/// <returns> Index'th texture sampler information </returns>
				virtual BindingInfo TextureSamplerInfo(size_t index)const = 0;

				/// <summary>
				/// Texture sampler by binding index
				/// </summary>
				/// <param name="index"> Texture sampler binding index </param>
				/// <returns> Index'th texture sampler </returns>
				virtual Reference<TextureSampler> Sampler(size_t index)const = 0;
			};

			/// <summary>  Number of binding sets, available to the pipeline </summary>
			virtual size_t BindingSetCount()const = 0;

			/// <summary>
			/// Binding set decriptor by index [Binding 'Shapes' should stay immutable throught pipeline's lifecycle, but the actual resources that are bound may change]
			/// </summary>
			/// <param name="index"> Binding set index </param>
			/// <returns> Index'th binding set </returns>
			virtual const BindingSetDescriptor* BindingSet(size_t index)const = 0;
		};


		/// <summary>
		/// Arbitrary GPU pipeline
		/// </summary>
		class Pipeline : public virtual Object {
		public:
			/// <summary>
			/// Information about a command buffer, the pipeline can execute on
			/// </summary>
			struct CommandBufferInfo {
				/// <summary> Command buffer to execute pipeline on </summary>
				CommandBuffer* commandBuffer;

				/// <summary> Index of the command buffer when we use, let's say, something like double or triple buffering </summary>
				size_t inFlightBufferId;

				/// <summary>
				/// Constructor
				/// </summary>
				/// <param name="buf"> Command buffer </param>
				/// <param name="bufferId"> Index of the command buffer </param>
				inline CommandBufferInfo(CommandBuffer* buf = nullptr, size_t bufferId = 0) : commandBuffer(buf), inFlightBufferId(bufferId) {}
			};


			/// <summary>
			/// Executes pipeline on the command buffer
			/// </summary>
			/// <param name="bufferInfo"> Command buffer and it's index </param>
			virtual void Execute(const CommandBufferInfo& bufferInfo) = 0;

			/// <summary>
			/// Executes pipeline on the command buffer
			/// </summary>
			/// <param name="commandBuffer"> Command buffer </param>
			/// <param name="inFlightBufferId"> Index of the command buffer (when we have something like double/triple/quadrouple/whatever buffering; otherwise should be 0) </param>
			inline void Execute(CommandBuffer* commandBuffer, size_t inFlightBufferId) { Execute(CommandBufferInfo(commandBuffer, inFlightBufferId)); }
		};
	}
}
