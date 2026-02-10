#include "Logger.h"
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#ifdef _WIN32
#include <Windows.h>
#endif

namespace Logger {

static std::ofstream s_file;

static std::string defaultLogPath() {
#ifdef _WIN32
    char path[MAX_PATH];
    if (GetModuleFileNameA(NULL, path, MAX_PATH) != 0) {
        std::string s(path);
        size_t pos = s.find_last_of("\\/");
        if (pos != std::string::npos) s.resize(pos + 1);
        return s + "simulation.log";
    }
#endif
    return "simulation.log";
}

void init(const std::string& filename) {
    if (s_file.is_open()) s_file.close();
    std::string path = filename.empty() ? defaultLogPath() : filename;
    s_file.open(path, std::ios::out | std::ios::app);
    if (s_file.is_open()) {
        s_file << "--- log started, file: " << path << " ---\n";
        s_file.flush();
    }
}

void log(const char* tag, const std::string& msg) {
    if (!s_file.is_open()) return;
    s_file << "[" << tag << "] " << msg << "\n";
    s_file.flush();
}

static std::string now() {
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    std::ostringstream os;
    os << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S");
    return os.str();
}

void logWithTime(const char* tag, const std::string& msg) {
    if (!s_file.is_open()) return;
    s_file << "[" << now() << "] [" << tag << "] " << msg << "\n";
    s_file.flush();
}

void shutdown() {
    if (s_file.is_open()) {
        s_file.close();
    }
}

} // namespace Logger
