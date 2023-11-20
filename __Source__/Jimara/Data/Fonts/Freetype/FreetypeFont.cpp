#include "FreetypeFont.h"
#include <ft2build.h>
#include FT_FREETYPE_H


namespace Jimara {
	struct FreetypeFont::Helpers {
		class Library : public virtual Object {
		private:
			static_assert(std::is_pointer_v<FT_Library>);
			const Reference<OS::Logger> m_log;
			const FT_Library m_library;
			std::mutex m_lock;

			inline Library(OS::Logger* log, FT_Library lib)
				: m_log(log), m_library(lib) {
				assert(m_log != nullptr);
				assert(m_library != nullptr);
			}

		public:
			inline static Reference<Library> Create(OS::Logger* log) {
				assert(log != nullptr);
				FT_Library lib = nullptr;
				FT_Error error = FT_Init_FreeType(&lib);
				if (error) {
					log->Error("FreetypeFont::Helpers::Library::Create - FT_Init_FreeType failed with code ", error,
						"! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return nullptr;
				}
				Reference<Library> library = new Library(log, lib);
				library->ReleaseRef();
				return library;
			}

			inline virtual ~Library() {
				FT_Error error = FT_Done_FreeType(m_library);
				if (error)
					m_log->Error("FreetypeFont::Helpers::Library::~Library - FT_Done_FreeType failed with code ", error,
						"! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline operator const FT_Library& ()const { return m_library; }
			inline std::mutex& Lock() { return m_lock; }
			inline OS::Logger* Log()const { return m_log; }
		};





		class Face : public virtual Object {
		private:
			static_assert(std::is_pointer_v<FT_Face>);
			const FT_Face m_face;
			const Reference<Library> m_library;
			const MemoryBlock m_memory;
			std::mutex m_lock;

			inline Face(FT_Face face, Library* library, const MemoryBlock& memory)
				: m_face(face), m_library(library), m_memory(memory) {
				assert(m_face != nullptr);
				assert(m_library != nullptr);
				assert(m_memory.Data() != nullptr && m_memory.Size() > 0u);
			}

		public:
			inline static Reference<Face> Create(Library* library, const MemoryBlock& memory, FT_Long faceIndex) {
				if (library == nullptr)
					return nullptr;
				auto fail = [&](const auto&... message) {
					library->Log()->Error("FreetypeFont::Helpers::Face::Create - ", message...);
					return nullptr;
					};
				if (memory.Data() == nullptr || memory.Size() <= 0u)
					return fail("Empty memory block provided! [File:", __FILE__, "; Line: ", __LINE__, "]");

				FT_Face face = nullptr;
				{
					std::unique_lock<std::mutex> lock(library->Lock());
					if (static_cast<FT_Long>(memory.Size()) != memory.Size())
						return fail("static_cast<FT_Long>(memory.Size()) != memory.Size()! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					FT_Error error = FT_New_Memory_Face(*library,
						reinterpret_cast<const FT_Byte*>(memory.Data()), static_cast<FT_Long>(memory.Size()),
						static_cast<FT_Long>(faceIndex), &face);
					if (error)
						return fail("FT_Done_Face failed with code ", error, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				};

				const Reference<Face> rv = new Face(face, library, memory);
				rv->ReleaseRef();
				return rv;
			}

			virtual inline ~Face() {
				std::unique_lock<std::mutex> lock(m_library->Lock());
				FT_Error error = FT_Done_Face(m_face);
				if (error)
					m_library->Log()->Error("FreetypeFont::Helpers::Face::~Face - FT_Done_Face failed with code ", error,
						"! [File: ", __FILE__, "; Line: ", __LINE__, "]");
			}

			inline operator const FT_Face& ()const { return m_face; }
			inline std::mutex& Lock() { return m_lock; }
			inline Library* Lib()const { return m_library; }
		};

		inline static uint32_t QueryFaceCount(Library* library, const MemoryBlock& memory) {
			if (library == nullptr)
				return 0u;
			const Reference<Face> face = Face::Create(library, memory, -1);
			if (face == nullptr) {
				library->Log()->Error("FreetypeFont::Helpers::Face::QueryFaceCount - Failed to create dummy face! [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return 0u;
			}
			const uint32_t rv = static_cast<uint32_t>(face->operator const FT_Face & ()->num_faces);
			return rv;
		}

		inline static bool SetGlyphSize(Face* face, uint32_t size, volatile uint32_t& lastKnownSize) {
			if (size == lastKnownSize)
				return true;
			const FT_Error error = FT_Set_Pixel_Sizes(*face, 0, size);
			if (error) {
				face->Lib()->Log()->Error("FreetypeFont::Helpers::SetGlyphSize - Failed to set font pixel size to ", size,
					"! (FT_Set_Pixel_Sizes error code ", error, ") [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return false;
			}
			lastKnownSize = size;
			return true;
		}

		inline static bool LoadGlyph(Face* face, const Font::Glyph& glyph) {
			const FT_UInt glyphIndex = FT_Get_Char_Index(*face, glyph);
			const FT_Error error = FT_Load_Glyph(*face, glyphIndex, FT_LOAD_DEFAULT);
			if (error) {
				face->Lib()->Log()->Error("FreetypeFont::Helpers::LoadGlyph - Failed to load glyph ", glyph, "(", glyphIndex, ")"
					"! (FT_Load_Glyph error code ", error, ") [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return false;
			}
			return true;
		}
	};





	Reference<FreetypeFont> FreetypeFont::Create(const MemoryBlock& fontBinary, uint32_t faceIndex, Graphics::GraphicsDevice* device) {
		if (device == nullptr)
			return nullptr;
		auto fail = [&](const auto&... message) { 
			device->Log()->Error("FreetypeFont::Create - ", message...); 
			return nullptr; 
		};

		const Reference<Helpers::Library> library = Helpers::Library::Create(device->Log());
		if (library == nullptr)
			return fail("Could not allocate Freetype Library object! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<Helpers::Face> face = Helpers::Face::Create(library, fontBinary, static_cast<FT_Long>(faceIndex));
		if (face == nullptr)
			return fail("Could not create Freetype face! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<FreetypeFont> font = new FreetypeFont(device, fontBinary, face);
		font->ReleaseRef();

		// __TODO__: Implement this crap!
		device->Log()->Warning(
			"FreetypeFont::Create - Additional face vaildation required ",
			"(for scalability and charmaps; Also the internal test with addition of symbols should be removed)! ",
			"[File: ", __FILE__, "; Line: ", __LINE__, "]");

		font->RequireGlyphs(L"abcdefgh");
		const Reference<Font::Atlas> atlas = font->GetAtlas(64, Font::AtlasFlags::CREATE_UNIQUE);

		return font;
	}

	FreetypeFont::FreetypeFont(Graphics::GraphicsDevice* device, const MemoryBlock& binary, Object* face)
		: Font(device)
		, m_face(face)
		, m_fontBinary(binary) {
		assert(dynamic_cast<Helpers::Face*>(m_face.operator->()) != nullptr);
	}

	FreetypeFont::~FreetypeFont() {
		// __TODO__: Implement this crap!
	}

	float FreetypeFont::PrefferedAspectRatio(const Glyph& glyph) {
		Helpers::Face* const face = dynamic_cast<Helpers::Face*>(m_face.operator->());
		assert(face != nullptr);
		std::unique_lock<std::mutex> lock(face->Lock());
		auto fail = [&](const auto&... message) {
			GraphicsDevice()->Log()->Error("FreetypeFont::PrefferedAspectRatio - ", message...);
			return 0.0f;
		};

		const constexpr uint32_t height = 16u;

		if (!Helpers::SetGlyphSize(face, height, m_lastSize))
			return fail("Failed to set nominal glyph size! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		if (!Helpers::LoadGlyph(face, glyph)) 
			return fail("Failed to load glyph! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const uint32_t width = static_cast<uint32_t>(face->operator const FT_Face & ()->glyph->advance.x >> 6u);
		return static_cast<float>(width) / static_cast<float>(height);
	}

	bool FreetypeFont::DrawGlyphs(const Graphics::TextureView* targetImage, const GlyphInfo* glyphs, size_t glyphCount, Graphics::CommandBuffer* commandBuffer) {
		// __TODO__: Implement this crap!
		Helpers::Face* const face = dynamic_cast<Helpers::Face*>(m_face.operator->());
		assert(face != nullptr);
		std::unique_lock<std::mutex> lock(face->Lock());

		const Vector2 imageSize = targetImage->TargetTexture()->Size();
		const GlyphInfo* const glyphEnd = glyphs + glyphCount;
		for (const GlyphInfo* glyphPtr = glyphs; glyphPtr < glyphEnd; glyphPtr++) {
			// Get boundary rect:
			const Vector2 startPos = glyphPtr->boundaries.start * imageSize;
			const Vector2 endPos = glyphPtr->boundaries.end * imageSize;
			if (startPos.x < 0.0f || startPos.x >= endPos.x ||
				startPos.y < 0.0f || startPos.y >= endPos.y) {
				GraphicsDevice()->Log()->Error(
					"FreetypeFont::DrawGlyphs - Boundary for '", glyphPtr->glyph,
					"' ([", glyphPtr->boundaries.start.x, ";", glyphPtr->boundaries.start.y, "] - ",
					"[", glyphPtr->boundaries.end.x, "; ", glyphPtr->boundaries.end.y, "]) not supported!",
					" [File: ", __FILE__, "; Line: ", __LINE__, "]");
				continue;
			}
			
			// Update glyph size:
			const Size2 glyphSize = endPos - startPos;
			if (!Helpers::SetGlyphSize(face, glyphSize.y, m_lastSize))
				continue;

			// Load glyph:
			if (!Helpers::LoadGlyph(face, glyphPtr->glyph))
				continue;

			// Render glyph:
			{
				const FT_Error error = FT_Render_Glyph(face->operator const FT_Face & ()->glyph, FT_RENDER_MODE_NORMAL);
				if (error) {
					GraphicsDevice()->Log()->Error("FreetypeFont::DrawGlyphs - Failed to render glyph ", glyphPtr->glyph,
						"! (FT_Render_Glyph error code ", error, ") [File: ", __FILE__, "; Line: ", __LINE__, "]");
					continue;
				}
			}

			// Transfer glyph data:
			{
				FT_GlyphSlot glyph = face->operator const FT_Face & ()->glyph;
				const FT_Bitmap& bitmap = glyph->bitmap;
				if (bitmap.buffer == nullptr || bitmap.rows <= 0u || bitmap.width <= 0u)
					continue;
				//const uint32_t advance_x = 
				if (glyph->bitmap_left < 0)
					continue;
			}
		}

		GraphicsDevice()->Log()->Error("FreetypeFont::DrawGlyphs - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		return false;
	}
}
