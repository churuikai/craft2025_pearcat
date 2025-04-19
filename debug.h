/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  ██████╗ ███████╗██████╗ ██╗   ██╗ ██████╗    ██╗  ██╗
 *  ██╔══██╗██╔════╝██╔══██╗██║   ██║██╔════╝    ██║  ██║
 *  ██║  ██║█████╗  ██████╔╝██║   ██║██║  ███╗   ███████║
 *  ██║  ██║██╔══╝  ██╔══██╗██║   ██║██║   ██║   ██╔══██║
 *  ██████╔╝███████╗██████╔╝╚██████╔╝╚██████╔╝   ██║  ██║
 *  ╚═════╝ ╚══════╝╚═════╝  ╚═════╝  ╚═════╝    ╚═╝  ╚═╝
 * 
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * 【模块功能】
 * ┌─────────────────┬───────────────────────────────────────────────────────────┐
 * │ 日志管理         │ 提供线程安全的日志记录功能                                  │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 调试支持         │ 实现DEBUG和INFO级别的日志输出                               │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 格式化输出       │ 支持多种数据类型的格式化输出                                │
 * └─────────────────┴───────────────────────────────────────────────────────────┘
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once

#if defined(DEBUG) || defined(INFO)

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <ctime>
#include <sstream>
#include <type_traits>
#include <iterator>
#include <filesystem>  // C++17 filesystem support

/*╔══════════════════════════════ 类型特征定义 ═══════════════════════════════╗*/
/**
 * @brief     迭代器类型检测
 * @details   使用SFINAE技术检测类型是否可迭代:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 检查类型是否支持begin/end操作                                      │
 * │ 2. 显式排除std::string类型                                           │
 * │ 3. 提供类型特征辅助函数                                               │
 * └──────────────────────────────────────────────────────────────────────┘
 */
template <typename T, typename = void>
struct is_iterable : std::false_type {};

template <typename T>
struct is_iterable<T, std::void_t<
    decltype(std::begin(std::declval<T>())),
    decltype(std::end(std::declval<T>()))
    >> : std::true_type {};

template <typename T>
constexpr bool is_iterable_v = is_iterable<T>::value;
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 默认配置定义 ═══════════════════════════════╗*/
/**
 * @brief     日志文件配置
 * @details   定义日志文件的默认命名规则:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ PREFIX: 文件名前缀，区分DEBUG和INFO                                   │
 * │ SUFFIX: 文件扩展名，默认为.txt                                        │
 * └──────────────────────────────────────────────────────────────────────┘
 */
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
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 日志类定义 ═══════════════════════════════╗*/
/**
 * @brief     日志管理类
 * @details   实现线程安全的日志记录功能:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 单例模式：确保全局唯一的日志实例                                   │
 * │ 2. 线程安全：使用互斥锁保护日志操作                                   │
 * │ 3. 文件管理：自动创建和维护日志文件                                   │
 * │ 4. 格式化输出：支持多种数据类型的输出                                 │
 * └──────────────────────────────────────────────────────────────────────┘
 */
class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }
    
    // Initialize logs: creates the "log" folder (if needed), generates new file names, and clears them.
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
    
    // Returns a human-readable timestamp.
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
    
    // Returns a filename-friendly timestamp (e.g. "20250407_123456").
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
    
    // Generate new file names for this run and ensure the "log" folder exists.
    void generate_file_names() {
        std::filesystem::create_directories("log");
        debug_file_name_ = std::string("log/") + DEFAULT_DEBUG_FILE_PREFIX + file_timestamp() + DEFAULT_DEBUG_FILE_SUFFIX;
        info_file_name_  = std::string("log/") + DEFAULT_INFO_FILE_PREFIX + file_timestamp() + DEFAULT_INFO_FILE_SUFFIX;
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
/*╚═════════════════════════════════════════════════════════════════════════╝*/

#endif // defined(DEBUG) || defined(INFO)

/*╔══════════════════════════════ 公共接口定义 ═══════════════════════════════╗*/
/**
 * @brief     初始化日志系统
 * @details   创建新的日志文件并清理旧内容:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 创建log目录（如果不存在）                                         │
 * │ 2. 生成新的日志文件                                                  │
 * │ 3. 清空文件内容                                                      │
 * └──────────────────────────────────────────────────────────────────────┘
 */
inline void init_logs() {
#if defined(DEBUG) || defined(INFO)
    Logger::instance().init_logs();
#endif
}

/**
 * @brief     调试日志输出
 * @details   在DEBUG模式下记录调试信息:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 支持可变参数输出                                                  │
 * │ 2. 自动添加时间戳                                                    │
 * │ 3. 包含调试信息编号                                                  │
 * └──────────────────────────────────────────────────────────────────────┘
 */
#ifdef DEBUG
template<typename... Args>
inline void debug(const Args&... args) {
#if defined(DEBUG)
    Logger::instance().log_debug(args...);
#endif
}
#else
inline void debug(...) {}
#endif

/**
 * @brief     信息日志输出
 * @details   在INFO模式下记录系统信息:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 支持可变参数输出                                                  │
 * │ 2. 自动添加时间戳                                                    │
 * │ 3. 包含信息编号                                                      │
 * └──────────────────────────────────────────────────────────────────────┘
 */
#ifdef INFO
template<typename... Args>
inline void info(const Args&... args) {
#if defined(INFO)
    Logger::instance().log_info(args...);
#endif
}
#else
inline void info(...) {}
#endif
/*╚═════════════════════════════════════════════════════════════════════════╝*/
