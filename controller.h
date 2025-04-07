#pragma once
#include "constants.h"
#include "disk_obj_req.h"
// #include "data_analysis.h"

#include <vector>
#include <unordered_set>
#include <string>
#include <cassert>
#include <map>
#include <unordered_map>
// 前向声明
class Disk;
class Object;
class Req;
struct Part;
struct Cell;
struct PartStatsInfo;
struct DiskStatsInfo;
// 控制器
class Controller
{
public:

    std::vector<Disk> DISKS;
    std::vector<Object> OBJECTS;
    std::vector<Req> REQS;

    int timestamp;
    std::unordered_set<int> activate_reqs;
    std::vector<int> over_load_reqs;


    int busy_count = 0;
    int over_load_count = 0;

    int write_count = 0;


    Controller() : DISKS(MAX_DISK_NUM), OBJECTS(MAX_OBJECT_NUM), REQS(LEN_REQ) {}
    
    // 初始化
    void disk_init();

    // 删除
    std::vector<int> delete_obj(int obj_id);

    // 写入
    Object *write(int obj_id, int obj_size, int tag);

    // 处理请求
    std::pair<std::vector<std::string>, std::vector<int>> read();

    // 添加请求
    void add_req(int req_id, int obj_id);
    
    // 获取磁盘统计信息
    DiskStatsInfo get_disk_stats(int disk_id);


private:

    // 获取磁盘和对应分区
    std::vector<std::pair<int, Part*>> _get_disk(int obj_size, int tag);
};

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