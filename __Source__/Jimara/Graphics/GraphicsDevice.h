#pragma once
namespace Jimara { namespace Graphics { class GraphicsDevice; } }
#include "../Core/Object.h"
#include "PhysicalDevice.h"
#include "Pipeline/BindlessSet.h"
#include "Pipeline/RenderPass.h"
#include "Pipeline/DeviceQueue.h"
#include "Pipeline/IndirectBuffers.h"
#include "Pipeline/ComputePipeline.h"
#include "Pipeline/GraphicsPipeline.h"
#include "Pipeline/Experimental/Pipeline.h"
#include "Rendering/RenderEngine.h"
#include "Rendering/RenderSurface.h"
#include "Data/ShaderBinaries/SPIRV_Binary.h"


namespace Jimara {
	namespace Graphics {
		/// <summary>
		/// Logical graphics device
		/// </summary>
		class JIMARA_API GraphicsDevice : public virtual Object {
		public:
			/// <summary> Virtual destructor </summary>
			virtual ~GraphicsDevice();

			/// <summary> Underlying physical device </summary>
			Graphics::PhysicalDevice* PhysicalDevice()const;

			/// <summary> "Owner" graphics instance </summary>
			Graphics::GraphicsInstance* GraphicsInstance()const;

			/// <summary> Logger </summary>
			OS::Logger* Log()const;

			/// <summary> Access to main graphics queue </summary>
			virtual DeviceQueue* GraphicsQueue()const = 0;

			/// <summary>
			/// Instantiates a render engine (Depending on the context/os etc only one per surface may be allowed)
			/// </summary>
			/// <param name="targetSurface"> Surface to render to </param>
			/// <returns> New instance of a render engine </returns>
			virtual Reference<RenderEngine> CreateRenderEngine(RenderSurface* targetSurface) = 0;

			/// <summary>
			/// Instantiates a shader module
			/// <para/> Note: Generally speaking, it's recommended to use shader cache for effective shader reuse.
			/// </summary>
			/// <param name="bytecode"> SPIR-V bytecode </param>
			/// <returns> New instance of a shader module </returns>
			virtual Reference<Shader> CreateShader(const SPIRV_Binary* bytecode) = 0;

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
			inline ArrayBufferReference<Type> CreateArrayBuffer(size_t objectCount, ArrayBuffer::CPUAccess cpuAccess = ArrayBuffer::CPUAccess::CPU_WRITE_ONLY) {
				return CreateArrayBuffer(sizeof(Type), objectCount, cpuAccess);
			}

			/// <summary>
			/// Creates an indirect draw buffer of given size
			/// </summary>
			/// <param name="objectCount"> Number of elements within the indirect draw buffer </param>
			/// <param name="cpuAccess"> CPU access flags </param>
			/// <returns> A new instance of an indirect draw buffer </returns>
			virtual IndirectDrawBufferReference CreateIndirectDrawBuffer(size_t objectCount, ArrayBuffer::CPUAccess cpuAccess = ArrayBuffer::CPUAccess::CPU_WRITE_ONLY) = 0;

			/// <summary>
			/// Creates an image texture
			/// </summary>
			/// <param name="type"> Texture type </param>
			/// <param name="format"> Texture format </param>
			/// <param name="size"> Texture size </param>
			/// <param name="arraySize"> Texture array slice count </param>
			/// <param name="generateMipmaps"> If true, image will generate mipmaps </param>
			/// <param name="type"> Texture type </param>
			/// <returns> New instance of an ImageTexture object </returns>
			virtual Reference<ImageTexture> CreateTexture(
				Texture::TextureType type, Texture::PixelFormat format, Size3 size, uint32_t arraySize, bool generateMipmaps) = 0;

			/// <summary>
			/// Creates a multisampled texture for color/depth attachments
			/// </summary>
			/// <param name="type"> Texture type </param>
			/// <param name="format"> Texture format </param>
			/// <param name="size"> Texture size </param>
			/// <param name="arraySize"> Texture array slice count </param>
			/// <param name="sampleCount"> Desired multisampling (if the device does not support this amount, some lower number may be chosen) </param>
			/// <returns> New instance of a multisampled texture </returns>
			virtual Reference<Texture> CreateMultisampledTexture(
				Texture::TextureType type, Texture::PixelFormat format, Size3 size, uint32_t arraySize, Texture::Multisampling sampleCount) = 0;

			/// <summary>
			/// Creates a texture, that can be read from CPU
			/// </summary>
			/// <param name="type"> Texture type </param>
			/// <param name="format"> Pixel format </param>
			/// <param name="size"> Texture size </param>
			/// <param name="arraySize"> Texture array slice count </param>
			/// <returns> New instance of a texture with ArrayBuffer::CPUAccess::CPU_READ_WRITE flags </returns>
			virtual Reference<ImageTexture> CreateCpuReadableTexture(
				Texture::TextureType type, Texture::PixelFormat format, Size3 size, uint32_t arraySize) = 0;

			/// <summary> Selects a depth format supported by the device (there may be more than one in actuality, but this picks one of them by prefference) </summary>
			virtual Texture::PixelFormat GetDepthFormat() = 0;

			/// <summary> Creates a new instance of a bindless set of ArrayBuffer objects </summary>
			virtual Reference<BindlessSet<ArrayBuffer>> CreateArrayBufferBindlessSet() = 0;

			/// <summary> Creates a new instance of a bindless set of texture samplers </summary>
			virtual Reference<BindlessSet<TextureSampler>> CreateTextureSamplerBindlessSet() = 0;

			/// <summary>
			/// Creates a render pass or returns previously created pass with compatible layout
			/// </summary>
			/// <param name="sampleCount"> "MSAA" </param>
			/// <param name="numColorAttachments"> Color attachment count </param>
			/// <param name="colorAttachmentFormats"> Pixel format per color attachment </param>
			/// <param name="depthFormat"> Depth format (if value is outside [FIRST_DEPTH_FORMAT; LAST_DEPTH_FORMAT] range, the render pass will not have a depth format) </param>
			/// <param name="flags"> Clear and resolve flags </param>
			/// <returns> Shared instance of a render pass </returns>
			virtual Reference<RenderPass> GetRenderPass(
				Texture::Multisampling sampleCount, 
				size_t numColorAttachments, const Texture::PixelFormat* colorAttachmentFormats, 
				Texture::PixelFormat depthFormat, 
				RenderPass::Flags flags) = 0;

			/// <summary>
			/// Creates an environment pipeline
			/// </summary>
			/// <param name="descriptor"> Environment pipeline descriptor </param>
			/// <param name="maxInFlightCommandBuffers"> Maximal number of in-flight command buffers that may be using the pipeline at the same time </param>
			/// <returns> New instance of an environment pipeline object </returns>
			virtual Reference<Pipeline> CreateEnvironmentPipeline(PipelineDescriptor* descriptor, size_t maxInFlightCommandBuffers) = 0;

			/// <summary>
			/// Creates a compute pipeline
			/// </summary>
			/// <param name="descriptor"> Compute pipeline descriptor </param>
			/// <param name="maxInFlightCommandBuffers"> Maximal number of in-flight command buffers that may be using the pipeline at the same time </param>
			/// <returns> New instance of a compute pipeline object </returns>
			virtual Reference<ComputePipeline> CreateComputePipeline(ComputePipeline::Descriptor* descriptor, size_t maxInFlightCommandBuffers) = 0;

			/// <summary>
			/// Gets cached instance of a compute pipeline
			/// </summary>
			/// <param name="computeShader"> Compute shader bytecode </param>
			/// <returns> Pipeline instance </returns>
			virtual Reference<Experimental::ComputePipeline> GetComputePipeline(const SPIRV_Binary* computeShader) = 0;

			/// <summary>
			/// Creates new binding pool
			/// </summary>
			/// <param name="inFlightCommandBufferCount"> Number of in-flight binding copies per binding set allocated from the pool </param>
			/// <returns> New instance of a binding pool </returns>
			virtual Reference<Experimental::BindingPool> CreateBindingPool(size_t inFlightCommandBufferCount) = 0;


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
