#pragma once
#include "../../Core/Object.h"
#include "../../Math/Math.h"
#include "Buffers.h"

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
				TEXTURE_1D = 0,

				/// <summary> 2-dimensional image </summary>
				TEXTURE_2D = 1,

				/// <summary> 3-dimensional image </summary>
				TEXTURE_3D = 2,

				/// <summary> Not an actual type; denotes how many types there are </summary>
				TYPE_COUNT = 3
			};

			/// <summary>
			/// Pixel formtas
			/// </summary>
			enum class PixelFormat : uint8_t {
				/// <summary> Some pixel format that backend API defaulted to and is not exposed in Engine pixel formats </summary>
				OTHER = 0,

				/// <summary> Non-linear 8 bit single channel format (srgb) </summary>
				R8_SRGB = 1,

				/// <summary> Linear 8 bit single channel format </summary>
				R8_UNORM = 2,

				/// <summary> Non-linear 8 bit dual channel format (srgb) </summary>
				R8G8_SRGB = 3,

				/// <summary> Linear 8 bit dual channel format </summary>
				R8G8_UNORM = 4,

				/// <summary> Non-linear 8 bit triple channel format (srgb) </summary>
				R8G8B8_SRGB = 5,

				/// <summary> Linear 8 bit triple channel format </summary>
				R8G8B8_UNORM = 6,

				/// <summary> Non-linear 8 bit quad channel format (srgb) </summary>
				R8G8B8A8_SRGB = 7,

				/// <summary> Linear 8 bit quad channel format </summary>
				R8G8B8A8_UNORM = 8,

				/// <summary> Non-linear 16 bit single channel unsigned integer format </summary>
				R16_UINT = 9,

				/// <summary> Non-linear 16 bit single channel signed integer format </summary>
				R16_SINT = 10,

				/// <summary> Linear 16 bit single channel format </summary>
				R16_UNORM = 11,

				/// <summary> Unscaled 16 bit single channel floating point format </summary>
				R16_SFLOAT = 12,

				/// <summary> Non-linear 16 bit dual channel unsigned integer format </summary>
				R16G16_UINT = 13,

				/// <summary> Non-linear 16 bit dual channel signed integer format </summary>
				R16G16_SINT = 14,

				/// <summary> Linear 16 bit dual channel format </summary>
				R16G16_UNORM = 15,

				/// <summary> Unscaled 16 bit dual channel floating point format </summary>
				R16G16_SFLOAT = 16,

				/// <summary> Non-linear 16 bit triple channel unsigned integer format </summary>
				R16G16B16_UINT = 17,

				/// <summary> Non-linear 16 bit triple channel signed integer format </summary>
				R16G16B16_SINT = 18,

				/// <summary> Linear 16 bit triple channel format </summary>
				R16G16B16_UNORM = 19,

				/// <summary> Unscaled 16 bit triple channel floating point format </summary>
				R16G16B16_SFLOAT = 20,

				/// <summary> Non-linear 16 bit quad channel unsigned integer format </summary>
				R16G16B16A16_UINT = 21,

				/// <summary> Non-linear 16 bit quad channel signed integer format </summary>
				R16G16B16A16_SINT = 22,

				/// <summary> Linear 16 bit quad channel format </summary>
				R16G16B16A16_UNORM = 23,

				/// <summary> Unscaled 16 bit quad channel floating point format </summary>
				R16G16B16A16_SFLOAT = 24,

				/// <summary> Unscaled 32 bit single channel unsigned integer format </summary>
				R32_UINT = 25,

				/// <summary> Unscaled 32 bit single channel signed integer format </summary>
				R32_SINT = 26,

				/// <summary> Unscaled 32 bit single channel floating point format </summary>
				R32_SFLOAT = 27,

				/// <summary> Unscaled 32 bit dual channel unsigned integer format </summary>
				R32G32_UINT = 28,

				/// <summary> Unscaled 32 bit dual channel signed integer format </summary>
				R32G32_SINT = 29,

				/// <summary> Unscaled 32 bit dual channel floating point format </summary>
				R32G32_SFLOAT = 30,

				/// <summary> Unscaled 32 bit triple channel unsigned integer format </summary>
				R32G32B32_UINT = 31,

				/// <summary> Unscaled 32 bit triple channel signed integer format </summary>
				R32G32B32_SINT = 32,

				/// <summary> Unscaled 32 bit triple channel floating point format </summary>
				R32G32B32_SFLOAT = 33,

				/// <summary> Unscaled 32 bit quad channel unsigned integer format </summary>
				R32G32B32A32_UINT = 34,

				/// <summary> Unscaled 32 bit quad channel signed integer format </summary>
				R32G32B32A32_SINT = 35,

				/// <summary> Unscaled 32 bit quad channel floating point format </summary>
				R32G32B32A32_SFLOAT = 36,

				/// <summary> First depth format </summary>
				FIRST_DEPTH_FORMAT = 37,

				/// <summary> 32 bit floating point depth buffer format </summary>
				D32_SFLOAT = 37,

				/// <summary> First depth and stencil format </summary>
				FIRST_DEPTH_AND_STENCIL_FORMAT = 38,

				/// <summary> 32 bit floating point depth buffer + 8 bit stencil buffer </summary>
				D32_SFLOAT_S8_UINT = 38,

				/// <summary> 24 bit depth buffer + 8 bit stencil buffer </summary>
				D24_UNORM_S8_UINT = 39,

				/// <summary> Last depth and stencil format </summary>
				LAST_DEPTH_AND_STENCIL_FORMAT = 39,
				
				/// <summary> Last depth format </summary>
				LAST_DEPTH_FORMAT = 39,

				/// <summary> Not an actual format; tells, how many different formats reside in enumeration </summary>
				FORMAT_COUNT = 40
			};

			/// <summary> Type of the image </summary>
			virtual TextureType Type()const = 0;
			
			/// <summary> Pixel format of the image </summary>
			virtual PixelFormat ImageFormat()const = 0;

			/// <summary> Image size (or array slice size) </summary>
			virtual Size3 Size()const = 0;

			/// <summary> Image array slice count </summary>
			virtual uint32_t ArraySize()const = 0;

			/// <summary> Mipmap level count </summary>
			virtual uint32_t MipLevels()const = 0;
		};



		/// <summary>
		/// A texture that can be written to by CPU and can generate it's own mipmaps 
		/// (supposed to be used for texture assets; generally has only one layer, is 2-dimensional and has a known format)
		/// </summary>
		class ImageTexture : public virtual Texture {
		public:
			/// <summary> CPU access flags </summary>
			typedef Buffer::CPUAccess CPUAccess;

			/// <summary> CPU access info </summary>
			virtual CPUAccess HostAccess()const = 0;

			/// <summary>
			/// Maps texture memory to CPU
			/// Notes:
			///		0. Each Map call should be accompanied by corresponding Unmap() and it's a bad idea to call additional Map()s in between;
			///		1. Depending on the CPUAccess flag used during texture creation(or texture type when CPUAccess does not apply), 
			///		 the actual content of the texture will or will not be present in mapped memory.
			/// </summary>
			/// <returns> Mapped memory </returns>
			virtual void* Map() = 0;

			/// <summary>
			/// Unmaps memory previously mapped via Map() call
			/// </summary>
			/// <param name="write"> If true, the system will understand that the user modified mapped memory and update the content on GPU </param>
			virtual void Unmap(bool write) = 0;
		};
	}
}
