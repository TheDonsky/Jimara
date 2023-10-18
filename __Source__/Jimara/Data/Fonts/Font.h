#pragma once
#include "../../Graphics/GraphicsDevice.h"


namespace Jimara {
	/// <summary>
	/// Loaded font, providing drawable glyph atlasses
	/// </summary>
	class JIMARA_API Font : public virtual Resource {
	public:
		/// <summary> Character identifier </summary>
		using Glyph = wchar_t;

		/// <summary> Glyph UV and atlass reader (creating this freezes RequireGlyphs() calls, making it safe to read UV coordinates) </summary>
		class JIMARA_API Reader;

		/// <summary> Atlass options </summary>
		enum class JIMARA_API AtlasFlags : uint16_t;

		/// <summary> Glyph and UV rectangle </summary>
		struct JIMARA_API GlyphInfo;

		/// <summary>
		/// Loads additional glyphs and recalculates UV-s if required.
		/// <para/> If the atlasses need to be recreated, OnAtlasInvalidated will fire.
		/// </summary>
		/// <param name="glyphs"> List of glyphs that need to be included </param>
		/// <param name="count"> Number of glyphs that need to be included </param>
		/// <returns> True, if everything goes OK, false if any glyph load fails </returns>
		bool RequireGlyphs(const Glyph* glyphs, size_t count);

		/// <summary>
		/// Loads additional glyphs and recalculates UV-s if required.
		/// <para/> If the atlasses need to be recreated, OnAtlasInvalidated will fire.
		/// </summary>
		/// <param name="glyphs"> String view of glyphs that need to be included </param>
		/// <returns> True, if everything goes OK, false if any glyph load fails </returns>
		inline bool RequireGlyphs(const std::wstring_view& glyphs) { return RequireGlyphs(glyphs.data(), glyphs.length()); };

		/// <summary>
		/// Invoked each time glyph UV coordinates and atlast textures become outdated.
		/// <para/> Keep in mind, that even if you ignore this, your atlass will still work, but will be consuming memory unnecessarily.
		/// </summary>
		inline Event<Font*>& OnAtlasInvalidated() { return m_onAtlasInvalidated; }

		/// <summary> Graphics device, the atlasses are created on </summary>
		inline Graphics::GraphicsDevice* GraphicsDevice()const { return m_graphicsDevice; }

		/// <summary>
		/// Aspect ratio for given glyph
		/// <para/> Negative values mean the glyph load failed
		/// </summary>
		/// <param name="glyph"> Symbol </param>
		/// <returns> (width / height) if glyph is valid and anything negative if it fails </returns>
		virtual float PrefferedAspectRatio(const Glyph& glyph) = 0;

		/// <summary>
		/// Draws glyphs on a texture
		/// <para/> Notes:
		/// <para/>		0. Primary purpose of this method is internal use by Font class through Font::Reader interface; it is exposed, so that if one
		///				really needs to, it is possible to create the atlasses by hand;
		/// <para/>		1. Keep in mind, that there is a requirenment that the call does not change the texture contents beyond the glyph boundaries;
		/// <para/>		2. Drawing may or may not be immediate; A background command buffer can be used. So reading back texture data on CPU directly might not work;
		///				(Generally, one would draw glyphs on GPU-RW texture and that's what implementations will assume; if you want to read-back, 
		///				feel free to submit any command buffer and wait on it; however, this API is not supposed to be used for CPU read-back!)
		/// </summary>
		/// <param name="targetImage"> Target texture </param>
		/// <param name="glyphs"> List of glyphs and corresponding boundaries </param>
		/// <param name="glyphCount"> Number of glyphs </param>
		/// <returns> True, if nothing fails </returns>
		virtual bool DrawGlyphs(const Graphics::TextureView* targetImage, const GlyphInfo* glyphs, size_t glyphCount) = 0;


	private:
		// Graphics device
		const Reference<Graphics::GraphicsDevice> m_graphicsDevice;

		// Invoked, whenever new glyphs get added, old atlass fills up and old atlasses get invalidated
		EventInstance<Font*> m_onAtlasInvalidated;
		
		// Invoked before m_onAtlasInvalidated under the write lock for internal cleanup
		EventInstance<> m_invalidateAtlasses;

		// Lock for reading glyphs UV-s
		std::shared_mutex m_uvLock;
		
		// Cached glyph aspect ratio; Kept constant after glyph addition;
		std::unordered_map<Glyph, float> m_glyphAspects;

		// Glyph-to-UV map, recalculated after each atlas invalidation
		std::unordered_map<Glyph, Rect> m_glyphBounds;

		// Current glyph UV state (basically, tells that space is filled up to this coordinate);
		Vector2 m_filledUVptr = Vector2(0.0f);

		// Lock for atlas generation/retrieval
		std::mutex m_atlasLock;

		// Private stuff is here..
		struct Helpers;
	};


	/// <summary> Atlass options (bitmask) </summary>
	enum class JIMARA_API Font::AtlasFlags : uint16_t {
		/// <summary> No requirenment (main shared atlas) </summary>
		NONE = 0u,
		
		/// <summary> Exact glyph size (if not requested, a shared atlas will be returned, with possibly larger glyphs) </summary>
		EXACT_GLYPH_SIZE = (1u << 0u),

		/// <summary> Atlas will have no mipmaps </summary>
		NO_MIPMAPS = (1u << 1u),

		/// <summary> If this flag is set, GetAtlas will not use any of the shared atlasses and will simply create a new texture </summary>
		CREATE_UNIQUE = (1u << 2u)
	};


	/// <summary> Glyph UV and atlass reader (creating this freezes RequireGlyphs() calls, making it safe to read UV coordinates) </summary>
	class JIMARA_API Font::Reader final {
	public:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="font"> Font to read </param>
		Reader(Font* font);

		/// <summary> Destructor </summary>
		~Reader();

		/// <summary>
		/// Looks up glyph boundary
		/// </summary>
		/// <param name="glyph"> Symbol </param>
		/// <returns> Boundaries of given glyph if found on the atlas; will not have value otherwise </returns>
		std::optional<Rect> GetGlyphBoundaries(const Glyph& glyph);

		/// <summary>
		/// Retrieves shared font atlas texture of given size
		/// </summary>
		/// <param name="size"> Font size (use EXACT_GLYPH_SIZE to make sure you do not get a larger image) </param>
		/// <param name="flags"></param>
		/// <returns></returns>
		Reference<Graphics::TextureSampler> GetAtlas(float size, AtlasFlags flags);

	private:
		std::shared_lock<std::shared_mutex> m_uvLock;
		Reference<Font> m_font;
	};


	/// <summary> Glyph and UV rectangle </summary>
	struct JIMARA_API Font::GlyphInfo {
		/// <summary> Symbol </summary>
		Glyph glyph = '/0';

		/// <summary> Glyph UV coordinate boundaries </summary>
		Rect boundaries = {};
	};





	/// <summary>
	/// 'Or' operator with AtlasFlags
	/// </summary>
	/// <param name="a"> First set of flags </param>
	/// <param name="b"> Second set of flags </param>
	/// <returns> a | b </returns>
	inline static constexpr Font::AtlasFlags operator|(Font::AtlasFlags a, Font::AtlasFlags b) {
		return static_cast<Font::AtlasFlags>(
			static_cast<std::underlying_type_t<Font::AtlasFlags>>(a) |
			static_cast<std::underlying_type_t<Font::AtlasFlags>>(b));
	}

	/// <summary>
	/// 'And' operator with AtlasFlags
	/// </summary>
	/// <param name="a"> First set of flags </param>
	/// <param name="b"> Second set of flags </param>
	/// <returns> a & b </returns>
	inline static constexpr Font::AtlasFlags operator&(Font::AtlasFlags a, Font::AtlasFlags b) {
		return static_cast<Font::AtlasFlags>(
			static_cast<std::underlying_type_t<Font::AtlasFlags>>(a) &
			static_cast<std::underlying_type_t<Font::AtlasFlags>>(b));
	}

	/// <summary>
	/// 'Xor' operator with AtlasFlags
	/// </summary>
	/// <param name="a"> First set of flags </param>
	/// <param name="b"> Second set of flags </param>
	/// <returns> a ^ b </returns>
	inline static constexpr Font::AtlasFlags operator^(Font::AtlasFlags a, Font::AtlasFlags b) {
		return static_cast<Font::AtlasFlags>(
			static_cast<std::underlying_type_t<Font::AtlasFlags>>(a) ^
			static_cast<std::underlying_type_t<Font::AtlasFlags>>(b));
	}

	/// <summary>
	/// Bitwise inverse of AtlasFlags
	/// </summary>
	/// <param name="f"> Flags </param>
	/// <returns> ~f </returns>
	inline static constexpr Font::AtlasFlags operator~(Font::AtlasFlags f) {
		return static_cast<Font::AtlasFlags>(~static_cast<std::underlying_type_t<Font::AtlasFlags>>(f));
	}
}
