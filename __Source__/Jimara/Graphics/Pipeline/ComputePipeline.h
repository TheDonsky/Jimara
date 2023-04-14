#pragma once
namespace Jimara {
	namespace Graphics {
		namespace Legacy {
		class ComputePipeline;
		}
	}
}
#include "Pipeline.h"


namespace Jimara {
	namespace Graphics {
		namespace Legacy {
		/// <summary>
		/// Pipeline that executes compute kernels
		/// </summary>
		class JIMARA_API ComputePipeline : public virtual Pipeline {
		public:
			/// <summary>
			/// Compute pipeline descriptor
			/// </summary>
			class JIMARA_API Descriptor : public virtual PipelineDescriptor {
			public:
				/// <summary> Compute shader </summary>
				virtual Reference<Shader> ComputeShader()const = 0;

				/// <summary> Number of blocks to execute </summary>
				virtual Size3 NumBlocks()const = 0;
			};
		};
		}
	}
}
