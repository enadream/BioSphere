#pragma once

// // Detect Release/Debug mode to strip logs automatically
// #if defined(NDEBUG) || defined(RELEASE)
//     #define ENGINE_COMPILE_LEVEL LOG_LEVEL_INFO // Strip Trace/Info in release
// #else
//     #define ENGINE_COMPILE_LEVEL LOG_LEVEL_TRACE // Keep everything in debug
// #endif

// --- Log Level Definitions ---
// Change this macro to control the *maximum* level of logs to be compiled into the final binary.
// For Release builds, set this to LOG_LEVEL_ERROR or LOG_LEVEL_FATAL.
#define ENGINE_COMPILE_LEVEL LOG_LEVEL_FATAL

enum LogLevel {
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_TRACE,
    LOG_LEVEL_COUNT
};

// --- Log Class ---
class Log {
public:
    static void Init();
    static void Shutdown();

    // Sets the minimum log level that will be printed at runtime.
    static void SetLevel(LogLevel level);

    // Core log function
    static void Print(LogLevel level, const char* format, ...);

private:
    static LogLevel s_CurrentLevel;
};


// --- Conditional Logging Macros ---
// These macros use an 'if' check at the *compile level*. 
// If ENGINE_COMPILE_LEVEL is lower than the macro's level, the entire block is ignored by the compiler.
#if ENGINE_COMPILE_LEVEL >= LOG_LEVEL_FATAL
    #define LOG_FATAL(format, ...) Log::Print(LOG_LEVEL_FATAL, format, ##__VA_ARGS__)
#else
    #define LOG_FATAL(format, ...)
#endif

#if ENGINE_COMPILE_LEVEL >= LOG_LEVEL_ERROR
    #define LOG_ERROR(format, ...) Log::Print(LOG_LEVEL_ERROR, format, ##__VA_ARGS__)
#else
    #define LOG_ERROR(format, ...)
#endif

#if ENGINE_COMPILE_LEVEL >= LOG_LEVEL_WARN
    #define LOG_WARN(format, ...) Log::Print(LOG_LEVEL_WARN, format, ##__VA_ARGS__)
#else
    #define LOG_WARN(format, ...)
#endif

#if ENGINE_COMPILE_LEVEL >= LOG_LEVEL_INFO
    #define LOG_INFO(format, ...) Log::Print(LOG_LEVEL_INFO, format, ##__VA_ARGS__)
#else
    #define LOG_INFO(format, ...)
#endif

#if ENGINE_COMPILE_LEVEL >= LOG_LEVEL_TRACE
    #define LOG_TRACE(format, ...) Log::Print(LOG_LEVEL_TRACE, format, ##__VA_ARGS__)
#else
    #define LOG_TRACE(format, ...)
#endif