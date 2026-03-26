#include "core/log.hpp"

#include <iostream>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <mutex> // Added for thread safety

// Initialize static member
LogLevel Log::s_CurrentLevel = LOG_LEVEL_TRACE;
static std::mutex s_LogMutex; // protects stdout

// Array for color-coded level names
static const char* s_LogLevelNames[] = {
    "\033[31;1m[FATAL]\033[0m", // Red (Bold)
    "\033[31m[ERROR]\033[0m",   // Red
    "\033[33m[WARN]\033[0m",    // Yellow
    "\033[32m[INFO]\033[0m",    // Green
    "\033[34m[TRACE]\033[0m"    // Blue
};

void Log::Init() {
    // Initialization logic, if any, can go here (e.g., opening a log file)
    s_CurrentLevel = LOG_LEVEL_TRACE;
    // Sync with stdio not needed if we use fprintf exclusively, but good practice
    std::ios::sync_with_stdio(false); 
    LOG_INFO("Logger initialized (Thread-Safe).");
}

void Log::Shutdown() {
    // Shutdown logic (e.g., closing a log file)
}

void Log::SetLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(s_LogMutex);
    s_CurrentLevel = level;
}

void Log::Print(LogLevel level, const char* format, ...) {
    // Runtime check: Only print if the message level is >= the current set level.
    if (level > s_CurrentLevel) {
        return;
    }

    // Lock mutex for the duration of this print
    std::lock_guard<std::mutex> lock(s_LogMutex);
    
    // Get current time
    time_t raw_time;
    struct tm* time_info;
    char time_buffer[80];

    time(&raw_time);
    time_info = localtime(&raw_time);

    // Format time: HH:MM:SS
    strftime(time_buffer, sizeof(time_buffer), "%H:%M:%S", time_info);

    // Print header: [TIME] [LEVEL]
    std::fprintf(stdout, "[%s] %s: ", time_buffer, s_LogLevelNames[level]);

    // Format the actual message
    va_list args;
    va_start(args, format);
    std::vfprintf(stdout, format, args);
    va_end(args);

    // Print newline
    std::fprintf(stdout, "\n");

    // Flush immediately for critical messages
    if (level <= LOG_LEVEL_ERROR) {
        std::fflush(stdout);
    }
}