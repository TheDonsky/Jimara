#pragma once
#include "../Font.h"
#include "../../../Core/Memory/MemoryBlock.h"


namespace Jimara {
	/// <summary>
	/// Freetype-backed Font implementation
	/// </summary>
	class JIMARA_API FreetypeFont : public virtual Font {
	public:
		/// <summary>
		/// Creates a Freetype-backed Font 
		/// </summary>
		/// <param name="fontBinary"> Font binary data </param>
		/// <param name="faceIndex"> Index of the face in the font file </param>
		/// <param name="device"> Graphics device </param>
		static Reference<FreetypeFont> Create(const MemoryBlock& fontBinary, uint32_t faceIndex, Graphics::GraphicsDevice* device);

		/// <summary> Virtual destructor </summary>
		virtual ~FreetypeFont() = 0;

		/// <summary>
		/// Aspect ratio for given glyph
		/// <para/> Negative values mean the glyph load failed
		/// </summary>
		/// <param name="glyph"> Symbol </param>
		/// <returns> (width / height) if glyph is valid and anything negative if it fails </returns>
		virtual float PrefferedAspectRatio(const Glyph& glyph) override;

		/// <summary>
		/// Draws glyphs on a texture
		/// </summary>
		/// <param name="targetImage"> Target texture </param>
		/// <param name="glyphs"> List of glyphs and corresponding boundaries </param>
		/// <param name="glyphCount"> Number of glyphs </param>
		/// <param name="commandBuffer"> Command buffer for any graphics operations that may be needed within the backend </param>
		/// <returns> True, if nothing fails </returns>
		virtual bool DrawGlyphs(const Graphics::TextureView* targetImage, const GlyphInfo* glyphs, size_t glyphCount, Graphics::CommandBuffer* commandBuffer) = 0;

	private:
		const MemoryBlock m_fontBinary;

		// Private stuff
		struct Helpers;

		// Actual implementation is hidden
		inline FreetypeFont();
	};
}
