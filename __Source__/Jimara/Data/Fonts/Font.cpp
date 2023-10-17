#include "Font.h"


namespace Jimara {
	bool Font::RequireGlyphs(const Glyph* glyphs, size_t count) {
		// __TODO__: Implement this crap!
		GraphicsDevice()->Log()->Error("Font::RequireGlyphs - Not yet implemented!");
		return false;
	}

	Font::Reader::Reader(Font* font) 
		: m_font(font)
		, m_uvLock([&]() -> std::shared_mutex& {
		static decltype(Font::m_uvLock) lock;
		return font == nullptr ? lock : font->m_uvLock;
			}()) {}

	Font::Reader::~Reader() {}

	std::optional<Rect> Font::Reader::GetGlyphBoundaries(const Glyph& glyph) {
		if (m_font == nullptr)
			return std::optional<Rect>();
		decltype(m_font->m_glyphBounds)::const_iterator it = m_font->m_glyphBounds.find(glyph);
		if (it == m_font->m_glyphBounds.end())
			return std::optional<Rect>();
		return it->second;
	}

	
	Reference<Graphics::TextureSampler> Font::Reader::GetAtlas(float size, AtlasFlags flags) {
		if (m_font == nullptr)
			return nullptr;
		// __TODO__: Implement this crap!
		m_font->GraphicsDevice()->Log()->Error("Font::Reader::GetAtlas - Not yet implemented!");
		return nullptr;
	}
}
