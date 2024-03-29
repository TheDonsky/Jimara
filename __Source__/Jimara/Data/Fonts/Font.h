#pragma once
#include "../../Graphics/Pipeline/OneTimeCommandPool.h"


namespace Jimara {
	/// <summary>
	/// Loaded font, providing drawable glyph atlasses
	/// </summary>
	class JIMARA_API Font : public virtual Resource {
	public:
		/// <summary> Character identifier </summary>
		using Glyph = wchar_t;

		/// <summary> Glyph and start UV position </summary>
		struct JIMARA_API GlyphPlacement;

		/// <summary> Line spacing information </summary>
		struct JIMARA_API LineSpacing;

		/// <summary> Relative glyph size and origin offset (all values are scaled down by the factor of font size) </summary>
		struct JIMARA_API GlyphShape;

		/// <summary> Information about a glyph, it's shape and UV rectangle </summary>
		struct JIMARA_API GlyphInfo;

		/// <summary> Font atlas with texture and UV-s (you need the Reader to access it's internals) </summary>
		class JIMARA_API Atlas;

		/// <summary> Atlass options </summary>
		enum class JIMARA_API AtlasFlags : uint16_t;

		/// <summary> Glyph UV and atlass reader (creating this freezes RequireGlyphs() calls, making it safe to read UV coordinates) </summary>
		class JIMARA_API Reader;

		/// <summary> Virtual destructor </summary>
		virtual ~Font();

		/// <summary>
		/// Gets or creates atlas based on the size and flags
		/// </summary>
		/// <param name="size"> Atlas size </param>
		/// <param name="flags"> Atlas flags </param>
		/// <returns> Atlas instance </returns>
		Reference<Atlas> GetAtlas(float size, AtlasFlags flags);

		/// <summary> Graphics device, the atlasses are created on </summary>
		inline Graphics::GraphicsDevice* GraphicsDevice()const { return m_graphicsDevice; }

		/// <summary> Line spacing information </summary>
		/// <param name="fontSize"> Font size (in pixels) </param>
		virtual LineSpacing GetLineSpacing(uint32_t fontSize) = 0;

		/// <summary>
		/// General size/offset information for given glyph
		/// <para/> Negative scale values mean the glyph load failed
		/// </summary>
		/// <param name="fontSize"> Font size (in pixels) </param>
		/// <param name="glyph"> Symbol </param>
		/// <returns> valid GlyphShape if glyph is valid or negative size if it fails </returns>
		virtual GlyphShape GetGlyphShape(uint32_t fontSize, const Glyph& glyph) = 0;

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
		/// <param name="fontSize"> Font size (in pixels) </param>
		/// <param name="glyphs"> List of glyphs and corresponding UV coordinates </param>
		/// <param name="glyphCount"> Number of glyphs </param>
		/// <param name="commandBuffer"> Command buffer for any graphics operations that may be needed within the backend </param>
		/// <returns> True, if nothing fails </returns>
		virtual bool DrawGlyphs(const Graphics::TextureView* targetImage, uint32_t fontSize,
			const GlyphPlacement* glyphs, size_t glyphCount, Graphics::CommandBuffer* commandBuffer) = 0;


	protected:
		/// <summary>
		/// Constructor
		/// </summary>
		/// <param name="device"> Graphics device </param>
		Font(Graphics::GraphicsDevice* device);


	private:
		// Graphics device
		const Reference<Graphics::GraphicsDevice> m_graphicsDevice;

		// One-time command buffer pool:
		const Reference<Graphics::OneTimeCommandPool> m_commandPool;

		// Atlas cache for each setting configuration
		const Reference<Object> m_atlasCache;

		// Lock for atlas creation
		std::mutex m_atlasCacheLock;

		// Private stuff is here..
		struct Helpers;
	};





	/// <summary> Glyph and start UV position </summary>
	struct JIMARA_API Font::GlyphPlacement {
		/// <summary> Symbol </summary>
		Glyph glyph = static_cast<Glyph>(0u);

		/// <summary> Placement rect </summary>
		SizeRect boundaries;
	};

	/// <summary> Line spacing information </summary>
	struct JIMARA_API Font::LineSpacing {
		/// <summary> Vertical distance from the baseline to the top Y coordinate of 'highest' character (in relative terms, scaled down by font size) </summary>
		float ascender = 1.0f;
		
		/// <summary> Vertical distance from the baseline to the bottom Y coordinate of 'lowest' character (in relative terms, scaled down by font size) </summary>
		float descender = 0.0f;

		/// <summary> Vertical distance between baselines (in relative terms, scaled down by font size) </summary>
		float lineHeight = 1.0f;
	};

	/// <summary> Relative glyph size and origin offset (all values are scaled down by the factor of font size) </summary>
	struct JIMARA_API Font::GlyphShape {
		/// <summary> Relative size scale of the glyph bitmap, compared to the font size value </summary>
		Vector2 size = Vector2(0.0f);

		/// <summary> Relative offset of glyph bitmap origin </summary>
		Vector2 offset = Vector2(0.0f);

		/// <summary> Relative width to advance the cursor with before hitting the next character </summary>
		float advance = 0.0f;
	};

	/// <summary> Information about a glyph, it's shape and UV rectangle </summary>
	struct JIMARA_API Font::GlyphInfo {
		Glyph glyph = static_cast<Glyph>(0u);
		GlyphShape shape;
		Rect boundaries = {};
	};





	/// <summary> Font atlas with texture and UV-s (you need the Reader to access it's internals) </summary>
	class JIMARA_API Font::Atlas : public virtual Object {
	public:
		/// <summary> Virtual destructor </summary>
		virtual ~Atlas();

		/// <summary> 'Underlying font' </summary>
		inline Jimara::Font* Font()const { return m_font; }

		/// <summary> Glyph size in pixels </summary>
		inline float Size()const { return float(m_size); }

		/// <summary> Atlas flags used during creation </summary>
		inline AtlasFlags Flags()const { return m_flags; }

		/// <summary> Line spacing information </summary>
		inline LineSpacing Spacing()const { return m_spacing; }

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
		/// Invoked each time glyph UV coordinates and atlast texture become outdated.
		/// <para/> Keep in mind, that even if you ignore this, your atlass will still work, but will be consuming memory unnecessarily.
		/// </summary>
		inline Event<Jimara::Font::Atlas*>& OnAtlasInvalidated() { return m_onAtlasInvalidated; }

	private:
		// Font
		const Reference<Jimara::Font> m_font;

		// Glyph size
		const uint32_t m_size;

		// Atlas flags
		const AtlasFlags m_flags;

		// Line spacing
		const LineSpacing m_spacing;

		// Underlying texture
		Reference<Graphics::TextureSampler> m_texture;

		// Invoked, whenever new glyphs get added, old atlass fills up and old atlasses get invalidated
		EventInstance<Jimara::Font::Atlas*> m_onAtlasInvalidated;

		// Lock for reading glyphs UV-s
		std::shared_mutex m_uvLock;

		// Cached glyph shape data; Kept constant after glyph addition;
		std::unordered_map<Glyph, GlyphShape> m_glyphShapes;

		// Glyph-to-UV map, recalculated after each atlas invalidation
		std::unordered_map<Glyph, GlyphInfo> m_glyphBounds;
		volatile float m_glyphUVSize = 1.0f;

		// Current glyph UV state (basically, tells that space is filled up to this coordinate)
		Vector2 m_filledUVptr = Vector2(0.0f);

		// Bottommost UV coordinate that any of the glyphs on currently filled row occupies
		float m_nextRowY = 0.0f;

		// Constructor is private!
		friend class Jimara::Font;
		friend struct Jimara::Font::Helpers;
		Atlas(Jimara::Font* font, uint32_t size, AtlasFlags flags);
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
		/// <param name="atlas"> Font atlas to read </param>
		Reader(Atlas* atlas);

		/// <summary> Destructor </summary>
		~Reader();

		/// <summary>
		/// Looks up glyph boundary
		/// </summary>
		/// <param name="glyph"> Symbol </param>
		/// <returns> Info of given glyph if found on the atlas; will not have value otherwise </returns>
		std::optional<GlyphInfo> GetGlyphInfo(const Glyph& glyph);

		/// <summary> Retrieves shared font atlas texture of given size </summary>
		Reference<Graphics::TextureSampler> GetTexture();

	private:
		std::shared_lock<std::shared_mutex> m_uvLock;
		const Reference<Atlas> m_atlas;
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
