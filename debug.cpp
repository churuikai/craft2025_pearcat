#include "debug.h"

#ifdef DEBUG

// 全局调试计数初始化
int DEBUG_COUNT = 0;

// 基础类型输出实现
void debug_print(int value, std::ofstream& out) { out << value; }
void debug_print(long value, std::ofstream& out) { out << value; }
void debug_print(long long value, std::ofstream& out) { out << value; }
void debug_print(unsigned int value, std::ofstream& out) { out << value; }
void debug_print(unsigned long value, std::ofstream& out) { out << value; }
void debug_print(unsigned long long value, std::ofstream& out) { out << value; }
void debug_print(float value, std::ofstream& out) { out << value; }
void debug_print(double value, std::ofstream& out) { out << value; }
void debug_print(bool value, std::ofstream& out) { out << (value ? "true" : "false"); }
void debug_print(char value, std::ofstream& out) { out << "'" << value << "'"; }
void debug_print(const char* value, std::ofstream& out) { out << "\"" << value << "\""; }
void debug_print(const std::string& value, std::ofstream& out) { out << "\"" << value << "\""; }

// 容器类型输出实现
void debug_print(const std::vector<int>& vec, std::ofstream& out) {
    out << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) out << ", ";
        debug_print(vec[i], out);
    }
    out << "]";
}

void debug_print(const std::vector<std::string>& vec, std::ofstream& out) {
    out << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) out << ", ";
        debug_print(vec[i], out);
    }
    out << "]";
}

void debug_print(const std::vector<double>& vec, std::ofstream& out) {
    out << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) out << ", ";
        debug_print(vec[i], out);
    }
    out << "]";
}

void debug_print(const std::vector<bool>& vec, std::ofstream& out) {
    out << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) out << ", ";
        debug_print(vec[i], out);
    }
    out << "]";
}

void debug_print(const std::vector<char>& vec, std::ofstream& out) {
    out << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i > 0) out << ", ";
        debug_print(vec[i], out);
    }
    out << "]";
}

// map类型输出实现
void debug_print(const std::map<int, int>& m, std::ofstream& out) {
    out << "map{";
    size_t i = 0;
    for (const auto& pair : m) {
        if (i++ > 0) out << ", ";
        debug_print(pair.first, out);
        out << ": ";
        debug_print(pair.second, out);
    }
    out << "}";
}

void debug_print(const std::map<std::string, int>& m, std::ofstream& out) {
    out << "map{";
    size_t i = 0;
    for (const auto& pair : m) {
        if (i++ > 0) out << ", ";
        debug_print(pair.first, out);
        out << ": ";
        debug_print(pair.second, out);
    }
    out << "}";
}

void debug_print(const std::map<int, std::string>& m, std::ofstream& out) {
    out << "map{";
    size_t i = 0;
    for (const auto& pair : m) {
        if (i++ > 0) out << ", ";
        debug_print(pair.first, out);
        out << ": ";
        debug_print(pair.second, out);
    }
    out << "}";
}

void debug_print(const std::map<std::string, std::string>& m, std::ofstream& out) {
    out << "map{";
    size_t i = 0;
    for (const auto& pair : m) {
        if (i++ > 0) out << ", ";
        debug_print(pair.first, out);
        out << ": ";
        debug_print(pair.second, out);
    }
    out << "}";
}

// set类型输出实现
void debug_print(const std::set<int>& s, std::ofstream& out) {
    out << "set{";
    size_t i = 0;
    for (const auto& item : s) {
        if (i++ > 0) out << ", ";
        debug_print(item, out);
    }
    out << "}";
}

void debug_print(const std::set<std::string>& s, std::ofstream& out) {
    out << "set{";
    size_t i = 0;
    for (const auto& item : s) {
        if (i++ > 0) out << ", ";
        debug_print(item, out);
    }
    out << "}";
}

// unordered_map类型输出
void debug_print(const std::unordered_map<int, int>& m, std::ofstream& out) {
    out << "unordered_map{";
    size_t i = 0;
    for (const auto& pair : m) {
        if (i++ > 0) out << ", ";
        debug_print(pair.first, out);
        out << ": ";
        debug_print(pair.second, out);
    }
    out << "}";
}

void debug_print(const std::unordered_map<std::string, int>& m, std::ofstream& out) {
    out << "unordered_map{";
    size_t i = 0;
    for (const auto& pair : m) {
        if (i++ > 0) out << ", ";
        debug_print(pair.first, out);
        out << ": ";
        debug_print(pair.second, out);
    }
    out << "}";
}

void debug_print(const std::unordered_map<int, std::string>& m, std::ofstream& out) {
    out << "unordered_map{";
    size_t i = 0;
    for (const auto& pair : m) {
        if (i++ > 0) out << ", ";
        debug_print(pair.first, out);
        out << ": ";
        debug_print(pair.second, out);
    }
    out << "}";
}

// unordered_set类型输出
void debug_print(const std::unordered_set<int>& s, std::ofstream& out) {
    out << "unordered_set{";
    size_t i = 0;
    for (const auto& item : s) {
        if (i++ > 0) out << ", ";
        debug_print(item, out);
    }
    out << "}";
}

void debug_print(const std::unordered_set<std::string>& s, std::ofstream& out) {
    out << "unordered_set{";
    size_t i = 0;
    for (const auto& item : s) {
        if (i++ > 0) out << ", ";
        debug_print(item, out);
    }
    out << "}";
}

// pair类型输出
void debug_print(const std::pair<int, int>& p, std::ofstream& out) {
    out << "(";
    debug_print(p.first, out);
    out << ", ";
    debug_print(p.second, out);
    out << ")";
}

void debug_print(const std::pair<std::string, int>& p, std::ofstream& out) {
    out << "(";
    debug_print(p.first, out);
    out << ", ";
    debug_print(p.second, out);
    out << ")";
}

void debug_print(const std::pair<int, std::string>& p, std::ofstream& out) {
    out << "(";
    debug_print(p.first, out);
    out << ", ";
    debug_print(p.second, out);
    out << ")";
}

void debug_print(const std::pair<std::string, std::string>& p, std::ofstream& out) {
    out << "(";
    debug_print(p.first, out);
    out << ", ";
    debug_print(p.second, out);
    out << ")";
}

// 通用未知类型处理
void debug_print_unknown(const void*, std::ofstream& out) {
    out << "<复杂对象>";
}

// 单参数调试函数实现
void debug(int value) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(value, out);
    out << std::endl;
    out.close();
}

void debug(long value) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(value, out);
    out << std::endl;
    out.close();
}

void debug(long long value) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(value, out);
    out << std::endl;
    out.close();
}

void debug(unsigned int value) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(value, out);
    out << std::endl;
    out.close();
}

void debug(unsigned long value) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(value, out);
    out << std::endl;
    out.close();
}

void debug(unsigned long long value) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(value, out);
    out << std::endl;
    out.close();
}

void debug(float value) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(value, out);
    out << std::endl;
    out.close();
}

void debug(double value) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(value, out);
    out << std::endl;
    out.close();
}

void debug(bool value) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(value, out);
    out << std::endl;
    out.close();
}

void debug(char value) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(value, out);
    out << std::endl;
    out.close();
}

void debug(const char* value) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(value, out);
    out << std::endl;
    out.close();
}

void debug(const std::string& value) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(value, out);
    out << std::endl;
    out.close();
}

void debug(const std::vector<int>& vec) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(vec, out);
    out << std::endl;
    out.close();
}

void debug(const std::vector<std::string>& vec) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(vec, out);
    out << std::endl;
    out.close();
}

void debug(const std::vector<double>& vec) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(vec, out);
    out << std::endl;
    out.close();
}

void debug(const std::vector<bool>& vec) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(vec, out);
    out << std::endl;
    out.close();
}

void debug(const std::vector<char>& vec) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(vec, out);
    out << std::endl;
    out.close();
}

void debug(const std::map<int, int>& m) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(m, out);
    out << std::endl;
    out.close();
}

void debug(const std::map<std::string, int>& m) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(m, out);
    out << std::endl;
    out.close();
}

void debug(const std::map<int, std::string>& m) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(m, out);
    out << std::endl;
    out.close();
}

void debug(const std::map<std::string, std::string>& m) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(m, out);
    out << std::endl;
    out.close();
}

void debug(const std::set<int>& s) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(s, out);
    out << std::endl;
    out.close();
}

void debug(const std::set<std::string>& s) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(s, out);
    out << std::endl;
    out.close();
}

// 多参数调试函数实现
void debug(int v1, int v2) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(v1, out);
    out << ", ";
    debug_print(v2, out);
    out << std::endl;
    out.close();
}

void debug(const std::string& v1, int v2) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(v1, out);
    out << ", ";
    debug_print(v2, out);
    out << std::endl;
    out.close();
}

void debug(int v1, const std::string& v2) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(v1, out);
    out << ", ";
    debug_print(v2, out);
    out << std::endl;
    out.close();
}

void debug(const std::string& v1, const std::string& v2) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(v1, out);
    out << ", ";
    debug_print(v2, out);
    out << std::endl;
    out.close();
}

void debug(const char* v1, int v2) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(v1, out);
    out << ", ";
    debug_print(v2, out);
    out << std::endl;
    out.close();
}

void debug(int v1, const char* v2) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(v1, out);
    out << ", ";
    debug_print(v2, out);
    out << std::endl;
    out.close();
}

void debug(const char* v1, const char* v2) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(v1, out);
    out << ", ";
    debug_print(v2, out);
    out << std::endl;
    out.close();
}

void debug(int v1, int v2, int v3) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(v1, out);
    out << ", ";
    debug_print(v2, out);
    out << ", ";
    debug_print(v3, out);
    out << std::endl;
    out.close();
}

void debug(const std::string& v1, int v2, int v3) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(v1, out);
    out << ", ";
    debug_print(v2, out);
    out << ", ";
    debug_print(v3, out);
    out << std::endl;
    out.close();
}

void debug(int v1, const std::string& v2, int v3) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(v1, out);
    out << ", ";
    debug_print(v2, out);
    out << ", ";
    debug_print(v3, out);
    out << std::endl;
    out.close();
}

void debug(int v1, int v2, const std::string& v3) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(v1, out);
    out << ", ";
    debug_print(v2, out);
    out << ", ";
    debug_print(v3, out);
    out << std::endl;
    out.close();
}

void debug(int v1, int v2, int v3, int v4) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(v1, out);
    out << ", ";
    debug_print(v2, out);
    out << ", ";
    debug_print(v3, out);
    out << ", ";
    debug_print(v4, out);
    out << std::endl;
    out.close();
}

void debug(const std::string& v1, int v2, int v3, int v4) {
    std::ofstream out(DEBUG_FILE, std::ios::app);
    out << "DEBUG " << DEBUG_COUNT++ << ": ";
    debug_print(v1, out);
    out << ", ";
    debug_print(v2, out);
    out << ", ";
    debug_print(v3, out);
    out << ", ";
    debug_print(v4, out);
    out << std::endl;
    out.close();
}

// 清空调试文件
void clear_debug() {
    std::ofstream out(DEBUG_FILE, std::ios::trunc);
    out.close();
    DEBUG_COUNT = 0;
}

#endif // DEBUG