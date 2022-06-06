#pragma once
#include <string>
#include <optional>
#include <string_view>
#include "../Logging/Logger.h"
#include "../../Core/Memory/MemoryBlock.h"


namespace Jimara {
	namespace OS {
		/// <summary>
		/// OS-level clipboard for copy-pasted
		/// <para/> Note: Depending on the operation system and the platform, 
		///		the actual implementation might not be possible and in that case,
		///		we'll have clipboard in RAM
		/// </summary>
		namespace Clipboard {
			/// <summary> Clears clipboard (for all types) </summary>
			/// <param name="logger"> Logger for error reporting (optional) </param>
			/// <returns> True, if successful </returns>
			bool Clear(Logger* logger = nullptr);

			/// <summary>
			/// Stores the text in the clipboard
			/// </summary>
			/// <param name="text"> Text to store </param>
			/// <param name="logger"> Logger for error reporting (optional) </param>
			/// <returns> True, if successful </returns>
			bool SetText(std::string_view text, Logger* logger = nullptr);

			/// <summary>
			/// Retrieves text from the clipboard
			/// </summary>
			/// <param name="logger"> Logger for error reporting (optional) </param>
			/// <returns> Text from the clipboard (if available) </returns>
			std::optional<std::string> GetText(Logger* logger = nullptr);

			/// <summary>
			/// Stores custom data in the clipboard
			/// </summary>
			/// <param name="typeId"> Unique type identifier for the user data type </param>
			/// <param name="data"> User data bytes (will not set, if data is nullptr or size is 0) </param>
			/// <param name="logger"> Logger for error reporting (optional) </param>
			/// <returns> True, if successful </returns>
			bool SetData(std::string_view typeId, MemoryBlock data, Logger* logger = nullptr);

			/// <summary>
			/// Retrieves custom data from the clipboard
			/// </summary>
			/// <param name="typeId"> Unique type identifier for the user data type </param>
			/// <param name="logger"> Logger for error reporting (optional) </param>
			/// <returns> Custom data if stored, otherwise, MemoryBlock will be empty (nullptr with size 0) </returns>
			MemoryBlock GetData(std::string_view typeId, Logger* logger = nullptr);
		}
	}
}
