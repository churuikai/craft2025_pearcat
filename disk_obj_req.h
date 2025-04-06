#pragma once
#include "constants.h"
#include "controller.h"
#include <vector>
#include <unordered_set>
#include <cassert>
#include "tools.h"

// 前向声明
class Controller;
class Disk;
class Object;
class Req;
struct Part;
struct Cell;


// 磁盘点
struct Cell
{
    std::unordered_set<int> req_ids;
    int obj_id; // 0 表示空闲
    int unit_id;
    int tag;
    Part* part;
    Cell() : obj_id(0), unit_id(0), tag(0), part(nullptr) {}
    // 释放
    void free(){req_ids.clear(); obj_id = 0; unit_id = 0; tag = 0;}

};

// 分区
struct Part
{
    int start;
    int end;
    int free_cells;
    int last_write_pos;
    int tag;
    int size;
    Part() : start(0), end(0), free_cells(0), last_write_pos(0), tag(0), size(0) {}
    Part(int start, int end, int free_cells, int last_write_pos, int tag, int size) : 
    start(start), end(end), free_cells(free_cells), last_write_pos(last_write_pos), tag(tag), size(size) {}
};

// 磁盘
class Disk
{
public:
    Controller* controller;

    int id;
    int size;
    Cell** cells; // 改为指针数组
    
    int point1;
    int point2;

    int tokens1;
    int tokens2;

    int prev_read_token1;
    int prev_read_token2;

    int data_size1;
    int data_size2;

    // 当前负载系数
    float load_coefficient = 1.0;
    
    // 磁盘分区 [start, end, free_cells, 上次写入的位置]
    std::vector<std::vector<Part>> part_tables; 
    
    int back; // 备份区数量 0-2
    int req_cells_num = 0;
    int consume_token_tmp[MAX_DISK_SIZE];

    // 请求位置 {req_id: [pos1, pos2, ...]}
    IncIDMap<Int16Array> req_pos; 

    //标签间接反向, 该标签对应的标签
    int tag_reverse[MAX_TAG_NUM+1] = {0};

    // 分片存储策略
    // int tag_free_count[MAX_TAG_NUM+1] = {0};

    Disk() : id(0), point1(1), point2(1), size(0), tokens1(0), tokens2(0), back(0), prev_read_token1(80), prev_read_token2(80), cells(nullptr) {}
    ~Disk();

    void init(int size, const std::vector<int> &tag_order, const std::vector<double> &tag_size_rate, const std::vector<std::vector<double>> &tag_size_db);

    std::vector<Part>& get_parts(int tag, int size){return part_tables[tag*5+size];}

    void free_cell(int cell_id);

    std::vector<int> write(int obj_id, const std::vector<int> &units, int tag, Part* part);

    void add_req(int req_id, const std::vector<int> &cells_idx);

    std::pair<std::string, std::vector<int>> read(int op_id);

    int _get_best_start(int op_id);

    std::tuple<std::string, std::vector<int>, std::vector<int>> _read_by_best_path(int start, int op_id);

    void _read_cell(int cell_idx, std::vector<int>& completed_reqs);

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
    void update(int req_id, Object& obj, int timestamp);
};