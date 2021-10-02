//
//  Console.h
//  Heatray
//
//  Log specific for a debug console.
//
//

#pragma once

#include "Log.h"

namespace util {
 
class ConsoleLog final : public Log {
public:
	virtual ~ConsoleLog() = default;

	static void install() {
		ConsoleLog* log = new ConsoleLog();
		std::shared_ptr<ConsoleLog> newLog(log);
		Log::setInstance(newLog);
	}

	void addNewItem(const std::string& item, Log::Type type) override;

private:
	ConsoleLog() = default;
};

}  // namespace util.

