#pragma once
#include <vector>
#include <string>
#include <array>
#include <iostream>
#include <algorithm>
#include <cstring>


//参数
inline const int BACK_NUM = 2; // 备份区数量 0-2
inline const int IS_PART_BY_SIZE = 0; // 是否按大小分区 0-1

//是否间隔反向
inline const int IS_INTERVAL_REVERSE = 1; // 0-1

// 动态分区策略
inline const int BLOCK_SIZE = 0;  // 0 表示关闭

// inline int write_count = 1;

// 使用inline关键字允许在头文件中定义常量
inline const int WINDOW_SIZE = 10;
inline const int SCORES_DECAY_DISTANCE = 350;
inline const int FRE_PER_SLICING = 1800;
inline const int MAX_SLICING_NUM = (86400+1);
inline const int MAX_TAG_NUM = 16;
inline const int MAX_DISK_NUM = (10 + 1);
inline const int MAX_DISK_SIZE = (16384 + 1);
inline const int MAX_REQUEST_NUM = (30000000 + 1);
inline const int MAX_OBJECT_NUM = (100000 + 1);
inline const int LEN_REQ = 20000000;
inline const int REP_NUM = 3;
inline const int EXTRA_TIME = 105;
inline const int MAX_G = 1000;

// 复杂常量也可以用inline
inline const std::vector<double> TAG_SIZE_RATE = {0, 
0.09681, 0.0393 , 0.10434, 0.09115, 0.04644, 0.02577, 0.09509, 0.07413,
0.0244 , 0.07595, 0.03104, 0.08461, 0.02144, 0.0546 , 0.02681, 0.10803};

inline const std::vector<std::vector<double>> TAG_SIZE_DB = {
    {},
    {0.36755, 0.25564, 0.22792, 0.11396, 0.0349},
    {0.38539, 0.27586, 0.21298, 0.0791, 0.04665},
    {0.34114, 0.24338, 0.23625, 0.11507, 0.06415},
    {0.3399, 0.25638, 0.21113, 0.13341, 0.05916},
    {0.3913, 0.26956, 0.21043, 0.10434, 0.02434},
    {0.36571, 0.22285, 0.22857, 0.12857, 0.05428},
    {0.33111, 0.27555, 0.22222, 0.12888, 0.04222},
    {0.33781, 0.25841, 0.23822, 0.11574, 0.04979},
    {0.41666, 0.20277, 0.22222, 0.10833, 0.05},
    {0.32734, 0.26657, 0.21961, 0.13259, 0.05386},
    {0.38129, 0.22781, 0.2398, 0.09832, 0.05275},
    {0.38315, 0.23131, 0.2147, 0.11862, 0.05219},
    {0.3467, 0.2808, 0.23495, 0.11174, 0.02578},
    {0.39329, 0.24237, 0.21646, 0.1006, 0.04725},
    {0.35602, 0.28534, 0.22774, 0.10732, 0.02356},
    {0.35714, 0.24324, 0.22779, 0.11872, 0.05308}
};


// 用于编译期计算的辅助函数
inline constexpr int get_next_token(int token) {
    switch (token) {
        case 80: return 64;
        case 64: return 52;
        case 52: return 42;
        case 42: return 34;
        case 34: return 28;
        case 28: return 23;
        case 23: return 19;
        case 19: return 16;
        case 16: return 16;
        default: return 0;
    }
}

inline constexpr int token_to_index(int token) {
    switch (token) {
        case 16: return 0;
        case 19: return 1;
        case 23: return 2;
        case 28: return 3;
        case 34: return 4;
        case 42: return 5;
        case 52: return 6;
        case 64: return 7;
        case 80: return 8;
        default: return 0;
    }
}

inline constexpr int index_to_token(int index) {
    switch (index) {
        case 0: return 16;
        case 1: return 19;
        case 2: return 23;
        case 3: return 28;
        case 4: return 34;
        case 5: return 42;
        case 6: return 52;
        case 7: return 64;
        case 8: return 80;
        default: return 0;
    }
}
// 编译期计算token表
struct RPRResult {bool r_or_p; int cost; int prev_token;};

inline auto init_token_table() {
    using ResultArray = std::array<std::array<std::array<RPRResult, 64>, 1024>, 9>;
    ResultArray result{};

    // 填充数组
    for (int token_index = 0; token_index < 9; ++token_index) {
        int prev_token = index_to_token(token_index);
            
        for (int free_count = 0; free_count < 1024; ++free_count) {
            for (int read_count = 0; read_count < 64; ++read_count) {
                // 一直r代价
                int r_cost = 0;
                int r_end_token = prev_token;
                for (int i = 0; i < free_count + read_count; ++i) {
                    r_end_token = get_next_token(r_end_token);
                    r_cost += r_end_token;
                }
                // pr代价
                int p_cost = free_count;
                int p_end_token = free_count == 0 ? prev_token : 80;
                for (int i = 0; i < read_count; ++i) {
                    p_end_token = get_next_token(p_end_token);
                    p_cost += p_end_token;
                }
                
                if (r_cost <= p_cost) {
                    result[token_index][free_count][read_count] = RPRResult{true, r_cost, r_end_token};
                } else {
                    result[token_index][free_count][read_count] = RPRResult{false, p_cost, p_end_token};
                }
            }
        }
    }
    
    return result;
}

// 创建常量表
inline const auto TOKEN_TABLE = init_token_table();

// 辅助函数以便主程序使用
// 使用前次消耗的token、前方空闲单元数、前方待读取单元数，查询（路径、代价、最后一次token消耗）
inline const RPRResult& GET_TOKEN_TABLE(int prev_token, int free_count, int read_count) {
    return TOKEN_TABLE[token_to_index(prev_token)][free_count][read_count];
}



