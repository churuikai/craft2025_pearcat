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
inline float G_float;
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

struct Part
{
    int start;
    int end;
    int free_cells;
    int last_write_pos;
    int tag;
    int size;
    Part() : start(0), end(0), free_cells(0), last_write_pos(0), tag(0), size(0) {}

    [[deprecated("Dont use operator[], Please use member variables directly: start, end, free_cells, last_write_pos!")]]
    Part(std::initializer_list<int> init) : start(0), end(0), free_cells(0), last_write_pos(0) {
        int i = 0;
        for (int val : init) {
            if (i == 0) start = val;
            else if (i == 1) end = val;
            else if (i == 2) free_cells = val;
            else if (i == 3) last_write_pos = val;
            else if (i == 4) tag = val;
            else if (i == 5) size = val;
            i++;
            if (i > 5) break;
        }
    }
    // 标记为废弃，建议直接使用成员变量
    [[deprecated("Dont use operator[], Please use member variables directly: start, end, free_cells, last_write_pos!")]]
    int& operator[](int idx) {
        switch(idx) {
            case 0: return start;
            case 1: return end;
            case 2: return free_cells;
            case 3: return last_write_pos;
            case 4: return tag;
            case 5: return size;
            default: return start;
        }
    }
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
    std::vector<Part> part_tables; // 磁盘分区 [start, end, free_cells, 上次写入的位置]
    int back; // 备份区数量 0-2
    int prev_read_token;
    int req_cells_num = 0;
    int consume_token_tmp[MAX_DISK_SIZE];
    IncIDMap<Int16Array> req_pos; // 请求位置 {req_id: [pos1, pos2, ...]}

    //标签间接反向, 该标签对应的标签
    int tag_reverse[MAX_TAG_NUM+1] = {0};

    // 分片存储策略
    // int tag_free_count[MAX_TAG_NUM+1] = {0};

    Disk() : id(0), point(1), size(0), tokens(0), back(0), prev_read_token(80), cells(nullptr) {}
    ~Disk();

    void init(int size, const std::vector<int> &tag_order, const std::vector<double> &tag_size_rate, const std::vector<std::vector<double>> &tag_size_db);

    std::vector<Part>& get_parts(int tag, int size){}

    void free_cell(int cell_id);

    std::vector<int> write(int obj_id, const std::vector<int> &units, int tag, int part_idx);

    void add_req(int req_id, const std::vector<int> &cells_idx);

    void remove_req(int req_id);

    std::pair<std::string, std::vector<int>> read(int timestamp);

    std::tuple<std::string, std::vector<int>, std::vector<int>> _read_by_best_path(int start);

    int _get_best_start(int timestamp);

    void _get_consume_token(int start_point, int last_token, int target_point);
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

};


// 请求
class Req
{
public:
    // int id;
    int obj_id;
    Int3Set remain_units;
    int timestamp;
    void update(int req_id, int obj_id, int timestamp);
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
