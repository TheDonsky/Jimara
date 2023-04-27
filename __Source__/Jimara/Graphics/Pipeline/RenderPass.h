#pragma once
namespace Jimara { namespace Graphics { class RenderPass; } }
#include "FrameBuffer.h"
#include "CommandBuffer.h"
#include "GraphicsPipeline.h"
#include "Experimental/Pipeline.h"
#include "../../Core/Collections/Stacktor.h"
#include <stdint.h>


namespace Jimara { 
	namespace Graphics {
		/// <summary>
		/// Render pass
		/// Note: Mainly defines the shape of a frame buffer.
		/// </summary>
		class JIMARA_API RenderPass : public virtual Object {
		public:
			/// <summary> Render pass flags </summary>
			enum class JIMARA_API Flags : uint8_t {
				/// <summary> 'Empty' flag; does nothing </summary>
				NONE = 0,

				/// <summary> If set, color attachments will be cleared </summary>
				CLEAR_COLOR = (1 << 0),

				/// <summary> If set, depth attachments will be cleared (ignored, if depth buffer is not present) </summary>
				CLEAR_DEPTH = (1 << 1),

				/// <summary> 
				/// If set, color attachments will be resolved 
				/// (color resolve attachments will be required; ignored, if multisampling set to SAMPLE_COUNT_1) 
				/// </summary>
				RESOLVE_COLOR = (1 << 2),

				/// <summary> 
				/// If set, depth attachments will be resolved 
				/// (depth resolve attachment will be required; ignored, if multisampling set to SAMPLE_COUNT_1 or depth buffer is not present) 
				/// </summary>
				RESOLVE_DEPTH = (1 << 3)
			};

			/// <summary> "Owner" graphics device </summary>
			virtual GraphicsDevice* Device()const = 0;

			/// <summary>
			/// Creates a frame buffer based on given attachments
			/// Note: Array sizes should be as defined by the render pass itself, so they are not received here
			/// </summary>
			/// <param name="colorAttachments"> Color attachments (can and should be multisampled if the render pass is set up for msaa) </param>
			/// <param name="depthAttachment"> Depth attachments (can and should be multisampled if the render pass is set up for msaa) </param>
			/// <param name="colorResolveAttachments"> 
			/// Resolve attachments for colorAttachments (should not be multisampled; ignored, if the render pass is not multisampled or does not have RESOLVE_COLOR flag set) 
			/// </param>
			/// <param name="depthResolveAttachment"> 
			/// Resolve attachments for depthAttachment (should not be multisampled; ignored, if the render pass is not multisampled or does not have RESOLVE_DEPTH flag set) 
			/// </param>
			/// <returns> New instance of a frame buffer </returns>
			virtual Reference<FrameBuffer> CreateFrameBuffer(
				Reference<TextureView>* colorAttachments, Reference<TextureView> depthAttachment, 
				Reference<TextureView>* colorResolveAttachments, Reference<TextureView> depthResolveAttachment) = 0;

			/// <summary>
			/// Creates or retrieves a cached instance of a graphics pipeline based on the shaders and vertex input configuration
			/// </summary>
			/// <param name="descriptor"> Graphics pipeline descriptor </param>
			/// <returns> Instance of a pipeline </returns>
			virtual Reference<GraphicsPipeline> GetGraphicsPipeline(const GraphicsPipeline::Descriptor& descriptor) = 0;

			/// <summary>
			/// Begins render pass on the command buffer
			/// </summary>
			/// <param name="commandBuffer"> Command buffer to begin pass on </param>
			/// <param name="frameBuffer"> Frame buffer for the render pass </param>
			/// <param name="clearValues"> Clear values for the color attachments (Ignored, if the pass was created with 'false' for 'clearColor' argument) </param>
			/// <param name="renderWithSecondaryCommandBuffers"> If true, the render pass contents should be recorded using secondary command buffers </param>
			virtual void BeginPass(CommandBuffer* commandBuffer, FrameBuffer* frameBuffer, const Vector4* clearValues, bool renderWithSecondaryCommandBuffers = false) = 0;

			/// <summary>
			/// Ends render pass on the command buffer
			/// </summary>
			/// <param name="commandBuffer"> Command buffer to end the render pass on </param>
			virtual void EndPass(CommandBuffer* commandBuffer) = 0;

			/// <summary> Render pass clear/resolve flags </summary>
			inline Flags PassFlags()const { return m_flags; }

			/// <summary>
			/// Checks if all given flags are present
			/// </summary>
			/// <param name="flags"> Flags to check </param>
			/// <returns> True, if and only if all bits from given flags are set </returns>
			inline bool HasFlags(Flags flags)const { return static_cast<Flags>(static_cast<uint8_t>(m_flags) & static_cast<uint8_t>(flags)) == flags; }

			/// <summary> Checks if CLEAR_COLOR flag is present </summary>
			inline bool ClearsColor()const { return HasFlags(Flags::CLEAR_COLOR); }

			/// <summary> Checks if CLEAR_DEPTH flag is present </summary>
			inline bool ClearsDepth()const { return HasFlags(Flags::CLEAR_DEPTH); }

			/// <summary> Checks if RESOLVE_COLOR flag is present </summary>
			inline bool ResolvesColor()const { return HasFlags(Flags::RESOLVE_COLOR); }

			/// <summary> Checks if RESOLVE_DEPTH flag is present </summary>
			inline bool ResolvesDepth()const { return HasFlags(Flags::RESOLVE_DEPTH); }

			/// <summary> MSAA </summary>
			inline Texture::Multisampling SampleCount()const { return m_sampleCount; }

			/// <summary> Number of render pass color attachments </summary>
			inline size_t ColorAttachmentCount()const { return m_colorAttachmentFormats.Size(); }

			/// <summary>
			/// Color attachment format by index
			/// </summary>
			/// <param name="index"> Attachment index </param>
			/// <returns> Color attachment index </returns>
			inline Texture::PixelFormat ColorAttachmentFormat(size_t index)const { return m_colorAttachmentFormats[index]; }

			/// <summary>
			/// Checks if given texture format can be used as a depth format
			/// </summary>
			/// <param name="depthFormat"> Pixel format to check </param>
			/// <returns> True, if depthFormat if between FIRST_DEPTH_FORMAT and LAST_DEPTH_FORMAT </returns>
			inline static constexpr bool IsValidDepthFormat(Texture::PixelFormat depthFormat) {
				return (depthFormat >= Texture::PixelFormat::FIRST_DEPTH_FORMAT && depthFormat <= Texture::PixelFormat::LAST_DEPTH_FORMAT);
			}

			/// <summary> Tells, if the render pass uses depth attachment or not </summary>
			inline bool HasDepthAttachment()const { return IsValidDepthFormat(m_depthAttachmentFormat); }

			/// <summary> Supported depth attachment format if present, Texture::PixelFormat::FORMAT_COUNT otherwise </summary>
			inline Texture::PixelFormat DepthAttachmentFormat()const { return m_depthAttachmentFormat; }


		protected:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="flags"> Flags </param>
			/// <param name="sampleCount"> MSAA </param>
			/// <param name="numColorAttachments"> Color attachment count </param>
			/// <param name="colorAttachmentFormats"> Pixel format per color attachment </param>
			/// <param name="depthFormat"> Depth format (if value is outside [FIRST_DEPTH_FORMAT; LAST_DEPTH_FORMAT] range, the render pass will not have a depth format) </param>
			inline RenderPass(
				Flags flags, Texture::Multisampling sampleCount,
				size_t numColorAttachments, const Texture::PixelFormat* colorAttachmentFormats, 
				Texture::PixelFormat depthFormat)
				: m_flags([&]() -> Flags {
				uint8_t f = static_cast<uint8_t>(flags);
				if (!IsValidDepthFormat(depthFormat))
					f &= (~(static_cast<uint8_t>(RenderPass::Flags::CLEAR_DEPTH) | static_cast<uint8_t>(RenderPass::Flags::RESOLVE_DEPTH)));
				return static_cast<Flags>(f);
					}())
				, m_sampleCount(sampleCount)
				, m_colorAttachmentFormats(colorAttachmentFormats, numColorAttachments)
				, m_depthAttachmentFormat(IsValidDepthFormat(depthFormat) ? depthFormat : Texture::PixelFormat::FORMAT_COUNT) {}

		private:
			// Flags
			const Flags m_flags;

			// MSAA
			const Texture::Multisampling m_sampleCount;

			// Color attachment formats
			const Stacktor<Texture::PixelFormat, 4> m_colorAttachmentFormats;

			// Depth attachment format
			const Texture::PixelFormat m_depthAttachmentFormat;
		};


		/// <summary> Logical OR between two RenderPass::Flags </summary>
		inline constexpr RenderPass::Flags operator|(RenderPass::Flags a, RenderPass::Flags b) { 
			return static_cast<RenderPass::Flags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
		}

		/// <summary> Sets to logical OR between two RenderPass::Flags </summary>
		inline constexpr RenderPass::Flags& operator|=(RenderPass::Flags& a, RenderPass::Flags b) { return a = a | b; }

		/// <summary> Logical AND between two RenderPass::Flags </summary>
		inline constexpr RenderPass::Flags operator&(RenderPass::Flags a, RenderPass::Flags b) {
			return static_cast<RenderPass::Flags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
		}

		/// <summary> Sets to logical AND between two RenderPass::Flags </summary>
		inline constexpr RenderPass::Flags& operator&=(RenderPass::Flags& a, RenderPass::Flags b) { return a = a & b; }

		/// <summary> Logical XOR between two RenderPass::Flags </summary>
		inline constexpr RenderPass::Flags operator^(RenderPass::Flags a, RenderPass::Flags b) {
			return static_cast<RenderPass::Flags>(static_cast<uint8_t>(a) ^ static_cast<uint8_t>(b));
		}

		/// <summary> Sets to logical XOR between two RenderPass::Flags </summary>
		inline constexpr RenderPass::Flags& operator^=(RenderPass::Flags& a, RenderPass::Flags b) { return a = a ^ b; }

		/// <summary> Logical negation of RenderPass::Flags </summary>
		inline constexpr RenderPass::Flags operator~(RenderPass::Flags flags) {
			return static_cast<RenderPass::Flags>(~static_cast<uint8_t>(flags));
		}
	} 
}
