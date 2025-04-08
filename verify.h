#pragma once
#include "controller.h"
// 磁盘统计信息结构
struct DiskStatsInfo {
    int total_blocks;             // 总空闲块数量
    double avg_block_size;        // 平均块大小（不包含最大块）
    int max_block_size;           // 最大块大小
    int min_block_size;           // 最小块大小
    int fragmentation_count;      // 碎片数量
    std::map<int, int> fragment_size_distribution;  // 碎片大小分布 <大小,数量>
    
    // 每个tag的分区统计信息 <tag, vector<PartStatsInfo>>
    std::unordered_map<int, std::vector<PartStatsInfo>> part_stats; 
    
    DiskStatsInfo() : total_blocks(0), avg_block_size(0), 
                      max_block_size(0), min_block_size(0), 
                      fragmentation_count(0) {}
};
// 分区统计信息结构
struct PartStatsInfo {
    int tag;                      // 标签
    int size;                     // 分区大小
    int total_blocks;             // 总空闲块数量
    double avg_block_size;        // 平均块大小（不包含最大块）
    int max_block_size;           // 最大块大小
    int min_block_size;           // 最小块大小
    int fragmentation_count;      // 碎片数量
    std::map<int, int> fragment_size_distribution;  // 碎片大小分布 <大小,数量>
    
    PartStatsInfo() : tag(0), size(0), total_blocks(0), avg_block_size(0), 
                      max_block_size(0), min_block_size(0), 
                      fragmentation_count(0) {}
};

// 获取磁盘的统计信息
void process_verify(Controller &controller);