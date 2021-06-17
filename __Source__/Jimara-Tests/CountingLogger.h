#pragma once
#include "OS/Logging/StreamLogger.h"
#include <cassert>


namespace Jimara {
	namespace Test {
		class CountingLogger : public virtual OS::Logger {
		private:
			const Reference<OS::Logger> m_logger;
			std::atomic<size_t> m_debug = 0;
			std::atomic<size_t> m_info = 0;
			std::atomic<size_t> m_warning = 0;
			std::atomic<size_t> m_error = 0;
			std::atomic<size_t> m_fatal = 0;
			std::atomic<size_t> m_nonSpecified = 0;

		public:
			inline CountingLogger(OS::Logger* logger = nullptr) 
				: m_logger(logger == nullptr ? Reference<OS::Logger>(Object::Instantiate<OS::StreamLogger>()) : Reference<OS::Logger>(logger)) {}

			size_t NumDebug()const { return m_debug; }

			size_t NumInfo()const { return m_info; }

			size_t NumWarning()const { return m_warning; }

			size_t NumError()const { return m_error; }

			size_t NumFatal()const { return m_fatal; }

			size_t Numfailures()const { return m_error + m_fatal; }

			size_t NumUnsafe()const { return m_warning + Numfailures(); }

		protected:
			virtual void Log(const LogInfo& info) override {
				std::atomic<size_t>& cnt =
					(info.level == LogLevel::LOG_DEBUG) ? m_debug :
					(info.level == LogLevel::LOG_INFO) ? m_info :
					(info.level == LogLevel::LOG_WARNING) ? m_warning :
					(info.level == LogLevel::LOG_ERROR) ? m_error :
					(info.level == LogLevel::LOG_FATAL) ? m_fatal : m_nonSpecified;
				cnt++;
				assert(m_nonSpecified == 0);
				m_logger->Log(info.level, info.message);
			}
		};
	}
}

