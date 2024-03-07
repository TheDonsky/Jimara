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

		inline static bool SetPixelSize(Face* face, uint32_t size, volatile uint32_t& lastKnownSize) {
			if (size == lastKnownSize)
				return true;
			const FT_Error error = FT_Set_Pixel_Sizes(*face, 0, size);
			if (error) {
				face->Lib()->Log()->Error("FreetypeFont::Helpers::SetPixelSize - Failed to set font pixel size to ", size,
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
				const std::string glyphStr = Convert<std::string>(std::wstring_view(&glyph, 1u));
				face->Lib()->Log()->Error("FreetypeFont::Helpers::LoadGlyph - Failed to load glyph ", glyphStr, "(", glyphIndex, ")"
					"! (FT_Load_Glyph error code ", error, ") [File: ", __FILE__, "; Line: ", __LINE__, "]");
				return false;
			}
			return true;
		}

		class NewGlyphAtlas {
		public:
			struct Placement {
				Size2 atlasPos = {};
				Size2 targetPos = {};
				Size2 regionSize = {};
				Font::Glyph glyph = {};
			};

		private:
			Stacktor<Placement, 4u> m_placements;
			Size2 m_ptr = Size2(0u, 0u);
			bool m_advanceHorizontal = true;
			Size2 m_canvasSize = Size2(0u, 0u);

		public:
			inline void AddGlyph(const Font::Glyph& glyph, const Size2 targetPos, const Size2& regionSize) {
				// Add placement:
				{
					Placement placement = {};
					placement.atlasPos = m_ptr;
					placement.targetPos = targetPos;
					placement.regionSize = regionSize;
					placement.glyph = glyph;
					m_placements.Push(placement);
				}

				// Update canvas size:
				{
					m_canvasSize.x = Math::Max(m_canvasSize.x, m_ptr.x + regionSize.x);
					m_canvasSize.y = Math::Max(m_canvasSize.y, m_ptr.y + regionSize.y);
				}
				
				// Move pointer:
				if (m_advanceHorizontal) {
					m_ptr.x += regionSize.x;
					if (m_ptr.x >= m_canvasSize.x) {
						assert(m_ptr.x == m_canvasSize.x);
						m_ptr.y = 0u;
						m_advanceHorizontal = false;
					}
				}
				else {
					m_ptr.y += regionSize.y;
					if (m_ptr.y >= m_canvasSize.y) {
						assert(m_ptr.y == m_canvasSize.y);
						m_ptr.x = 0u;
						m_advanceHorizontal = true;
					}
				}
			}

			inline size_t Count()const { return m_placements.Size(); }
			inline const Placement& operator[](size_t index)const { return m_placements[index]; }
			inline Size2 CanvasSize()const { return m_canvasSize; }
		};

		inline static bool CopyTexture(
			const FT_GlyphSlot& glyph, const Size2& dstOffset,
			void* dstBuffer, Graphics::Texture::PixelFormat dstFormat, uint32_t dstStride, const Size2& dstSize, 
			OS::Logger* log) {
			
			const FT_Bitmap& bitmap = glyph->bitmap;
			if (bitmap.buffer == nullptr || bitmap.rows <= 0u || bitmap.width <= 0u)
				return false;

			auto fail = [&](const auto&... message) { 
				log->Error("FreetypeFont::Helpers::CopyTexture - ", message...);
				return false;
			};

			if (dstFormat != Graphics::Texture::PixelFormat::R8_SRGB && dstFormat != Graphics::Texture::PixelFormat::R8_UNORM)
				return fail("Only single channel 8 bit formats are supported (R8_SRGB & R8_UNORM)! Got ",
					static_cast<std::underlying_type_t<decltype(dstFormat)>>(dstFormat), "! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			if (bitmap.num_grays != 256 || bitmap.pixel_mode != FT_PIXEL_MODE_GRAY)
				return fail("FT_Bitmap pixel mode not supported! Got ", bitmap.pixel_mode, "! [File: ", __FILE__, "; Line: ", __LINE__, "]");

			const int32_t startPosX = static_cast<int32_t>(dstOffset.x);
			const int32_t startPosY = static_cast<int32_t>(dstOffset.y);

			const Size2 dstStartPos = Size2(
				static_cast<uint32_t>(Math::Max(0, startPosX)),
				static_cast<uint32_t>(Math::Max(0, startPosY)));

			const Size2 bitmapOffset = Size2(
				static_cast<uint32_t>(static_cast<int32_t>(dstStartPos.x) - startPosX),
				static_cast<uint32_t>(static_cast<int32_t>(dstStartPos.y) - startPosY));
			if (bitmapOffset.x >= bitmap.width)
				return true;

			static_assert(sizeof(char) == 1u);
			const size_t rowCopySize = Math::Min(dstSize.x - Math::Min(dstSize.x, dstStartPos.x), bitmap.width - bitmapOffset.x) * sizeof(char);
			
			uint32_t dstRow = dstStartPos.y;
			uint32_t bitmapRow = bitmapOffset.y;
			while (dstRow < dstSize.y && bitmapRow < bitmap.rows) {
				void* dst = reinterpret_cast<char*>(dstBuffer) + ((dstRow * dstStride) + dstStartPos.x);
				const void* src = reinterpret_cast<char*>(bitmap.buffer) + ((bitmapRow * bitmap.pitch) + bitmapOffset.x);
				std::memcpy(dst, src, rowCopySize);
				dstRow++;
				bitmapRow++;
			}

			return true;
		}

		inline static float FtToPixelSize(FT_Pos size) {
			return static_cast<float>(size) * (1.0f / 64.0f);
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

		if (!FT_IS_SCALABLE(face->operator const FT_Face &()))
			return fail("Non-scalable fonts are not supported! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const Reference<FreetypeFont> font = new FreetypeFont(device, fontBinary, face);
		font->ReleaseRef();
		return font;
	}

	FreetypeFont::FreetypeFont(Graphics::GraphicsDevice* device, const MemoryBlock& binary, Object* face)
		: Font(device)
		, m_face(face)
		, m_fontBinary(binary) {
		assert(dynamic_cast<Helpers::Face*>(m_face.operator->()) != nullptr);
	}

	FreetypeFont::~FreetypeFont() {}

	Font::LineSpacing FreetypeFont::GetLineSpacing(uint32_t fontSize) {
		Helpers::Face* ftFace = dynamic_cast<Helpers::Face*>(m_face.operator->());
		std::unique_lock<std::mutex> lock(ftFace->Lock());
		uint32_t lastSize = ~uint32_t(0u);
		if (!Helpers::SetPixelSize(ftFace, fontSize, lastSize)) {
			GraphicsDevice()->Log()->Error(
				"FreetypeFont::GetLineSpacing - Failed to set font size to calculate line height! ",
				"[File: ", __FILE__, "; Line: ", __LINE__, "]");
			return LineSpacing();
		}
		else {
			const FT_Face& type = *ftFace;
			const float scale = float(type->size->metrics.y_ppem);
			LineSpacing rv = {};
			rv.ascender = Helpers::FtToPixelSize(type->size->metrics.ascender) / scale;
			rv.descender = Helpers::FtToPixelSize(type->size->metrics.descender) / scale;
			rv.lineHeight = Helpers::FtToPixelSize(type->size->metrics.height) / scale;
			return rv;
		}
	}

	Font::GlyphShape FreetypeFont::GetGlyphShape(uint32_t fontSize, const Glyph& glyph) {
		Helpers::Face* const face = dynamic_cast<Helpers::Face*>(m_face.operator->());
		assert(face != nullptr);
		GlyphShape rv = {};

		std::unique_lock<std::mutex> lock(face->Lock());
		auto fail = [&](const auto&... message) {
			GraphicsDevice()->Log()->Error("FreetypeFont::PrefferedAspectRatio - ", message...);
			rv.size = Vector2(-1.0f);
			return rv;
		};

		if (!Helpers::SetPixelSize(face, fontSize, m_lastSize))
			return fail("Failed to set glyph size! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		if (!Helpers::LoadGlyph(face, glyph)) 
			return fail("Failed to load glyph! [File: ", __FILE__, "; Line: ", __LINE__, "]");

		const FT_Face& ftFace = *face;
		const float x_ppem = float(ftFace->size->metrics.x_ppem);
		const float y_ppem = float(ftFace->size->metrics.y_ppem);
		const FT_GlyphSlot& slot = ftFace->glyph;
		rv.size = Vector2(
			Helpers::FtToPixelSize(slot->metrics.width) / x_ppem,
			Helpers::FtToPixelSize(slot->metrics.height) / y_ppem);
		rv.offset = Vector2(
			Helpers::FtToPixelSize(slot->metrics.horiBearingX) / x_ppem,
			Helpers::FtToPixelSize(slot->metrics.horiBearingY - slot->metrics.height) / y_ppem);
		rv.advance = Helpers::FtToPixelSize(slot->metrics.horiAdvance) / x_ppem;
		return rv;
	}

	bool FreetypeFont::DrawGlyphs(const Graphics::TextureView* targetImage, uint32_t fontSize, 
		const GlyphPlacement* glyphs, size_t glyphCount, Graphics::CommandBuffer* commandBuffer) {
		if (targetImage == nullptr || glyphs == nullptr || glyphCount <= 0u || commandBuffer == nullptr)
			return false;

		Helpers::Face* const face = dynamic_cast<Helpers::Face*>(m_face.operator->());
		assert(face != nullptr);
		std::unique_lock<std::mutex> lock(face->Lock());

		// Populate glyphs on a virtual atlas:
		Helpers::NewGlyphAtlas glyphAtlasses;
		{
			const Vector2 imageSize = targetImage->TargetTexture()->Size();
			const GlyphPlacement* const glyphEnd = glyphs + glyphCount;
			for (const GlyphPlacement* glyphPtr = glyphs; glyphPtr < glyphEnd; glyphPtr++) {
				const Size2 startPos = glyphPtr->boundaries.start;
				const Size2 endPos = glyphPtr->boundaries.end;
				if (startPos.x < 0.0f || startPos.x >= endPos.x ||
					startPos.y < 0.0f || startPos.y >= endPos.y)
					continue;
				glyphAtlasses.AddGlyph(glyphPtr->glyph, startPos, endPos - startPos);
			}
		}

		// Create temporary texture:
		if (glyphAtlasses.CanvasSize().x <= 0u || glyphAtlasses.CanvasSize().y <= 0u)
			return true;
		const Graphics::Texture::PixelFormat bufferFormat = targetImage->TargetTexture()->ImageFormat();
		const Reference<Graphics::ArrayBuffer> stagingTexture = GraphicsDevice()->CreateArrayBuffer(
			Graphics::Texture::TexelSize(bufferFormat),
			size_t(glyphAtlasses.CanvasSize().x * glyphAtlasses.CanvasSize().y),
			Graphics::ArrayBuffer::CPUAccess::CPU_READ_WRITE);
		if (stagingTexture == nullptr) {
			GraphicsDevice()->Log()->Error(
				"FreetypeFont::DrawGlyphs - failed to create temporary CPU texture for transfering glyph data!",
				" [File: ", __FILE__, "; Line: ", __LINE__, "]");
			return false;
		}
		void* const bufferData = stagingTexture->Map();
		const uint32_t bufferPitch = glyphAtlasses.CanvasSize().x;

		// Zero out temporary texture:
		std::memset(bufferData, 0, size_t(bufferPitch) * glyphAtlasses.CanvasSize().y);

		// Update glyph size:
		if (!Helpers::SetPixelSize(face, fontSize, m_lastSize))
			return false;

		for (size_t i = 0u; i < glyphAtlasses.Count(); i++) {
			const Helpers::NewGlyphAtlas::Placement& placement = glyphAtlasses[i];

			// Load glyph:
			if (!Helpers::LoadGlyph(face, placement.glyph))
				continue;

			// Render glyph:
			{
				const FT_Error error = FT_Render_Glyph(face->operator const FT_Face & ()->glyph, FT_RENDER_MODE_NORMAL);
				if (error) {
					const std::string glyphStr = Convert<std::string>(std::wstring_view(&placement.glyph, 1u));
					GraphicsDevice()->Log()->Error("FreetypeFont::DrawGlyphs - Failed to render glyph ", glyphStr,
						"! (FT_Render_Glyph error code ", error, ") [File: ", __FILE__, "; Line: ", __LINE__, "]");
					continue;
				}
			}

			// Transfer glyph data:
			if (Helpers::CopyTexture(face->operator const FT_Face & ()->glyph, placement.atlasPos,
				bufferData, bufferFormat, bufferPitch, glyphAtlasses.CanvasSize(), GraphicsDevice()->Log()))
				targetImage->TargetTexture()->Copy(commandBuffer, stagingTexture,
					Size3(glyphAtlasses.CanvasSize(), 1u),
					Size3(placement.targetPos, 0u), Size3(placement.atlasPos, 0u),
					Size3(placement.regionSize.x, placement.regionSize.y, 1u));
		}

		// Unmap texture memory:
		stagingTexture->Unmap(true);

		return true;
	}
}
