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
					FT_Error error = FT_New_Memory_Face(*library,
						reinterpret_cast<const FT_Byte*>(memory.Data()), memory.Size(),
						faceIndex, &face);
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

		// __TODO__: Implement this crap!
		return fail("Not implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
	}

	FreetypeFont::~FreetypeFont() {
		// __TODO__: Implement this crap!
	}

	float FreetypeFont::PrefferedAspectRatio(const Glyph& glyph) {
		// __TODO__: Implement this crap!
		GraphicsDevice()->Log()->Error("FreetypeFont::PrefferedAspectRatio - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		return 1.0f;
	}

	bool FreetypeFont::DrawGlyphs(const Graphics::TextureView* targetImage, const GlyphInfo* glyphs, size_t glyphCount, Graphics::CommandBuffer* commandBuffer) {
		// __TODO__: Implement this crap!
		GraphicsDevice()->Log()->Error("FreetypeFont::DrawGlyphs - Not yet implemented! [File: ", __FILE__, "; Line: ", __LINE__, "]");
		return false;
	}
}
