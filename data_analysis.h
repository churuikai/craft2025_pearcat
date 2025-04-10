#pragma once
#include <vector>
#include <map>
#include <unordered_map>
#include "controller.h"

extern std::vector<std::vector<std::vector<int>>> FRE;
class Controller;


// 函数声明
// 频率相关函数声明
// int get_freq(int tag, int timestamp, int op_type); // 获取特定tag在特定时间的频率（op_type: 0删除，1写入，2读取）
std::vector<int>& get_sorted_read_tag(int timestamp); // 获取排序后的当前时间读频率的tag

void process_data_analysis();

// 计算标签顺序
void compute_tag_order();
// 计算交替标签顺序
void compute_alternating_tag_order();


// 辅助函数
std::vector<double> __normalize_curve(const std::vector<double> &curve);
double __compute_similarity(const std::vector<double> &curve1, const std::vector<double> &curve2, bool normalize_curve1 = true, bool normalize_curve2 = true);
