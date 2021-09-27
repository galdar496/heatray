//
//  Log.h
//  Heatray
//
//  Utility to support logging.
//
//

#pragma once

#include "StringUtils.h"

#include <assert.h>
#include <memory>
#include <stdarg.h>
#include <string>

namespace util {
 
class Log {
public:
	virtual ~Log() = default;

	static std::shared_ptr<Log> instance() { return m_instance;  }

	enum class Type {
		kInfo,
		kError,

		kCount
	};
	void log(Type type, const char* format, ...) {
		va_list args;
        va_start(args, format);
        std::string formattedString = util::createStringWithFormat(format, args);
        va_end(args);

		m_instance->addNewItem(formattedString, type);
	}
protected:
	Log() = default;
	static void setInstance(std::shared_ptr<Log> instance) { m_instance = instance;  }
	virtual void addNewItem(const std::string& item, Type type) = 0;
private:
	static std::shared_ptr<Log> m_instance;
};

}  // namespace util.

#define LOG_INFO(format, ...) \
  util::Log::instance()->log(util::Log::Type::kInfo, format, ##__VA_ARGS__ )

#define LOG_ERROR(format, ...) \
  util::Log::instance()->log(util::Log::Type::kError, format, ##__VA_ARGS__ )
