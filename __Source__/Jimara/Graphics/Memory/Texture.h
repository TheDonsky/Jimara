#pragma once
#include "../../Core/Object.h"
#include "../../Core/ThirdPartyHelpers/IncludeGLM.h"

namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Arbitrary texture object
		/// </summary>
		class Texture : public virtual Object {
		public:
			/// <summary> Texture type </summary>
			enum class TextureType : uint8_t {
				/// <summary> 1-dimensional image </summary>
				TEXTURE_1D,

				/// <summary> 2-dimensional image </summary>
				TEXTURE_2D,

				/// <summary> 3-dimensional image </summary>
				TEXTURE_3D
			};

			/// <summary> Type of the image </summary>
			virtual TextureType Type()const = 0;

			/// <summary> Image size (or array slice size) </summary>
			virtual glm::uvec3 Size()const = 0;

			/// <summary> Image array slice count </summary>
			virtual uint32_t ArraySize()const = 0;

			/// <summary> Mipmap level count </summary>
			virtual uint32_t MipLevels()const = 0;
		};
	}
}
