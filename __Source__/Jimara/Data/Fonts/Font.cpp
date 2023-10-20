#include "Font.h"


namespace Jimara {
	struct Font::Helpers {
		static Stacktor<GlyphInfo, 4u>& GetEmptyAddedGlyphBuffer() {
			static thread_local Stacktor<GlyphInfo, 4u> newGlyphs;
			newGlyphs.Clear();
			return newGlyphs;
		}

		inline static void OnGlyphUVsAdded(Atlas* atlas, const GlyphInfo* newGlyphs, size_t newGlyphCount) {
			assert(atlas->m_texture != nullptr);
			atlas->m_font->DrawGlyphs(atlas->m_texture->TargetView(), newGlyphs, newGlyphCount);
			// __TODO__: If texture has mipmaps, generate them...
		}

		inline static void OnGlyphUVsInvalidated(Atlas* atlas, const std::unordered_map<Glyph, Rect>* oldUVs) {
			// Failure message:
			auto fail = [&](const auto&... message) {
				atlas->Font()->GraphicsDevice()->Log()->Error("Font::Helpers::OnGlyphUVsInvalidated - ", message...);
				return nullptr;
			};

			// Store old texture for future copy
			const Reference<Graphics::TextureSampler> oldTexture = atlas->m_texture;

			// Create new texture:
			atlas->m_texture = [&]() -> Reference<Graphics::TextureSampler> {
				const float yStep = (atlas->Font()->m_glyphBounds.size() > 0u) ? atlas->Font()->m_glyphBounds.begin()->second.Size().y : 1.0f;
				assert(yStep > std::numeric_limits<float>::epsilon());
				const float vGlyphCount = (1.0f / yStep);
				const Size2 newTextureSize = Size2(static_cast<uint32_t>(vGlyphCount * atlas->Size()));
				assert(newTextureSize.x == newTextureSize.y);
				const Reference<Graphics::Texture> texture = atlas->Font()->GraphicsDevice()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8G8B8A8_SRGB,
					Size3(newTextureSize, 1u), 1u, (atlas->Flags() & AtlasFlags::NO_MIPMAPS) != AtlasFlags::NONE,
					Graphics::ImageTexture::AccessFlags::NONE);
				if (texture == nullptr)
					return fail("Failed to create new texture!");
				const Reference<Graphics::TextureView> view = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
				if (view == nullptr)
					return fail("Failed to create texture view!");
				const Reference<Graphics::TextureSampler> sampler = view->CreateSampler();
				if (sampler == nullptr)
					return fail("Failed to create texture samlpler!");
				return sampler;
			}();
			if (atlas->m_texture == nullptr)
				return;

			// Copy old texture contents:
			if (oldTexture != nullptr) {
				// __TODO__: Copy old texture regions if present...
			}
			else oldUVs = nullptr;

			// Detect and add new glyphs:
			{
				Stacktor<GlyphInfo, 4u>& glyphBuffer = Helpers::GetEmptyAddedGlyphBuffer();
				assert(glyphBuffer.Size() <= 0u);
				for (decltype(atlas->Font()->m_glyphBounds)::const_iterator it = atlas->Font()->m_glyphBounds.begin(); it != atlas->Font()->m_glyphBounds.end(); ++it)
					if (oldUVs == nullptr || (oldUVs->find(it->first) == oldUVs->end()))
						glyphBuffer.Push(GlyphInfo{ it->first, it->second });
				OnGlyphUVsAdded(atlas, glyphBuffer.Data(), glyphBuffer.Size());
				glyphBuffer.Clear();
			}
		}
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
			std::unordered_map<Glyph, Rect> oldGlyphBounds;
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
					oldUVsRecalculated = true;
					oldGlyphBounds = m_glyphBounds;
				}

				// Reset UV parameters and double the atlas size:
				m_filledUVptr = Vector2(0.0f);
				yStep *= 0.5f;
			}

			// Do the final cleanup:
			if (oldUVsRecalculated)
				m_invalidateAtlasses(&oldGlyphBounds);
			else {
				Stacktor<GlyphInfo, 4u>& newGlyphs = Helpers::GetEmptyAddedGlyphBuffer();
				assert(newGlyphs.Size() <= 0u);
				for (size_t i = 0u; i < addedGlyphs.Size(); i++)
					newGlyphs.Push(GlyphInfo{ addedGlyphs[i].glyph, m_glyphBounds[addedGlyphs[i].glyph] });
				m_addGlyphsToAtlasses(newGlyphs.Data(), newGlyphs.Size());
				newGlyphs.Clear();
			}
			addedGlyphs.Clear();
		}

		// If old atlasses are invalidated, we let the listeners know:
		if (oldUVsRecalculated)
			m_onAtlasInvalidated(this);

		// If we got here, all's good
		return allSymbolsLoaded;
	}




	Font::Atlas::Atlas(Jimara::Font* font, float size, AtlasFlags flags)
		: m_font(font), m_size(size), m_flags(flags) {
		assert(m_font != nullptr);
		assert(m_size > 0.0f);
		((Event<const std::unordered_map<Glyph, Rect>*>&)m_font->m_invalidateAtlasses) += 
			Callback<const std::unordered_map<Glyph, Rect>*>(Helpers::OnGlyphUVsInvalidated, this);
		((Event<const GlyphInfo*, size_t>&)m_font->m_addGlyphsToAtlasses) +=
			Callback<const GlyphInfo*, size_t>(Helpers::OnGlyphUVsAdded, this);
		// Note that we do not use any locks here only because Atlas objects can only be created under write lock:
		Helpers::OnGlyphUVsInvalidated(this, nullptr);
	}

	Font::Atlas::~Atlas() {
		((Event<const std::unordered_map<Glyph, Rect>*>&)m_font->m_invalidateAtlasses) -=
			Callback<const std::unordered_map<Glyph, Rect>*>(Helpers::OnGlyphUVsInvalidated, this);
		((Event<const GlyphInfo*, size_t>&)m_font->m_addGlyphsToAtlasses) -=
			Callback<const GlyphInfo*, size_t>(Helpers::OnGlyphUVsAdded, this);
	}




	Font::Reader::Reader(Atlas* atlas) 
		: m_atlas(atlas)
		, m_uvLock([&]() -> std::shared_mutex& {
		static decltype(Font::m_uvLock) lock;
		return atlas == nullptr ? lock : atlas->Font()->m_uvLock;
			}()) {}

	Font::Reader::~Reader() {}

	std::optional<Rect> Font::Reader::GetGlyphBoundaries(const Glyph& glyph) {
		if (m_atlas == nullptr)
			return std::optional<Rect>();
		decltype(m_atlas->Font()->m_glyphBounds)::const_iterator it = m_atlas->Font()->m_glyphBounds.find(glyph);
		if (it == m_atlas->Font()->m_glyphBounds.end())
			return std::optional<Rect>();
		return it->second;
	}

	Reference<Graphics::TextureSampler> Font::Reader::GetTexture() {
		if (m_atlas == nullptr)
			return nullptr;
		return m_atlas->m_texture;
	}
}
