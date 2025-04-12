#pragma once
#include <vector>
#include <array>

inline int T, M, N, V, G, k;
inline float G_float;

//参数
inline const int BACK_NUM = 2; // 备份区数量 0-2
inline const int IS_PART_BY_SIZE = 0; // 是否按大小分区 0-1
inline const float DATA_COMPRESSION = 0.82; // 数据压缩系数 0-1

//是否间隔反向
inline const int IS_INTERVAL_REVERSE = 1; // 0-1

inline const int IS_EXTEND = 1;

inline int const START_TAG = 3; //3-13
inline int const WRITE_START = 10; //2-10
// 藏分
inline const bool TANKLAB = false;

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
inline const int LEN_REQ = 1000000;
inline const int REP_NUM = 3;
inline const int EXTRA_TIME = 105;
inline const int MAX_G = 1000;

// 负载系数
inline const float LOAD_COEFFICIENT = 0.82*1.599818022273934e-05;

inline std::vector<std::vector<int>> TAG_ORDERS;

inline std::vector<double> TAG_SIZE_RATE = {0, 
// 最大
0.05369, 0.0625, 0.07024, 0.06187, 0.06496, 0.03706, 0.05793, 0.09764, 0.05306, 0.09258, 0.06063, 0.04831, 0.06799, 0.05039, 0.06893, 0.05224};
// 平均
// 0.04727, 0.04929, 0.06407, 0.05955, 0.05706, 0.03739, 0.06672, 0.08116, 0.06099, 0.10166, 0.06761, 0.05211, 0.08305, 0.04561, 0.07982, 0.04665};


inline const std::vector<std::vector<double>> TAG_SIZE_DB = {
{}, 
 {0.39754, 0.25757, 0.20634, 0.08946, 0.04906}, 
 {0.35511, 0.25473, 0.22253, 0.13446, 0.03314}, 
 {0.36055, 0.25064, 0.23985, 0.10169, 0.04725}, 
 {0.35727, 0.26086, 0.22495, 0.12003, 0.03686}, 
 {0.35039, 0.26279, 0.22639, 0.11262, 0.04778}, 
 {0.383, 0.24005, 0.21308, 0.11598, 0.04787}, 
 {0.37226, 0.2792, 0.21367, 0.09591, 0.03893}, 
 {0.34844, 0.25014, 0.23072, 0.12242, 0.04826}, 
 {0.34903, 0.24515, 0.24383, 0.10959, 0.05237}, 
 {0.36538, 0.24213, 0.2277, 0.11756, 0.0472}, 
 {0.34603, 0.26131, 0.22814, 0.11878, 0.04571}, 
 {0.40219, 0.22943, 0.21115, 0.10511, 0.0521}, 
 {0.37796, 0.25254, 0.19322, 0.1144, 0.06186}, 
 {0.34933, 0.24497, 0.23381, 0.12388, 0.04799}, 
 {0.3765, 0.23795, 0.21485, 0.12148, 0.04919}, 
 {0.3465, 0.24356, 0.23483, 0.12729, 0.04779}

};
