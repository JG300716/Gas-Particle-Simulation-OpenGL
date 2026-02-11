#pragma once

#include <string>

namespace Logger {

	void init(const std::string& filename = "simulation.log");

	void log(const char* tag, const std::string& msg);
	void logWithTime(const char* tag, const std::string& msg);
	void shutdown();

}
