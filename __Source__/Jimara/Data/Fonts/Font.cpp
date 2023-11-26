#include "Font.h"


namespace Jimara {
	struct Font::Helpers {
		static Stacktor<GlyphPlacement, 4u>& GetEmptyAddedGlyphBuffer() {
			static thread_local Stacktor<GlyphPlacement, 4u> newGlyphs;
			newGlyphs.Clear();
			return newGlyphs;
		}

		inline static void OnGlyphUVsAdded(Atlas* atlas, const GlyphPlacement* newGlyphs, size_t newGlyphCount, Graphics::CommandBuffer* commandBuffer) {
			assert(atlas->m_texture != nullptr);
			atlas->m_font->DrawGlyphs(atlas->m_texture->TargetView(), static_cast<uint32_t>(atlas->m_size), newGlyphs, newGlyphCount, commandBuffer);
			if ((atlas->m_flags & AtlasFlags::NO_MIPMAPS) == AtlasFlags::NONE)
				atlas->m_texture->TargetView()->TargetTexture()->GenerateMipmaps(commandBuffer);
		}

		inline static void OnGlyphUVsInvalidated(Atlas* atlas, const std::unordered_map<Glyph, Rect>* oldUVs, Graphics::CommandBuffer* commandBuffer) {
			// Failure message:
			auto fail = [&](const auto&... message) {
				atlas->Font()->GraphicsDevice()->Log()->Error("Font::Helpers::OnGlyphUVsInvalidated - ", message...);
				return nullptr;
			};

			// Store old texture for future copy
			const Reference<Graphics::TextureSampler> oldTexture = atlas->m_texture;

			// Create texture:
			atlas->m_texture = [&]() -> Reference<Graphics::TextureSampler> {
				const float yStep = atlas->Font()->m_glyphUVSize;
				assert(yStep > std::numeric_limits<float>::epsilon());
				const float vGlyphCount = std::ceil(1.0f / yStep);
				const Size2 newTextureSize = Size2(Math::Max(static_cast<uint32_t>(vGlyphCount * atlas->Size()), 1u));
				assert(newTextureSize.x == newTextureSize.y);
				const Reference<Graphics::Texture> texture = atlas->Font()->GraphicsDevice()->CreateTexture(
					Graphics::Texture::TextureType::TEXTURE_2D, Graphics::Texture::PixelFormat::R8_UNORM,
					Size3(newTextureSize, 1u), 1u, (atlas->Flags() & AtlasFlags::NO_MIPMAPS) == AtlasFlags::NONE,
					Graphics::ImageTexture::AccessFlags::NONE);
				if (texture == nullptr)
					return fail("Failed to create new texture!");
				const Reference<Graphics::TextureView> view = texture->CreateView(Graphics::TextureView::ViewType::VIEW_2D);
				if (view == nullptr)
					return fail("Failed to create texture view!");
				const Reference<Graphics::TextureSampler> sampler = view->CreateSampler(
					Graphics::TextureSampler::FilteringMode::LINEAR,
					Graphics::TextureSampler::WrappingMode::CLAMP_TO_BORDER);
				if (sampler == nullptr)
					return fail("Failed to create texture samlpler!");
				return sampler;
			}();
			if (atlas->m_texture == nullptr)
				return;

			// Copy old texture contents:
			if (oldTexture != nullptr) {
				const Vector3 targetSize = atlas->m_texture->TargetView()->TargetTexture()->Size();
				const Vector3 srcSize = oldTexture->TargetView()->TargetTexture()->Size();
				for (std::remove_pointer_t<decltype(oldUVs)>::const_iterator oldIt = oldUVs->begin(); oldIt != oldUVs->end(); ++oldIt) {
					decltype(atlas->Font()->m_glyphBounds)::const_iterator newIt = atlas->Font()->m_glyphBounds.find(oldIt->first);
					if (newIt == atlas->Font()->m_glyphBounds.end())
						continue;
					Size3 srcOffset = srcSize * Vector3(oldIt->second.start, 0.0f) + 0.5f;
					Size3 dstOffset = targetSize * Vector3(newIt->second.boundaries.start, 0.0f) + 0.5f;
					Size3 regionSize = targetSize * Vector3(newIt->second.boundaries.Size(), 1.0f) + 1.0f;
					atlas->m_texture->TargetView()->TargetTexture()->Copy(
						commandBuffer, oldTexture->TargetView()->TargetTexture(), dstOffset, srcOffset, regionSize);
				}
			}
			else oldUVs = nullptr;

			// Detect and add added glyphs:
			{
				Stacktor<GlyphPlacement, 4u>& glyphBuffer = Helpers::GetEmptyAddedGlyphBuffer();
				assert(glyphBuffer.Size() <= 0u);
				for (decltype(atlas->Font()->m_glyphBounds)::const_iterator it = atlas->Font()->m_glyphBounds.begin(); it != atlas->Font()->m_glyphBounds.end(); ++it)
					if (oldUVs == nullptr || (oldUVs->find(it->first) == oldUVs->end()))
						glyphBuffer.Push(GlyphPlacement{ it->first, it->second.boundaries });
				OnGlyphUVsAdded(atlas, glyphBuffer.Data(), glyphBuffer.Size(), commandBuffer);
				glyphBuffer.Clear();
			}
		}

		
		struct ExactSizeAtlasCache : public virtual ObjectCache<uint64_t> {
			Reference<Atlas> GetAtlas(Font* font, float size, AtlasFlags flags) {
#pragma warning (disable: 4250)
				struct CachedAtlas : public virtual Atlas, public virtual ObjectCache<uint64_t>::StoredObject {
					inline CachedAtlas(Jimara::Font* font, float size, AtlasFlags flags) : Atlas(font, size, flags) {}
					inline virtual ~CachedAtlas() {}
				};
#pragma warning (default: 4250)
				static_assert(sizeof(std::underlying_type_t<AtlasFlags>) <= 4u);
				static_assert(sizeof(float) == 4u);
				static_assert(sizeof(uint64_t) == 8u);
				const uint64_t sharedKey = (static_cast<uint64_t>(flags) << 32u) | (*reinterpret_cast<uint32_t*>(&size));
				return GetCachedOrCreate(sharedKey, false, [&]() { return Object::Instantiate<CachedAtlas>(font, size, flags); });
			}
		};

#pragma warning (disable: 4250)
		struct ResourceAtlas : public virtual Atlas, public virtual Resource {
			inline ResourceAtlas(Jimara::Font* font, float size, AtlasFlags flags) : Atlas(font, size, flags) {}
			inline virtual ~ResourceAtlas() {}
		};
#pragma warning (default: 4250)
		struct SharedAtlasAsset : public virtual Asset::Of<ResourceAtlas> {
			Font* const font;
			const float size;
			const AtlasFlags flags;

			inline SharedAtlasAsset(Font* fnt, float sz, AtlasFlags fl)
				: font(fnt), size(sz), flags(fl), Asset(GUID::Generate()) {}
			inline virtual ~SharedAtlasAsset() {}

			inline virtual Reference<ResourceAtlas> LoadItem() final override {
				return Object::Instantiate<ResourceAtlas>(font, size, flags);
			}
		};

		struct AtlasCache : public virtual Object {
			Reference<SharedAtlasAsset> sharedAtlasWithMips;
			Reference<SharedAtlasAsset> sharedAtlasWithoutMips;
			Reference<ExactSizeAtlasCache> exactSizeCache = Object::Instantiate<ExactSizeAtlasCache>();
		};
	};


	Font::Font(Graphics::GraphicsDevice* device) 
		: m_graphicsDevice(device)
		, m_commandPool(Graphics::OneTimeCommandPool::GetFor(device))
		, m_atlasCache(Object::Instantiate<Helpers::AtlasCache>()) {
		assert(m_graphicsDevice != nullptr);
		assert(m_commandPool != nullptr);
	}

	Font::~Font() {}


	Reference<Font::Atlas> Font::GetAtlas(float size, AtlasFlags flags) {
		std::unique_lock<std::shared_mutex> lock(m_uvLock);

		Helpers::AtlasCache* atlasCache = dynamic_cast<Helpers::AtlasCache*>(m_atlasCache.operator Jimara::Object * ());
		assert(atlasCache != nullptr);

		// Return shared atlas if shared one was requested and size is not important
		if ((flags & AtlasFlags::CREATE_UNIQUE) == AtlasFlags::NONE &&
			(flags & AtlasFlags::EXACT_GLYPH_SIZE) == AtlasFlags::NONE) {
			const AtlasFlags mipFlag = flags & AtlasFlags::NO_MIPMAPS;
			Reference<Helpers::SharedAtlasAsset>& sharedAtlasAsset =
				(mipFlag == AtlasFlags::NONE) ? atlasCache->sharedAtlasWithMips : atlasCache->sharedAtlasWithoutMips;
			
			size = Math::Max(size, (sharedAtlasAsset == nullptr) ? 8.0f : sharedAtlasAsset->size);
			if (sharedAtlasAsset == nullptr || size > sharedAtlasAsset->size) {
				uint64_t sz = 1u;
				while (sz < static_cast<uint64_t>(size))
					sz <<= 1u;
				sharedAtlasAsset = Object::Instantiate<Helpers::SharedAtlasAsset>(this, static_cast<float>(sz), mipFlag);
			}
			assert(sharedAtlasAsset != nullptr);
			return sharedAtlasAsset->Load();
		}

		// Size HAS TO BE positive:
		if (size <= 0.0f) {
			m_graphicsDevice->Log()->Error("Font::GetAtlas - Size has to be larger than 0! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return nullptr;
		}

		// If we need an unique atlas, we return it here:
		if ((flags & AtlasFlags::CREATE_UNIQUE) != AtlasFlags::NONE) {
			const Reference<Atlas> newAtlas = new Atlas(this, size, flags);
			newAtlas->ReleaseRef();
			return newAtlas;
		}
		
		// Return cached exact-size atlas:
		return atlasCache->exactSizeCache->GetAtlas(this, size, flags);
	}


	bool Font::RequireGlyphs(const Glyph* glyphs, size_t count) {
		bool oldUVsRecalculated = false;
		bool allSymbolsLoaded = true;
		{
			std::unique_lock<std::shared_mutex> lock(m_uvLock);

			struct GlyphAndShape {
				Glyph glyph = '/0';
				GlyphShape shape = {};
			};
			static Stacktor<GlyphAndShape, 4u> addedGlyphs;
			addedGlyphs.Clear();

			// Take a look at which glyphs got added and cache their aspect ratios:
			for (size_t i = 0u; i < count; i++) {
				const Glyph& glyph = glyphs[i];
				if (m_glyphShapes.find(glyph) != m_glyphShapes.end())
					continue;
				const GlyphShape shape = GetGlyphShape(glyph);
				if (shape.size.x < 0.0f || shape.size.y < 0.0f) {
					GraphicsDevice()->Log()->Error(
						"Font::RequireGlyphs - Failed to load glyph for '", glyph, "'! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					allSymbolsLoaded = false;
					continue;
				}
				addedGlyphs.Push(GlyphAndShape{ glyph, shape });
				m_glyphShapes[glyph] = shape;
			}

			// If addedGlyphs is empty, we can do an early exit:
			if (addedGlyphs.Size() <= 0u) {
				addedGlyphs.Clear();
				return allSymbolsLoaded;
			}

			// Add UV-s for added glyphs and recalculate if needed:
			std::unordered_map<Glyph, Rect> oldGlyphBounds;
			Stacktor<GlyphInfo, 4u> updatedGlyphBounds;
			while (true) {
				auto filledUVOutOfBounds = [&]() { return m_filledUVptr.y >= (1.0f + 0.5f * m_glyphUVSize); };

				// Try to place added glyphs:
				assert(updatedGlyphBounds.Size() <= 0u);
				for (size_t i = 0u; i < addedGlyphs.Size(); i++) {
					const GlyphAndShape& info = addedGlyphs[i];
					while (true) {
						const constexpr float paddingFactor = 0.1f;
						const float padding = m_glyphUVSize * paddingFactor;
						const Vector2 size = m_glyphUVSize * info.shape.size;
						const Vector2 end = m_filledUVptr + size + padding;
						m_nextRowY = Math::Max(m_nextRowY, end.y);
						if (end.x > 1.0f || end.y > 1.0f) {
							// If endpoint goes beyond the texture boundaries, move to next line:
							m_filledUVptr = Vector2(0.0f, m_nextRowY + padding);
							if (filledUVOutOfBounds())
								break;
							else continue;
						}
						else {
							// If end is within bounds, we can insert the UV normally:
							GlyphInfo newInfo = {};
							newInfo.glyph = info.glyph;
							newInfo.shape = info.shape;
							newInfo.shape.size += paddingFactor;
							newInfo.boundaries = Rect(m_filledUVptr, end);
							updatedGlyphBounds.Push(newInfo);
							m_filledUVptr.x = end.x + padding;
							break;
						}
					}

					// Early exit if glyph space is filled:
					if (filledUVOutOfBounds())
						break;
				}

				// UV generation is done if glyph space is NOT overfilled:
				if (!filledUVOutOfBounds()) {
					for (size_t i = 0u; i < updatedGlyphBounds.Size(); i++) {
						const GlyphInfo& entry = updatedGlyphBounds[i];
						m_glyphBounds[entry.glyph] = entry;
					}
					break;
				}

				// If we failed to place glyphs, we need full recalculation and, therefore, addedGlyphs has to be refilled:
				if (!oldUVsRecalculated) {
					addedGlyphs.Clear();
					for (decltype(m_glyphShapes)::const_iterator it = m_glyphShapes.begin(); it != m_glyphShapes.end(); ++it)
						addedGlyphs.Push(GlyphAndShape{ it->first, it->second });
					if (!oldUVsRecalculated)
						for (decltype(m_glyphBounds)::const_iterator it = m_glyphBounds.begin(); it != m_glyphBounds.end(); ++it)
							oldGlyphBounds[it->first] = it->second.boundaries;
					oldUVsRecalculated = true;
				}

				// Reset UV parameters and double the atlas size:
				m_filledUVptr = Vector2(0.0f);
				m_nextRowY = 0.0f;
				updatedGlyphBounds.Clear();
				m_glyphUVSize *= 0.5f;
			}

			// Do the final cleanup:
			Graphics::OneTimeCommandPool::Buffer commandBuffer(m_commandPool);
			if (oldUVsRecalculated)
				m_invalidateAtlasses(&oldGlyphBounds, commandBuffer);
			else {
				Stacktor<GlyphPlacement, 4u>& newGlyphs = Helpers::GetEmptyAddedGlyphBuffer();
				assert(newGlyphs.Size() <= 0u);
				for (size_t i = 0u; i < addedGlyphs.Size(); i++)
					newGlyphs.Push(GlyphPlacement{ addedGlyphs[i].glyph, m_glyphBounds[addedGlyphs[i].glyph].boundaries });
				m_addGlyphsToAtlasses(newGlyphs.Data(), newGlyphs.Size(), commandBuffer);
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
		((Event<const std::unordered_map<Glyph, Rect>*, Graphics::CommandBuffer*>&)m_font->m_invalidateAtlasses) +=
			Callback<const std::unordered_map<Glyph, Rect>*, Graphics::CommandBuffer*>(Helpers::OnGlyphUVsInvalidated, this);
		((Event<const GlyphPlacement*, size_t, Graphics::CommandBuffer*>&)m_font->m_addGlyphsToAtlasses) +=
			Callback<const GlyphPlacement*, size_t, Graphics::CommandBuffer*>(Helpers::OnGlyphUVsAdded, this);
		// Note that we do not use any locks here only because Atlas objects can only be created under write lock:
		Graphics::OneTimeCommandPool::Buffer commandBuffer(font->m_commandPool);
		Helpers::OnGlyphUVsInvalidated(this, nullptr, commandBuffer);
	}

	Font::Atlas::~Atlas() {
		((Event<const std::unordered_map<Glyph, Rect>*, Graphics::CommandBuffer*>&)m_font->m_invalidateAtlasses) -=
			Callback<const std::unordered_map<Glyph, Rect>*, Graphics::CommandBuffer*>(Helpers::OnGlyphUVsInvalidated, this);
		((Event<const GlyphPlacement*, size_t, Graphics::CommandBuffer*>&)m_font->m_addGlyphsToAtlasses) -=
			Callback<const GlyphPlacement*, size_t, Graphics::CommandBuffer*>(Helpers::OnGlyphUVsAdded, this);
	}




	Font::Reader::Reader(Atlas* atlas) 
		: m_atlas(atlas)
		, m_uvLock([&]() -> std::shared_mutex& {
		static decltype(Font::m_uvLock) lock;
		return atlas == nullptr ? lock : atlas->Font()->m_uvLock;
			}()) {}

	Font::Reader::~Reader() {}

	std::optional<Font::GlyphInfo> Font::Reader::GetGlyphInfo(const Glyph& glyph) {
		if (m_atlas == nullptr)
			return std::optional<GlyphInfo>();
		decltype(m_atlas->Font()->m_glyphBounds)::const_iterator it = m_atlas->Font()->m_glyphBounds.find(glyph);
		if (it == m_atlas->Font()->m_glyphBounds.end())
			return std::optional<GlyphInfo>();
		return it->second;
	}

	Reference<Graphics::TextureSampler> Font::Reader::GetTexture() {
		if (m_atlas == nullptr)
			return nullptr;
		return m_atlas->m_texture;
	}
}
