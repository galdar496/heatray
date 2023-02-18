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
#include <string>

namespace util {
 
class Log {
public:
    virtual ~Log() = default;

    static std::shared_ptr<Log> instance() { return m_instance;  }

    enum class Type {
        kInfo,
        kWarning,
        kError,

        kCount
    };

    //-------------------------------------------------------------------------
    // Log a message to the installed logger.
    template <class ... Args>
    void log(Type type, const char* format, Args &&... args) {
        std::string formattedString = util::createStringWithFormat(format, args...);
        m_instance->addNewItem(formattedString, type);
    }
protected:
    Log() = default;
    static void setInstance(std::shared_ptr<Log> instance) { m_instance = instance;  }
    virtual void addNewItem(const std::string& item, const Type type) = 0;
private:
    static std::shared_ptr<Log> m_instance;
};

}  // namespace util.

//-------------------------------------------------------------------------
// Logging macros to log specific types of messages.

#define LOG_INFO(format, ...) \
  util::Log::instance()->log(util::Log::Type::kInfo, format, ##__VA_ARGS__ )

#define LOG_WARNING(format, ...) \
  util::Log::instance()->log(util::Log::Type::kWarning, format, ##__VA_ARGS__ )

#define LOG_ERROR(format, ...) \
  util::Log::instance()->log(util::Log::Type::kError, format, ##__VA_ARGS__ )
