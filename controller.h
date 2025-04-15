#pragma once
#include "constants.h"
#include "disk_obj_req.h"

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

    // 时间戳
    int timestamp;

    // 活跃的 req 范围
    int req_105_idx = 0;
    int req_new_idx = 0;

    // 主动过滤的超载请求
    std::vector<int> over_load_reqs;
    // 被动过滤的繁忙请求
    std::vector<int> busy_reqs;


    int busy_count = 0;
    int over_load_count = 0;

    int write_count = 0;


    Controller() : DISKS(MAX_DISK_NUM), OBJECTS(MAX_OBJECT_NUM), REQS(LEN_REQ) {}
    
    // 初始化磁盘
    void disk_init();

    // 删除
    std::vector<int> delete_obj(int obj_id);

    // 写入
    Object *write(int obj_id, int obj_size, int tag);

    // 读取
    std::pair<std::vector<std::string>, std::vector<int>> read();

    // 添加请求
    void add_req(int req_id, int obj_id);

    // 删除请求
    void remove_req(int req_id);

    // 后置请求过滤
    void post_filter_req();

    // 前置请求过滤
    void pre_filter_req(std::vector<std::pair<int, int>> &reqs);
    
    // 获取磁盘统计信息
    DiskStatsInfo get_disk_stats(int disk_id);


private:
    // 获取磁盘和对应分区
    std::vector<std::pair<int, Part*>> _get_write_disk(int obj_size, int tag);

};

