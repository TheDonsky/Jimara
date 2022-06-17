#include "Clipboard.h"
#include <clip/clip.h>
#pragma warning(disable: 26451)
#pragma warning(disable: 6011)
#pragma warning(disable: 4267)
#pragma warning(disable: 4018)
#pragma warning(disable: 26439)
#pragma warning(disable: 4244)
#include <clip/clip.cpp>
#include <clip/image.cpp>
#ifdef _WIN32
#pragma comment(lib, "Shlwapi.lib")
#include <clip/clip_win.cpp>
#else
#include <clip/clip_x11.cpp>
#endif
#include <mutex>
#include <unordered_map>
#include <vector>
#pragma warning(default: 26451)
#pragma warning(default: 6011)
#pragma warning(default: 4267)
#pragma warning(default: 4018)
#pragma warning(default: 26439)
#pragma warning(default: 4244)

namespace Jimara {
	namespace OS {
		namespace Clipboard {
			namespace {
				std::mutex& ClipboardLock() {
					static std::mutex lock;
					return lock;
				}
				
				clip::format GetFormat(const std::string& typeId) {
					static std::unordered_map<std::string, clip::format> formats;
					clip::format rv;
					decltype(formats)::iterator it = formats.find(typeId);
					if (it != formats.end()) rv = it->second;
					else {
						rv = clip::register_format(typeId);
						formats[typeId] = rv;
					}
					return rv;
				}

				class ClipboardDataBuffer : public virtual Object, public virtual std::vector<char> {};
			}

			bool Clear(Logger* logger) {
				std::unique_lock<std::mutex> lock(ClipboardLock());
				if (clip::clear()) return true;
				else {
					if (logger != nullptr)
						logger->Error("OS::Clipboard::Clear - clip::clear() Failed! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}
			}

			bool SetText(std::string_view text, Logger* logger) {
				std::unique_lock<std::mutex> lock(ClipboardLock());
				clip::lock clipboard;
				if (!clipboard.locked()) {
					if (logger != nullptr)
						logger->Error("OS::Clipboard::SetText - Failed to lock clipboard! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}
				if (clipboard.set_data(clip::text_format(), text.data(), text.size())) return true;
				else {
					if (logger != nullptr)
						logger->Error("OS::Clipboard::SetText - clip::set_text() Failed! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}
			}

			std::optional<std::string> GetText(Logger*) {
				std::unique_lock<std::mutex> lock(ClipboardLock());
				std::string rv;
				if (clip::get_text(rv)) return rv;
				else return std::optional<std::string>();
			}

			bool SetData(std::string_view typeId, MemoryBlock data, Logger* logger) {
				std::unique_lock<std::mutex> lock(ClipboardLock());
				if (data.Data() == nullptr || data.Size() <= 0) return true;

				const clip::format format = GetFormat((std::string)typeId);
				if (format == clip::empty_format()) {
					if (logger != nullptr)
						logger->Error(
							"OS::Clipboard::SetData - Failed to get format for <", typeId, ">!",
							" [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}

				clip::lock clipboard;
				
				if (!clipboard.locked()) {
					if (logger != nullptr)
						logger->Error("OS::Clipboard::SetData - Failed to lock clipboard! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}

				if (clipboard.set_data(format, reinterpret_cast<const char*>(data.Data()), data.Size()))
					return true;
				else {
					if (logger != nullptr)
						logger->Error("OS::Clipboard::SetData - clipboard.set_data() failed! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return false;
				}
			}

			MemoryBlock GetData(std::string_view typeId, Logger* logger) {
				std::unique_lock<std::mutex> lock(ClipboardLock());
				const clip::format format = GetFormat((std::string)typeId);
				if (format == clip::empty_format()) {
					if (logger != nullptr)
						logger->Error(
							"OS::Clipboard::GetData - Failed to get format for <", typeId, ">!",
							" [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return MemoryBlock(nullptr, 0, nullptr);
				}

				clip::lock clipboard;
				if (!clipboard.locked()) {
					if (logger != nullptr)
						logger->Error("OS::Clipboard::GetData - Failed to lock clipboard! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return MemoryBlock(nullptr, 0, nullptr);
				}

				size_t size = clipboard.get_data_length(format);
				if (size <= 0)
					return MemoryBlock(nullptr, 0, nullptr);

				Reference<ClipboardDataBuffer> buffer = Object::Instantiate<ClipboardDataBuffer>();
				buffer->resize(size);

				if (clipboard.get_data(format, buffer->data(), buffer->size()))
					return MemoryBlock(buffer->data(), buffer->size(), buffer);
				else {
					if (logger != nullptr)
						logger->Error("OS::Clipboard::GetData - clipboard.get_data() failed! [File: ", __FILE__, "; Line: ", __LINE__, "]");
					return MemoryBlock(nullptr, 0, nullptr);
				}
			}
		}
	}
}
