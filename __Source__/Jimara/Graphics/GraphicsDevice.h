#pragma once
namespace Jimara { namespace Graphics { class GraphicsDevice; } }
#include "../Core/Object.h"
#include "PhysicalDevice.h"
#include "Pipeline/GraphicsPipeline.h"
#include "Rendering/RenderEngine.h"
#include "Rendering/RenderSurface.h"

namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Logical graphics device
		/// </summary>
		class GraphicsDevice : public virtual Object {
		public:
			/// <summary> Virtual destructor </summary>
			virtual ~GraphicsDevice();

			/// <summary> Underlying physical device </summary>
			Graphics::PhysicalDevice* PhysicalDevice()const;

			/// <summary> "Owner" graphics instance </summary>
			Graphics::GraphicsInstance* GraphicsInstance()const;

			/// <summary> Logger </summary>
			OS::Logger* Log()const;

			/// <summary>
			/// Instantiates a render engine (Depending on the context/os etc only one per surface may be allowed)
			/// </summary>
			/// <param name="targetSurface"> Surface to render to </param>
			/// <returns> New instance of a render engine </returns>
			virtual Reference<RenderEngine> CreateRenderEngine(RenderSurface* targetSurface) = 0;

			/// <summary>
			/// Instantiates a shader cache object.
			/// Note: It's recomended to have a single shader cache per GraphicsDevice, but nobody's really judging you if you have a cache per shader...
			/// </summary>
			/// <returns> New shader cache instance </returns>
			virtual Reference<ShaderCache> CreateShaderCache() = 0;

			/// <summary>
			/// Creates an instance of a buffer that can be used as a constant buffer
			/// </summary>
			/// <param name="size"> Buffer size </param>
			/// <returns> New constant buffer </returns>
			virtual Reference<Buffer> CreateConstantBuffer(size_t size) = 0;

			/// <summary>
			/// Creates a constant buffer of the given type
			/// </summary>
			/// <typeparam name="CBufferType"> Type of the constant buffer </typeparam>
			/// <returns> New instance of a constant buffer </returns>
			template<typename CBufferType>
			inline BufferReference<CBufferType> CreateConstantBuffer() {
				return BufferReference<CBufferType>(CreateConstantBuffer(sizeof(CBufferType)));
			}

			/// <summary>
			/// Creates an array-type buffer of given size
			/// </summary>
			/// <param name="objectSize"> Individual element size </param>
			/// <param name="objectCount"> Element count within the buffer </param>
			/// <param name="cpuAccess"> CPU access flags </param>
			/// <returns> New instance of a buffer </returns>
			virtual Reference<ArrayBuffer> CreateArrayBuffer(size_t objectSize, size_t objectCount, ArrayBuffer::CPUAccess cpuAccess = ArrayBuffer::CPUAccess::CPU_WRITE_ONLY) = 0;

			/// <summary>
			/// Creates an array-type buffer of given size
			/// </summary>
			/// <typeparam name="Type"> Type of the array elements </typeparam>
			/// <param name="objectCount"> Element count within the buffer </param>
			/// <param name="cpuAccess"> CPU access flags </param>
			/// <returns> New instance of a buffer </returns>
			template<typename Type>
			inline BufferArrayReference<Type> CreateArrayBuffer(size_t objectCount, ArrayBuffer::CPUAccess cpuAccess = ArrayBuffer::CPUAccess::CPU_WRITE_ONLY) {
				return CreateArrayBuffer(sizeof(Type), objectCount, cpuAccess);
			}

			/// <summary>
			/// Creates an image texture
			/// </summary>
			/// <param name="type"> Texture type </param>
			/// <param name="format"> Texture format </param>
			/// <param name="size"> Texture size </param>
			/// <param name="arraySize"> Texture array slice count </param>
			/// <param name="generateMipmaps"> If true, image will generate mipmaps </param>
			/// <param name="type"> Texture type </param>
			/// <param name="cpuAccess"> CPU access flags </param>
			/// <returns> New instance of an ImageTexture object </returns>
			virtual Reference<ImageTexture> CreateTexture(
				Texture::TextureType type, Texture::PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps
				, ImageTexture::CPUAccess cpuAccess = ImageTexture::CPUAccess::CPU_WRITE_ONLY) = 0;


		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="physicalDevice"> Underlying physical device </param>
			GraphicsDevice(Graphics::PhysicalDevice* physicalDevice);

		private:
			// Underlying physical device
			Reference<Graphics::PhysicalDevice> m_physicalDevice;
		};
	}
}
