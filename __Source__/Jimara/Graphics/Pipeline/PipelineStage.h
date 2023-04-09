#pragma once
#include "../../Core/JimaraApi.h"
#include <stdint.h>

namespace Jimara {
	namespace Graphics {
		/// <summary> Pipeline stages </summary>
		enum class JIMARA_API PipelineStage : uint8_t {
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
		using PipelineStageMask = PipelineStage;

		/// <summary>
		/// 'Or' operator
		/// </summary>
		/// <param name="a"> First stage mask </param>
		/// <param name="b"> Second stage mask </param>
		/// <returns> a | b </returns>
		inline static constexpr PipelineStage operator|(PipelineStage a, PipelineStage b) {
			return static_cast<PipelineStage>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
		}

		/// <summary>
		/// Locically 'or'-s with the oher stage mask
		/// </summary>
		/// <param name="a"> First stage mask </param>
		/// <param name="b"> Second stage mask </param>
		/// <returns> a </returns>
		inline static PipelineStage& operator|=(PipelineStage& a, PipelineStage b) { return a = a | b; }

		/// <summary>
		/// 'And' operator
		/// </summary>
		/// <param name="a"> First stage mask </param>
		/// <param name="b"> Second stage mask </param>
		/// <returns> a & b </returns>
		inline static constexpr PipelineStage operator&(PipelineStage a, PipelineStage b) {
			return static_cast<PipelineStage>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
		}

		/// <summary>
		/// Locically 'and'-s with the oher stage mask
		/// </summary>
		/// <param name="a"> First stage mask </param>
		/// <param name="b"> Second stage mask </param>
		/// <returns> a </returns>
		inline static PipelineStage& operator&=(PipelineStage& a, PipelineStage b) { return a = a & b; }

		/// <summary>
		/// 'Xor' operator
		/// </summary>
		/// <param name="a"> First stage mask </param>
		/// <param name="b"> Second stage mask </param>
		/// <returns> a ^ b </returns>
		inline static constexpr PipelineStage operator^(PipelineStage a, PipelineStage b) {
			return static_cast<PipelineStage>(static_cast<uint8_t>(a) ^ static_cast<uint8_t>(b));
		}

		/// <summary>
		/// Locically 'xor'-s with the oher stage mask
		/// </summary>
		/// <param name="a"> First stage mask </param>
		/// <param name="b"> Second stage mask </param>
		/// <returns> a </returns>
		inline static PipelineStage& operator^=(PipelineStage& a, PipelineStage b) { return a = a ^ b; }

		/// <summary> Casts stage to Stage to StageMask </summary>
		inline static constexpr PipelineStageMask StageMask(PipelineStage stage) { return stage; }

		/// <summary>
		/// Makes a stage mask from stage list
		/// </summary>
		/// <typeparam name="...Stages"> Basically, some amount of repeating Stage keywords </typeparam>
		/// <param name="stage"> First stage </param>
		/// <param name="anotherStage"> Second stage </param>
		/// <param name="...rest"> Rest of the stages </param>
		/// <returns> stage | PipelineStageMask(rest...) </returns>
		template<typename... Stages>
		inline static constexpr PipelineStageMask StageMask(PipelineStage stage, PipelineStage anotherStage, Stages... rest) {
			return stage | StageMask(anotherStage, rest...);
		}
	}
}
