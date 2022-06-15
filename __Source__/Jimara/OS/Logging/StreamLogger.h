#pragma once
#include "Logger.h"
#include <iostream>


namespace Jimara {
	namespace OS {
		/// <summary>
		/// Logger, that outputs to some stream
		/// </summary>
		class JIMARA_API StreamLogger : public Logger {
		public:
			/// <summary>
			/// Constructor
			/// </summary>
			/// <param name="minLogLevel"> Minimal log level that should not be ignored </param>
			/// <param name="stream"> Output stream </param>
			/// <param name="useColors"> If true, stream logger will try to use console colors </param>
			StreamLogger(LogLevel minLogLevel = LogLevel::LOG_DEBUG, std::ostream& stream = std::cout, bool useColors = true);

		protected:
			/// <summary>
			/// Outputs log on stream
			/// </summary>
			/// <param name="info"> Log information </param>
			virtual void Log(const LogInfo& info) override;

		private:
			// Underlying stream
			std::ostream* m_stream;

			// If true, stream logger will try to use console colors
			bool m_useColors;
		};
	}
}
