#pragma once
#include "../../Core/JimaraApi.h"
#include "../../Core/EnumClassBooleanOperands.h"
#include <stdint.h>
#include <iostream>

namespace Jimara {
	namespace Graphics {
		/// <summary> Pipeline stages </summary>
		enum class JIMARA_API PipelineStage : uint16_t {
			// No stage
			NONE = 0,

			// Compute shader
			COMPUTE = 1u,

			// Vertex shader
			VERTEX = (1u << 1u),

			// Fragment shader
			FRAGMENT = (1u << 2u),

			// Ray-Tracing ray-gen shader
			RAY_GENERATION = (1u << 3u),

			// Ray-Tracing miss shader
			RAY_MISS = (RAY_GENERATION << 1u),

			// Ray-Tracing any-hit shader
			RAY_ANY_HIT = (RAY_GENERATION << 2u),

			// Ray-Tracing closest-hit shader
			RAY_CLOSEST_HIT = (RAY_GENERATION << 3u),

			// Ray-Tracing intersection shader
			RAY_INTERSECTION = (RAY_GENERATION << 4u),

			// Callable shader
			CALLABLE = (RAY_GENERATION << 5u)
		};

		/// <summary> Pipeline stage bitmask </summary>
		using PipelineStageMask = PipelineStage;

		/// <summary> Define basic boolean operations </summary>
		JIMARA_DEFINE_ENUMERATION_BOOLEAN_OPERATIONS(PipelineStageMask);

		/// <summary>
		/// Prints-out PipelineStageMask as a concatenation of PipelineStage entries
		/// </summary>
		/// <param name="stream"> Generic output stream </param>
		/// <param name="stageMask"> Stage bitmask </param>
		/// <returns> stream </returns>
		inline static std::ostream& operator<<(std::ostream& stream, PipelineStageMask stageMask) {
			bool stagesFound = false;
			auto logStage = [&](PipelineStage stage, const char* name) {
				if ((stageMask & stage) == PipelineStageMask::NONE)
					return;
				if (stagesFound)
					stream << " | ";
				else stagesFound = true;
				stream << name;
			};

			logStage(PipelineStage::COMPUTE, "COMPUTE");
			logStage(PipelineStage::VERTEX, "VERTEX");
			logStage(PipelineStage::FRAGMENT, "FRAGMENT");
			logStage(PipelineStage::RAY_GENERATION, "RAY_GENERATION");
			logStage(PipelineStage::RAY_MISS, "RAY_MISS");
			logStage(PipelineStage::RAY_ANY_HIT, "RAY_ANY_HIT");
			logStage(PipelineStage::RAY_CLOSEST_HIT, "RAY_CLOSEST_HIT");
			logStage(PipelineStage::RAY_INTERSECTION, "RAY_INTERSECTION");
			logStage(PipelineStage::CALLABLE, "CALLABLE");

			if (!stagesFound)
				stream << "NONE";

			return stream;
		}
	}
}
