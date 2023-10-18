#include "Font.h"


namespace Jimara {
	struct Font::Helpers {

	};

	bool Font::RequireGlyphs(const Glyph* glyphs, size_t count) {
		bool oldUVsRecalculated = false;
		bool allSymbolsLoaded = true;
		{
			std::unique_lock<std::shared_mutex> lock(m_uvLock);

			struct GlyphAndAspect {
				Glyph glyph = '/0';
				float aspect = 0.0f;
			};
			static Stacktor<GlyphAndAspect, 4u> addedGlyphs;
			addedGlyphs.Clear();

			// Take a look at which glyphs got added and cache their aspect ratios:
			for (size_t i = 0u; i < count; i++) {
				const Glyph& glyph = glyphs[i];
				if (m_glyphAspects.find(glyph) != m_glyphAspects.end())
					continue;
				const float aspect = PrefferedAspectRatio(glyph);
				if (aspect < 0.0f) {
					GraphicsDevice()->Log()->Error(
						"Font::RequireGlyphs - Failed to load glyph for '", glyph, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					allSymbolsLoaded = false;
					continue;
				}
				addedGlyphs.Push(GlyphAndAspect{ glyph, aspect });
				m_glyphAspects[glyph] = aspect;
			}

			// If addedGlyphs is empty, we can do an early exit:
			if (addedGlyphs.Size() <= 0u) {
				addedGlyphs.Clear();
				return allSymbolsLoaded;
			}

			// Add new UV-s and recalculate if needed:
			float yStep = (m_glyphBounds.size() > 0u) ? m_glyphBounds.begin()->second.Size().y : 1.0f;
			while (true) {
				auto filledUVOutOfBounds = [&]() { return m_filledUVptr.y >= (1.0f + 0.5f * yStep); };

				// Try to place new glyphs:
				for (size_t i = 0u; i < addedGlyphs.Size(); i++) {
					const GlyphAndAspect& info = addedGlyphs[i];
					while (true) {
						const Vector2 size = yStep * Vector2(info.aspect, 1.0f);
						const Vector2 end = m_filledUVptr + size;
						if (end.x > 1.0f) {
							// If endpoint goes beyond the texture boundaries, move to next line:
							m_filledUVptr = Vector2(0.0f, m_filledUVptr.y + yStep);
							if (filledUVOutOfBounds())
								break;
							else continue;
						}
						else {
							// If end is within bounds, we can insert the UV normally:
							m_glyphBounds[info.glyph] = Rect(m_filledUVptr, end);
							m_filledUVptr.x = end.x;
							break;
						}
					}

					// Early exit if glyph space is filled:
					if (filledUVOutOfBounds())
						break;
				}

				// UV generation is done if glyph space is NOT overfilled:
				if (!filledUVOutOfBounds())
					break;

				// If we failed to place glyphs, we need full recalculation and, therefore, addedGlyphs has to be refilled:
				if (addedGlyphs.Size() < m_glyphAspects.size()) {
					addedGlyphs.Clear();
					for (decltype(m_glyphAspects)::const_iterator it = m_glyphAspects.begin(); it != m_glyphAspects.end(); ++it)
						addedGlyphs.Push(GlyphAndAspect{ it->first, it->second });
				}

				// Reset UV parameters and double the atlas size:
				oldUVsRecalculated = true;
				m_filledUVptr = Vector2(0.0f);
				yStep *= 0.5f;
			}

			// Do the final cleanup:
			addedGlyphs.Clear();
			if (oldUVsRecalculated)
				m_invalidateAtlasses();
		}

		// If old atlasses are invalidated, we let the listeners know:
		if (oldUVsRecalculated)
			m_onAtlasInvalidated(this);

		// If we got here, all's good
		return allSymbolsLoaded;
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
