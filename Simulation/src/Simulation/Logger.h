#pragma once

#include <string>

namespace Logger {

// Inicjalizacja – otwiera plik .log (np. simulation.log) w katalogu bieżącym.
// Powinna być wywołana raz przy starcie aplikacji.
void init(const std::string& filename = "simulation.log");

// Zapisuje linię: [tag] msg
void log(const char* tag, const std::string& msg);

// Zapisuje linię z znacznikiem czasu: [YYYY-MM-DD HH:MM:SS] [tag] msg
void logWithTime(const char* tag, const std::string& msg);

// Zamyka plik. Wywołaj przed zakończeniem aplikacji (opcjonalne).
void shutdown();

} // namespace Logger
