#pragma once
#include <vector>
#include <array>

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
// inline const std::vector<double> TAG_SIZE_RATE = {0, 
// 0.09681, 0.0393 , 0.10434, 0.09115, 0.04644, 0.02577, 0.09509, 0.07413,
// 0.0244 , 0.07595, 0.03104, 0.08461, 0.02144, 0.0546 , 0.02681, 0.10803};
// official_sample 最大空间比例
// inline const std::vector<double> TAG_SIZE_RATE = {0, 
// 0.02874, 0.06872, 0.0834, 0.04642, 0.05478, 0.04244, 0.04744, 0.11655, 0.07079, 0.06028, 0.05884, 0.12248, 0.03096, 0.02991, 0.08102, 0.05724};
// practice_sample 平均空间比例
inline const std::vector<double> TAG_SIZE_RATE = {0, 
0.02979, 0.07901, 0.08115, 0.0504, 0.04575, 0.03406, 0.05187, 0.13116, 0.05194, 0.05071, 0.07502, 0.13919, 0.03231, 0.03073, 0.06192, 0.05499};
inline const std::vector<std::vector<double>> TAG_SIZE_DB = {
{{}, 
    {0.74563, 0.08354, 0.08603, 0.0586, 0.02618}, 
    {0.11407, 0.76972, 0.07675, 0.02452, 0.01492}, 
    {0.10205, 0.76991, 0.07162, 0.0376, 0.0188}, 
    {0.78931, 0.0811, 0.07517, 0.03659, 0.0178}, 
    {0.1203, 0.73147, 0.08055, 0.04511, 0.02255}, 
    {0.72894, 0.10526, 0.09078, 0.05131, 0.02368}, 
    {0.77198, 0.09554, 0.08577, 0.03691, 0.00977}, 
    {0.12564, 0.0968, 0.07518, 0.68692, 0.01544}, 
    {0.13203, 0.09988, 0.70952, 0.04247, 0.01607}, 
    {0.13081, 0.1283, 0.67924, 0.04402, 0.01761}, 
    {0.81143, 0.06911, 0.06484, 0.04522, 0.00938}, 
    {0.11881, 0.099, 0.07425, 0.69108, 0.01683}, 
    {0.76221, 0.08577, 0.08686, 0.04668, 0.01845}, 
    {0.72935, 0.09275, 0.10165, 0.05209, 0.02414}, 
    {0.12636, 0.08869, 0.72782, 0.0486, 0.0085}, 
    {0.19101, 0.13001, 0.14125, 0.50561, 0.0321}}
};
