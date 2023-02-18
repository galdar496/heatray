#include "ConsoleLog.h"

#include <iostream>

namespace util {

void ConsoleLog::addNewItem(const std::string_view item, const Log::Type type)
{
    switch (type) {
        case Log::Type::kInfo:
            std::cout << "INFO: " << item << std::endl;
            break;
        case Log::Type::kWarning:
            std::cout << "WARNING: " << item << std::endl;
            break;
        case Log::Type::kError:
            std::cout << "ERROR: " << item << std::endl;
            break;
        default:
            break;
    }
}

} // namespace util.
