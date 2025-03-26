#pragma once

#include "constants.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cassert>

#include "tools.h"

// 全局变量声明
// 前向声明
class Disk;
class Object;
class Req;
struct Cell;

extern std::vector<std::vector<std::vector<int>>> FRE;
extern int T, M, N, V, G, TIME;
extern Disk DISKS[MAX_DISK_NUM];
extern Object OBJECTS[MAX_OBJECT_NUM];
extern Req REQS[LEN_REQ];

// 磁盘点
struct Cell
{
    std::unordered_set<int> req_ids;
    int obj_id; // 0 表示空闲
    int unit_id;
    int tag;
    int part_idx;
    Cell() : obj_id(0), unit_id(0), tag(0), part_idx(0) {}
    // 释放
    void free(){req_ids.clear(); obj_id = 0; unit_id = 0; tag = 0;}
    // 读取
    std::vector<int> read();
};

// 磁盘
class Disk
{
public:
    int id;
    Cell** cells; // 改为指针数组
    int point;
    int size;
    int tokens;
    std::vector<std::vector<int>> part_tables; // 磁盘分区 [start, end, free_cells, 上次写入的位置]
    int back; // 备份区数量 0-2
    int prev_read_token;
    std::vector<int> prev_occupied_obj;
    std::unordered_map<int, std::vector<int>> req_pos; // 请求位置 {req_id: [pos1, pos2, ...]}

    Disk() : id(0), point(1), size(0), tokens(0), back(0), prev_read_token(80), cells(nullptr) {}
    ~Disk(); // 析构函数用于释放内存
    // Disk(int id) : id(id), point(1), size(0), tokens(0), back(0), prev_read_token(80){}

    void init(int size, const std::vector<int> &tag_order, const std::vector<double> &tag_size_rate, const std::vector<std::vector<double>> &tag_size_db);

    void free_cell(int cell_id);

    std::vector<int> write(int obj_id, const std::vector<int> &units, int tag, int part_idx);

    void add_req(int req_id, const std::vector<int> &cells_idx);

    void remove_req(int req_id);

    std::pair<std::string, std::vector<int>> read(int timestamp);

    std::tuple<std::string, std::vector<int>, std::vector<int>> _read_by_best_path();

    int _get_best_start(int timestamp);
};

// 对象
class Object
{
public:
    int id;
    int size;
    int tag;
    std::vector<std::pair<int, std::vector<int>>> replicas; // disk_id, cell_idxs
    std::unordered_set<int> req_ids;
    bool occupied;

    Object() : id(0), size(0), tag(0), occupied(false)
    {
        replicas.resize(REP_NUM, {0, std::vector<int>()});
    }

    // Object(int id) : id(id), size(0), tag(0), occupied(false)
    // {
    //     replicas.resize(REP_NUM, {0, std::vector<int>()});
    // }
};


// 请求
class Req
{
public:
    // int id;
    int obj_id;
    // std::unordered_set<int> remain_units;
    Int3Set remain_units;
    int timestamp;

    // 添加默认构造函数
    // Req() : id(0), obj_id(0), timestamp(0) {}

    // Req(int id) : id(id), obj_id(0), timestamp(0) {}

    void update(int req_id, int obj_id, int timestamp);
    // void completed();
};


// 控制器
class Controller
{
public:
    int timestamp;
    std::unordered_set<int> activate_reqs;

    Controller() {}

    // 删除
    std::vector<int> delete_obj(int obj_id);

    // 写入
    Object *write(int obj_id, int obj_size, int tag);

    // 处理请求
    std::pair<std::vector<std::string>, std::vector<int>> read();

    // 添加请求
    void add_req(int req_id, int obj_id);

    // 同步时间
    void sync(int timestamp);

private:
    // 获取磁盘和对应分区
    std::vector<std::pair<int, int>> _get_disk(int obj_size, int tag);
};

// 读写删频率




// 函数声明
void init_input();
void process_timestamp(int timestamp, Controller &controller);
void process_delete(Controller &controller);
void process_write(Controller &controller);
void process_read(Controller &controller);
