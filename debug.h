#pragma once

// 简化的调试输出库，不使用模板
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

//  调试开关和文件设置
#ifndef DEBUG
// #define DEBUG 0
#endif

#ifndef DEBUG_FILE
#define DEBUG_FILE "log.txt"
#endif

// 定义debug宏，当DEBUG为0时，debug函数调用会被完全移除
#ifdef DEBUG
#define debug(...) debug(__VA_ARGS__)
#else
#define debug(...) ((void)0)
#endif

// 全局调试计数
#ifdef DEBUG
extern int DEBUG_COUNT;
#endif

// 基础类型输出函数声明
#ifdef DEBUG
void debug_print(int value, std::ofstream& out);
void debug_print(long value, std::ofstream& out);
void debug_print(long long value, std::ofstream& out);
void debug_print(unsigned int value, std::ofstream& out);
void debug_print(unsigned long value, std::ofstream& out);
void debug_print(unsigned long long value, std::ofstream& out);
void debug_print(float value, std::ofstream& out);
void debug_print(double value, std::ofstream& out);
void debug_print(bool value, std::ofstream& out);
void debug_print(char value, std::ofstream& out);
void debug_print(const char* value, std::ofstream& out);
void debug_print(const std::string& value, std::ofstream& out);

// 容器类型输出函数声明
void debug_print(const std::vector<int>& vec, std::ofstream& out);
void debug_print(const std::vector<std::string>& vec, std::ofstream& out);
void debug_print(const std::vector<double>& vec, std::ofstream& out);
void debug_print(const std::vector<bool>& vec, std::ofstream& out);
void debug_print(const std::vector<char>& vec, std::ofstream& out);

void debug_print(const std::map<int, int>& m, std::ofstream& out);
void debug_print(const std::map<std::string, int>& m, std::ofstream& out);
void debug_print(const std::map<int, std::string>& m, std::ofstream& out);
void debug_print(const std::map<std::string, std::string>& m, std::ofstream& out);

void debug_print(const std::set<int>& s, std::ofstream& out);
void debug_print(const std::set<std::string>& s, std::ofstream& out);

void debug_print(const std::unordered_map<int, int>& m, std::ofstream& out);
void debug_print(const std::unordered_map<std::string, int>& m, std::ofstream& out);
void debug_print(const std::unordered_map<int, std::string>& m, std::ofstream& out);

void debug_print(const std::unordered_set<int>& s, std::ofstream& out);
void debug_print(const std::unordered_set<std::string>& s, std::ofstream& out);

// 对偶类型
void debug_print(const std::pair<int, int>& p, std::ofstream& out);
void debug_print(const std::pair<std::string, int>& p, std::ofstream& out);
void debug_print(const std::pair<int, std::string>& p, std::ofstream& out);
void debug_print(const std::pair<std::string, std::string>& p, std::ofstream& out);

// 通用未知类型处理
void debug_print_unknown(const void* obj, std::ofstream& out);

// 单参数调试函数
void debug(int value);
void debug(long value);
void debug(long long value);
void debug(unsigned int value);
void debug(unsigned long value);
void debug(unsigned long long value);
void debug(float value);
void debug(double value);
void debug(bool value);
void debug(char value);
void debug(const char* value);
void debug(const std::string& value);
void debug(const std::vector<int>& vec);
void debug(const std::vector<std::string>& vec);
void debug(const std::vector<double>& vec);
void debug(const std::vector<bool>& vec);
void debug(const std::vector<char>& vec);
void debug(const std::map<int, int>& m);
void debug(const std::map<std::string, int>& m);
void debug(const std::map<int, std::string>& m);
void debug(const std::map<std::string, std::string>& m);
void debug(const std::set<int>& s);
void debug(const std::set<std::string>& s);

// 双参数调试函数
void debug(int v1, int v2);
void debug(const std::string& v1, int v2);
void debug(int v1, const std::string& v2);
void debug(const std::string& v1, const std::string& v2);
void debug(const char* v1, int v2);
void debug(int v1, const char* v2);
void debug(const char* v1, const char* v2);

// 三参数调试函数
void debug(int v1, int v2, int v3);
void debug(const std::string& v1, int v2, int v3);
void debug(int v1, const std::string& v2, int v3);
void debug(int v1, int v2, const std::string& v3);

// 四参数调试函数
void debug(int v1, int v2, int v3, int v4);
void debug(const std::string& v1, int v2, int v3, int v4);

// 清空调试文件
void clear_debug();
#endif // DEBUG





