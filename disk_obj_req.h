#pragma once
#include "constants.h"
#include "controller.h"
#include <vector>
#include <unordered_set>
#include <cassert>
#include "tools.h"
#include <deque>
#include <string>

// 前向声明
class Controller;
class Disk;
class Object;
class Req;
class Part;
struct Cell;
struct FreeBlock;


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

// 空闲块结构
struct FreeBlock {
    int start;          // 空闲块起始位置
    int end;            // 空闲块结束位置
    FreeBlock* prev;    // 前一个空闲块
    FreeBlock* next;    // 后一个空闲块
    FreeBlock() : start(0), end(0), prev(nullptr), next(nullptr) {}
    FreeBlock(int s, int e) : start(s), end(e), prev(nullptr), next(nullptr) {}
};

// 分区
class Part
{
public:
    int start;
    int end;
    int free_cells;
    int last_write_pos;
    int tag;
    int size;
    FreeBlock* free_list_head;  // 空闲块链表头指针
    FreeBlock* free_list_tail;  // 空闲块链表尾指针

    // 当前分区内不属于该分区的对象
    std::vector<int> other_objs;

    Part() : start(0), end(0), free_cells(0), last_write_pos(0), tag(0), size(0), free_list_head(nullptr), free_list_tail(nullptr) {}
    Part(int start, int end, int free_cells, int last_write_pos, int tag, int size) : 
    start(start), end(end), free_cells(free_cells), last_write_pos(last_write_pos), tag(tag), size(size), free_list_head(nullptr), free_list_tail(nullptr) {}

    // 初始化空闲块链表
    void init_free_list();
    
    // 分配空闲块（仅维护链表，不影响原有free_cells）
    void allocate_block(int pos);
    
    // 释放块（标记为空闲，并更新链表）
    void free_block(int pos);

    // 清理空闲块链表（释放内存）
    void _clear_free_list();

    // 按指定方向查找最合适匹配的空闲节点
    FreeBlock* _find_best_block(int target_size, bool is_reverse, bool first_or_best);

    // 验证链表函数
    // 验证空闲块链表是否与实际空闲单元一致
    int _verify_free_list_consistency(Disk* disk);
    // 验证空闲块链表的连接性和排序
    int _verify_free_list_integrity();

private:
        // 在链表中插入一个新的空闲块
    void _insert_free_block(int start_pos, int end_pos);
    
    // 从链表中移除一个空闲块
    void _remove_free_block(FreeBlock* block);
    
    // 合并相邻的空闲块
    void _merge_adjacent_blocks(FreeBlock* block);

};

// 磁盘
class Disk
{
public:
    Controller* controller;

    int id;
    int size;

    std::vector<Cell> cells;                   // 磁盘点
    std::vector<std::vector<Part>> part_tables; // 磁盘分区 [start, end, free_cells, 上次写入的位置]

    int K;
    
    int point1;
    int point2;

    int tokens1;
    int tokens2;

    int prev_read_token1;
    int prev_read_token2;

    int data_size1;
    int data_size2;


    //标签间接反向, 该标签对应的标签
    int tag_reverse[MAX_TAG_NUM+1] = {0};

    // 记录最近请求的等待时间
    std::deque<int> recent_wait_times;
    // 最近请求的总等待时间
    long long total_wait_time = 0;
    // 请求数量统计范围
    static const int MAX_RECENT_REQS = 66;

    Disk() : id(0), point1(1), point2(1), size(0), tokens1(0), tokens2(0), prev_read_token1(80), prev_read_token2(80) {}
    ~Disk();

    // 初始化
    void init(int size, const std::vector<int> &tag_order, const std::vector<double> &tag_size_rate, const std::vector<std::vector<double>> &tag_size_db);

    // 释放单元格
    void free_cell(int cell_id);

    // 写入
    std::vector<int> write(int obj_id, const std::vector<int> &units, int tag, Part* part);

    // 读取
    std::pair<std::string, std::vector<int>> read(int op_id);

    // 垃圾回收
    std::vector<std::pair<int, int>> gc();

    // 获取指定分区
    std::vector<Part>& get_parts(int tag){return part_tables[tag];}

    // 获取平均等待时间
    float get_avg_wait_time() const { return static_cast<float>(total_wait_time) / recent_wait_times.size(); }

private:
    // 获取最佳读取起点
    int _get_best_start(int op_id);
    // 按最佳读取起点读取
    std::tuple<std::string, std::vector<int>, std::vector<int>> _read_by_best_path(int start, int op_id);
    // 读取指定单元格
    void _read_cell(int cell_idx, std::vector<int>& completed_reqs);
    // 更新请求等待时间统计
    void _update_wait_time_stats(int req_id, int current_timestamp);

    // 垃圾回收
    // 更新各个分区内不属于该分区的对象
    void _update_other_objs();
    // 一对多交换; 是否考虑加入空闲块
    void _disk_gc_s2m(std::vector<std::pair<int, int>>& gc_pairs, bool is_add_free);
    void _part_gc_s2m(Part& part, std::vector<std::pair<int, int>>& gc_pairs, bool is_add_free);
    // 多对多交换
    void _disk_gc_m2m(std::vector<std::pair<int, int>>& gc_pairs);
    void _part_gc_m2m(Part& part, std::vector<std::pair<int, int>>& gc_pairs);
    // 分区内部聚拢
    void _part_gc_inner(Part& part, std::vector<std::pair<int, int>>& gc_pairs, bool is_split_obj);
    // 交换两个单元格
    void _swap_cell(int cell_idx1, int cell_idx2);
    // 查找一组对象，使其大小之和等于目标大小
    bool _find_s2m_match(const std::vector<int>& candidate_objs, int target_size, std::vector<int>& matched_objs, int& padding);
    // 多对多匹配
    bool _find_m2m_match(const std::vector<int>& candidate_objs1, const std::vector<int>& candidate_objs2, std::vector<int>& matched_objs1, std::vector<int>& matched_objs2, int max_target_size);
    // 动态规划求解子集和问题
    bool _dp_subset_sum(const std::vector<std::pair<int, int>>& size_obj_pairs, int target_size, std::vector<int>& matched_objs);
    // 执行一对多交换
    void _swap_s2m(int single_obj_idx, const std::vector<int>& multi_obj_idxs, std::vector<std::pair<int, int>>& gc_pairs, int padding, Part* target_part);
    // 执行多对多交换
    void _swap_m2m(const std::vector<int>& matched_objs1, const std::vector<int>& matched_objs2, std::vector<std::pair<int, int>>& gc_pairs);
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
    Req() : obj_id(0), timestamp(0) {}
    void init(int req_id, Object& obj, int timestamp)
    {
        obj_id = obj.id;
        remain_units.clear();
        for (int i = 1; i <= obj.size; ++i)
        {
            remain_units.insert(i);
        }
        this->timestamp = timestamp;
    };
    void clear(){obj_id = 0; remain_units.clear(); timestamp = 0;}
};