#pragma once
namespace Jimara {
	namespace Graphics {
		class Texture;
		class ImageTexture;
		class TextureView;
		class TextureSampler;

		// Forward declaration for GraphicsDevice and CommandBuffer (to avoid circular dependencies...)
		class GraphicsDevice;
		class CommandBuffer;
	}
}
#include "../../Core/Object.h"
#include "../../Math/Math.h"
#include "../../OS/IO/Path.h"
#include "../../Data/AssetDatabase/AssetDatabase.h"
#include "Buffers.h"
#include <string_view>
#include <string>

namespace Jimara {
	namespace Graphics {
		/// <summary> Texture sampler </summary>
		class TextureSampler : public virtual Object {
		public:
			/// <summary> Image filtering mode </summary>
			enum class FilteringMode : uint8_t {
				/// <summary> No interpolation </summary>
				NEAREST = 0,

				/// <summary> Linear interpolation </summary>
				LINEAR = 1,

				/// <summary> Number of possible filtering modes </summary>
				FILTER_COUNT = 2
			};

			/// <summary> Tells, how the image outside the bounds is sampled </summary>
			enum class WrappingMode : uint8_t {
				/// <summary> Repeat pattern </summary>
				REPEAT = 0,

				/// <summary> Repeat patern with mirrored images </summary>
				MIRRORED_REPEAT = 1,

				/// <summary> Keep closest edge color </summary>
				CLAMP_TO_EDGE = 2,

				/// <summary> Black outside boundaries </summary>
				CLAMP_TO_BORDER = 3,

				/// <summary> Number of possible wrapping modes </summary>
				MODE_COUNT = 4
			};

			/// <summary> Image filtering mode </summary>
			virtual FilteringMode Filtering()const = 0;

			/// <summary> Tells, how the image outside the bounds is sampled </summary>
			virtual WrappingMode Wrapping()const = 0;

			/// <summary> Lod bias </summary>
			virtual float LodBias()const = 0;

			/// <summary> Texture view, this sampler "belongs" to </summary>
			virtual TextureView* TargetView()const = 0;
		};


		/// <summary> View to a texture </summary>
		class TextureView : public virtual Object {
		public:
			/// <summary> Possible view types </summary>
			enum class ViewType : uint8_t {
				/// <summary> Access as 1D texture </summary>
				VIEW_1D = 0,

				/// <summary> Access as 2D texture </summary>
				VIEW_2D = 1,

				/// <summary> Access as 3D texture </summary>
				VIEW_3D = 2,

				/// <summary> Access as cubemap </summary>
				VIEW_CUBE = 3,

				/// <summary> Access as 1D texture array </summary>
				VIEW_1D_ARRAY = 4,

				/// <summary> Access as 2D texture array </summary>
				VIEW_2D_ARRAY = 5,

				/// <summary> Access as cubemap texture array </summary>
				VIEW_CUBE_ARRAY = 6,

				/// <summary> Number of availabe view types  </summary>
				TYPE_COUNT = 7
			};

			/// <summary> Type of the view </summary>
			virtual ViewType Type()const = 0;

			/// <summary> Texture, this view belongs to </summary>
			virtual Texture* TargetTexture()const = 0;

			/// <summary> Base mip level </summary>
			virtual uint32_t BaseMipLevel()const = 0;

			/// <summary> Number of view mip levels </summary>
			virtual uint32_t MipLevelCount()const = 0;

			/// <summary> Base array slice </summary>
			virtual uint32_t BaseArrayLayer()const = 0;

			/// <summary> Number of view array slices </summary>
			virtual uint32_t ArrayLayerCount()const = 0;
			
			/// <summary>
			/// Creates an image sampler
			/// </summary>
			/// <param name="filtering"> Image filtering mode </param>
			/// <param name="wrapping"> Tells, how the image outside the bounds is sampled </param>
			/// <param name="lodBias"> Lod bias </param>
			/// <returns> New instance of a texture sampler </returns>
			virtual Reference<TextureSampler> CreateSampler(
				TextureSampler::FilteringMode filtering = TextureSampler::FilteringMode::LINEAR
				, TextureSampler::WrappingMode wrapping = TextureSampler::WrappingMode::REPEAT
				, float lodBias = 0) = 0;
		};

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

				/// <summary> Non-linear 8 bit triple channel format (bgr order; srgb) </summary>
				B8G8R8_SRGB = 7,

				/// <summary> Linear 8 bit triple channel format (bgr order) </summary>
				B8G8R8_UNORM = 8,

				/// <summary> Non-linear 8 bit quad channel format (srgb) </summary>
				R8G8B8A8_SRGB = 9,

				/// <summary> Linear 8 bit quad channel format </summary>
				R8G8B8A8_UNORM = 10,

				/// <summary> Non-linear 8 bit triple channel format (bgra order; srgb) </summary>
				B8G8R8A8_SRGB = 11,

				/// <summary> Linear 8 bit triple channel format (bgra order) </summary>
				B8G8R8A8_UNORM = 12,

				/// <summary> Non-linear 16 bit single channel unsigned integer format </summary>
				R16_UINT = 13,

				/// <summary> Non-linear 16 bit single channel signed integer format </summary>
				R16_SINT = 14,

				/// <summary> Linear 16 bit single channel format </summary>
				R16_UNORM = 15,

				/// <summary> Unscaled 16 bit single channel floating point format </summary>
				R16_SFLOAT = 16,

				/// <summary> Non-linear 16 bit dual channel unsigned integer format </summary>
				R16G16_UINT = 17,

				/// <summary> Non-linear 16 bit dual channel signed integer format </summary>
				R16G16_SINT = 18,

				/// <summary> Linear 16 bit dual channel format </summary>
				R16G16_UNORM = 19,

				/// <summary> Unscaled 16 bit dual channel floating point format </summary>
				R16G16_SFLOAT = 20,

				/// <summary> Non-linear 16 bit triple channel unsigned integer format </summary>
				R16G16B16_UINT = 21,

				/// <summary> Non-linear 16 bit triple channel signed integer format </summary>
				R16G16B16_SINT = 22,

				/// <summary> Linear 16 bit triple channel format </summary>
				R16G16B16_UNORM = 23,

				/// <summary> Unscaled 16 bit triple channel floating point format </summary>
				R16G16B16_SFLOAT = 24,

				/// <summary> Non-linear 16 bit quad channel unsigned integer format </summary>
				R16G16B16A16_UINT = 25,

				/// <summary> Non-linear 16 bit quad channel signed integer format </summary>
				R16G16B16A16_SINT = 26,

				/// <summary> Linear 16 bit quad channel format </summary>
				R16G16B16A16_UNORM = 27,

				/// <summary> Unscaled 16 bit quad channel floating point format </summary>
				R16G16B16A16_SFLOAT = 28,

				/// <summary> Unscaled 32 bit single channel unsigned integer format </summary>
				R32_UINT = 29,

				/// <summary> Unscaled 32 bit single channel signed integer format </summary>
				R32_SINT = 30,

				/// <summary> Unscaled 32 bit single channel floating point format </summary>
				R32_SFLOAT = 31,

				/// <summary> Unscaled 32 bit dual channel unsigned integer format </summary>
				R32G32_UINT = 32,

				/// <summary> Unscaled 32 bit dual channel signed integer format </summary>
				R32G32_SINT = 33,

				/// <summary> Unscaled 32 bit dual channel floating point format </summary>
				R32G32_SFLOAT = 34,

				/// <summary> Unscaled 32 bit triple channel unsigned integer format </summary>
				R32G32B32_UINT = 35,

				/// <summary> Unscaled 32 bit triple channel signed integer format </summary>
				R32G32B32_SINT = 36,

				/// <summary> Unscaled 32 bit triple channel floating point format </summary>
				R32G32B32_SFLOAT = 37,

				/// <summary> Unscaled 32 bit quad channel unsigned integer format </summary>
				R32G32B32A32_UINT = 38,

				/// <summary> Unscaled 32 bit quad channel signed integer format </summary>
				R32G32B32A32_SINT = 39,

				/// <summary> Unscaled 32 bit quad channel floating point format </summary>
				R32G32B32A32_SFLOAT = 40,

				/// <summary> First depth format </summary>
				FIRST_DEPTH_FORMAT = 41,

				/// <summary> 32 bit floating point depth buffer format </summary>
				D32_SFLOAT = 41,

				/// <summary> First depth and stencil format </summary>
				FIRST_DEPTH_AND_STENCIL_FORMAT = 42,

				/// <summary> 32 bit floating point depth buffer + 8 bit stencil buffer </summary>
				D32_SFLOAT_S8_UINT = 42,

				/// <summary> 24 bit depth buffer + 8 bit stencil buffer </summary>
				D24_UNORM_S8_UINT = 43,

				/// <summary> Last depth and stencil format </summary>
				LAST_DEPTH_AND_STENCIL_FORMAT = 43,
				
				/// <summary> Last depth format </summary>
				LAST_DEPTH_FORMAT = 43,

				/// <summary> Not an actual format; tells, how many different formats reside in enumeration </summary>
				FORMAT_COUNT = 44
			};

			/// <summary> Sample count for multisampling </summary>
			enum class Multisampling : uint8_t {
				// No multisampling
				SAMPLE_COUNT_1 = 1,

				// MSAA 2
				SAMPLE_COUNT_2 = 2,

				// MSAA 4
				SAMPLE_COUNT_4 = 4,

				// MSAA 8
				SAMPLE_COUNT_8 = 8,

				// MSAA 16
				SAMPLE_COUNT_16 = 16,

				// MSAA 32
				SAMPLE_COUNT_32 = 32,

				// MSAA 64
				SAMPLE_COUNT_64 = 64,

				// Maximal sample count supported by the device
				MAX_AVAILABLE = (uint8_t)~((uint8_t)0)
			};

			/// <summary> Type of the image </summary>
			virtual TextureType Type()const = 0;
			
			/// <summary> Pixel format of the image </summary>
			virtual PixelFormat ImageFormat()const = 0;

			/// <summary> Sample count for multisampling </summary>
			virtual Multisampling SampleCount()const = 0;

			/// <summary> Image size (or array slice size) </summary>
			virtual Size3 Size()const = 0;

			/// <summary> Image array slice count </summary>
			virtual uint32_t ArraySize()const = 0;

			/// <summary> Mipmap level count </summary>
			virtual uint32_t MipLevels()const = 0;

			/// <summary>
			/// Creates an image view
			/// </summary>
			/// <param name="type"> View type </param>
			/// <param name="baseMipLevel"> Base mip level (default 0) </param>
			/// <param name="mipLevelCount"> Number of mip levels (default is all) </param>
			/// <param name="baseArrayLayer"> Base array slice (default 0) </param>
			/// <param name="arrayLayerCount"> Number of array slices (default is all) </param>
			/// <returns> A new instance of an image view </returns>
			virtual Reference<TextureView> CreateView(TextureView::ViewType type
				, uint32_t baseMipLevel = 0, uint32_t mipLevelCount = ~((uint32_t)0u)
				, uint32_t baseArrayLayer = 0, uint32_t arrayLayerCount = ~((uint32_t)0u)) = 0;

			/// <summary>
			/// "Blits"/Copies data from a region of some other buffer to a part of this one
			/// </summary>
			/// <param name="commandBuffer"> Command buffer to record operation on </param>
			/// <param name="srcTexture"> Source texture </param>
			/// <param name="dstRegion"> Region to copy to </param>
			/// <param name="srcRegion"> Source region to copy from </param>
			virtual void Blit(CommandBuffer* commandBuffer, Texture* srcTexture, 
				const SizeAABB& dstRegion = SizeAABB(Size3(0u), Size3(~static_cast<uint32_t>(0u))), 
				const SizeAABB& srcRegion = SizeAABB(Size3(0u), Size3(~static_cast<uint32_t>(0u)))) = 0;
		};


#pragma warning(disable: 4250)
		/// <summary>
		/// A texture that can be written to by CPU and can generate it's own mipmaps 
		/// (supposed to be used for texture assets; generally has only one layer, is 2-dimensional and has a known format)
		/// </summary>
		class ImageTexture : public virtual Texture, public virtual Resource {
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

			/// <summary>
			/// Loads texture from file
			/// </summary>
			/// <param name="device"> Graphics device to base texture on </param>
			/// <param name="filename"> Path to the file to load from </param>
			/// <param name="createMipmaps"> If true, texture will generate mipmaps </param>
			/// <returns> Texture, if found, nullptr otherwise </returns>
			static Reference<ImageTexture> LoadFromFile(GraphicsDevice* device, const OS::Path& filename, bool createMipmaps);
		};
#pragma warning(default: 4250)
	}

	// Parent type definition for the resource
	template<> inline void TypeIdDetails::GetParentTypesOf<Graphics::ImageTexture>(const Callback<TypeId>& report) {
		report(TypeId::Of<Graphics::Texture>());
		report(TypeId::Of<Resource>()); 
	}
}
