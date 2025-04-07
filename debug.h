#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <ctime>
#include <sstream>
#include <type_traits>
#include <iterator>

// SFINAE helper: detects if a type T is iterable (has std::begin/end),
// but we explicitly exclude std::string so that it is printed as a normal type.
template <typename T, typename = void>
struct is_iterable : std::false_type {};

template <typename T>
struct is_iterable<T, std::void_t<
    decltype(std::begin(std::declval<T>())),
    decltype(std::end(std::declval<T>()))
    >> : std::true_type {};

template <typename T>
constexpr bool is_iterable_v = is_iterable<T>::value;

// Default file name prefixes and suffixes. These will be combined with a run-specific timestamp.
#ifndef DEFAULT_DEBUG_FILE_PREFIX
#define DEFAULT_DEBUG_FILE_PREFIX "log_"
#endif

#ifndef DEFAULT_INFO_FILE_PREFIX
#define DEFAULT_INFO_FILE_PREFIX "info_"
#endif

#ifndef DEFAULT_DEBUG_FILE_SUFFIX
#define DEFAULT_DEBUG_FILE_SUFFIX ".txt"
#endif

#ifndef DEFAULT_INFO_FILE_SUFFIX
#define DEFAULT_INFO_FILE_SUFFIX ".txt"
#endif

// Logger class: a singleton that handles logging with thread safety,
// timestamped entries, unique file names per run, and support for printing
// iterable containers.
class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }
    
    // Initialize logs: generate new log file names and clear the files.
    void init_logs() {
        std::lock_guard<std::mutex> lock(mutex_);
        generate_file_names();
        {
            std::ofstream dbg(debug_file_name_, std::ios::trunc);
        }
        {
            std::ofstream inf(info_file_name_, std::ios::trunc);
        }
        debug_count_ = 0;
        info_count_ = 0;
    }
    
    // Log a debug message using variadic arguments.
    template<typename... Args>
    void log_debug(const Args&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ofstream dbg(debug_file_name_, std::ios::app);
        dbg << timestamp() << " [DEBUG " << debug_count_++ << "]: ";
        log_impl(dbg, args...);
        dbg << "\n";
    }
    
    // Log an info message using variadic arguments.
    template<typename... Args>
    void log_info(const Args&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::ofstream inf(info_file_name_, std::ios::app);
        inf << timestamp() << " [INFO " << info_count_++ << "]: ";
        log_impl(inf, args...);
        inf << "\n";
    }
    
private:
    Logger() : debug_count_(0), info_count_(0) {
        generate_file_names();
    }
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::mutex mutex_;
    int debug_count_;
    int info_count_;
    std::string debug_file_name_;
    std::string info_file_name_;
    
    // Returns a human-readable timestamp for log entries.
    std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        char buf[32];
#if defined(_WIN32) || defined(_WIN64)
        ctime_s(buf, sizeof(buf), &now_time);
        std::string time_str(buf);
        if (!time_str.empty() && time_str.back() == '\n')
            time_str.pop_back();
        return time_str;
#else
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));
        return std::string(buf);
#endif
    }
    
    // Generates a filename-friendly timestamp (e.g. "20250407_123456").
    std::string file_timestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        char buf[32];
#if defined(_WIN32) || defined(_WIN64)
        std::tm tm;
        localtime_s(&tm, &now_time);
        std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm);
#else
        std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", std::localtime(&now_time));
#endif
        return std::string(buf);
    }
    
    // Generate new file names for this run based on the current timestamp.
    void generate_file_names() {
        debug_file_name_ = std::string(DEFAULT_DEBUG_FILE_PREFIX) + file_timestamp() + DEFAULT_DEBUG_FILE_SUFFIX;
        info_file_name_ = std::string(DEFAULT_INFO_FILE_PREFIX) + file_timestamp() + DEFAULT_INFO_FILE_SUFFIX;
    }
    
    // Base case: no arguments.
    void log_impl(std::ostream& out) {}
    
    // Overload for iterable types (excluding std::string).
    template<typename T>
    auto log_impl(std::ostream& out, const T& arg) 
      -> std::enable_if_t<is_iterable_v<T> && !std::is_same<T, std::string>::value> 
    {
        out << "[";
        bool first = true;
        for (const auto& item : arg) {
            if (!first)
                out << ", ";
            first = false;
            log_impl(out, item);
        }
        out << "]";
    }
    
    // Generic overload for a single non-container argument (or std::string).
    template<typename T>
    auto log_impl(std::ostream& out, const T& arg) 
      -> std::enable_if_t<!(is_iterable_v<T> && !std::is_same<T, std::string>::value)> 
    {
        out << arg;
    }
    
    // Overload for multiple arguments.
    template<typename T, typename... Args>
    void log_impl(std::ostream& out, const T& first, const Args&... rest) {
        log_impl(out, first);
        if constexpr (sizeof...(rest) > 0) {
            out << " ";
            log_impl(out, rest...);
        }
    }
};

// -- Public interface functions --

// Clears logs and creates new log/info files for this run.
inline void init_logs() {
    Logger::instance().init_logs();
}

#ifdef DEBUG
// Debug function: accepts any number of arguments.
template<typename... Args>
inline void debug(const Args&... args) {
    Logger::instance().log_debug(args...);
}
#else
inline void debug(...) {}
#endif

#ifdef INFO
// Info function: accepts any number of arguments.
template<typename... Args>
inline void info(const Args&... args) {
    Logger::instance().log_info(args...);
}
#else
inline void info(...) {}
#endif
