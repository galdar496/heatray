#include "ConsoleLog.h"

#include <iostream>

namespace util {

void ConsoleLog::addNewItem(const std::string& item, Log::Type type)
{
    switch (type) {
        case Log::Type::kInfo:
            std::cout << "INFO: " << item << std::endl;
            break;
        case Log::Type::kError:
            std::cout << "ERROR: " << item << std::endl;
            break;
        default:
            break;
    }
}

} // namespace util.