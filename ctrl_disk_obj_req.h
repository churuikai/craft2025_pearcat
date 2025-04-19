/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *   ██████╗ ██████╗ ██████╗ ███████╗    ██╗  ██╗
 *  ██╔════╝██╔═══██╗██╔══██╗██╔════╝    ██║  ██║
 *  ██║     ██║   ██║██████╔╝█████╗      ███████║
 *  ██║     ██║   ██║██╔══██╗██╔══╝      ██╔══██║
 *  ╚██████╗╚██████╔╝██║  ██║███████╗    ██║  ██║
 *   ╚═════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝    ╚═╝  ╚═╝
 * 
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * 【模块功能】
 * ┌─────────────────┬───────────────────────────────────────────────────────────┐
 * │ 核心数据结构     │ 定义系统核心组件：控制器、磁盘、对象和请求                    │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 资源管理         │ 提供磁盘管理、对象操作和请求处理的接口                       │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 空间管理         │ 实现磁盘分区、空闲块管理和读写操作                          │
 * └─────────────────┴───────────────────────────────────────────────────────────┘
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include "constants.h"       // 系统常量
#include "tools.h"           // 工具类
#include <vector>
#include <unordered_set>
#include <cassert>
#include <deque>

/*╔══════════════════════════════ 前向声明 ═══════════════════════════════╗*/
class Controller;    // 系统控制器
class Disk;         // 磁盘设备
class Object;       // 存储对象
class Req;          // 访问请求
class Part;         // 磁盘分区
struct Cell;        // 磁盘单元格
struct FreeBlock;   // 空闲块
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 控制器类定义 ═══════════════════════════════╗*/
/**
 * @brief     系统控制器类
 * @details   系统核心组件，负责以下功能:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 资源管理：磁盘、对象和请求的统一管理                               │
 * │ 2. 时间管理：维护系统时间戳                                          │
 * │ 3. 请求管理：处理和过滤请求                                          │
 * │ 4. 统计信息：记录系统运行状态                                        │
 * └──────────────────────────────────────────────────────────────────────┘
 */
class Controller
{
public:
    // 系统资源
    std::vector<Disk> DISKS;       // 磁盘集合
    std::vector<Object> OBJECTS;   // 对象集合
    std::vector<Req> REQS;         // 请求集合

    // 时间管理
    int timestamp;                 // 当前时间戳
    int timestamp_real;            // 真实时间戳

    // 请求管理
    int req_105_idx = 0;           // 即将超时请求索引
    int req_new_idx = 0;           // 新请求索引

    // 过滤请求集合
    std::vector<int> over_load_reqs; // 主动过滤的超载请求
    std::vector<int> busy_reqs;      // 被动过滤的繁忙请求

    // 统计信息
    int busy_count = 0;            // 被动过滤请求计数
    int over_load_count = 0;       // 主动过滤请求计数
    int write_count = 0;           // 写入计数

    /**
     * @brief 控制器构造函数
     */
    Controller() : DISKS(MAX_DISK_NUM), OBJECTS(MAX_OBJECT_NUM), REQS(LEN_REQ) {}
    
    /**
     * @brief 初始化所有磁盘
     */
    void disk_init();

    /**
     * @brief 删除对象
     * @param obj_id 对象ID
     * @return 被中断的请求ID列表
     */
    std::vector<int> delete_obj(int obj_id);

    /**
     * @brief 写入对象
     * @param obj_id 对象ID
     * @param obj_size 对象大小
     * @param tag 对象标签
     * @return 写入的对象指针
     */
    Object *write(int obj_id, int obj_size, int tag);

    /**
     * @brief 执行读取操作
     * @return 磁头操作和完成的请求ID
     */
    std::pair<std::vector<std::string>, std::vector<int>> read();

    /**
     * @brief 添加请求
     * @param req_id 请求ID
     * @param obj_id 请求的对象ID
     */
    void add_req(int req_id, int obj_id);

    /**
     * @brief 移除请求
     * @param req_id 请求ID
     */
    void remove_req(int req_id);

    /**
     * @brief 请求后置过滤
     * @details 过滤已超时或即将超时的请求
     */
    void post_filter_req();

    /**
     * @brief 请求前置过滤
     * @param reqs 请求列表
     * @details 根据磁盘负载动态过滤请求
     */
    void pre_filter_req(std::vector<std::pair<int, int>> &reqs);
    
private:
    /**
     * @brief 选择写入磁盘和分区
     * @param obj_size 对象大小
     * @param tag 对象标签
     * @return 磁盘ID和分区指针对
     */
    std::vector<std::pair<int, Part*>> _get_write_disk(int obj_size, int tag);
};
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 磁盘单元格定义 ═══════════════════════════════╗*/
/**
 * @brief     磁盘单元格结构
 * @details   存储系统的基本单元，包含以下信息:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 请求信息：当前单元关联的请求                                       │
 * │ 2. 对象信息：存储的对象ID和单元ID                                    │
 * │ 3. 分区信息：所属分区和标签                                          │
 * └──────────────────────────────────────────────────────────────────────┘
 */
struct Cell
{
    std::unordered_set<int> req_ids;  // 请求ID集合
    int obj_id;                       // 对象ID，0表示空闲
    int unit_id;                      // 单元ID
    int tag;                          // 标签
    Part* part;                       // 所属分区

    /**
     * @brief 单元格构造函数
     */
    Cell() : obj_id(0), unit_id(0), tag(0), part(nullptr) {}
    
    /**
     * @brief 释放单元格
     */
    void free()
    {
        req_ids.clear(); 
        obj_id = 0; 
        unit_id = 0; 
        tag = 0;
    }
};
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 空闲块定义 ═══════════════════════════════╗*/
/**
 * @brief     空闲块结构
 * @details   管理连续空闲空间的双向链表节点:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 位置信息：空闲块的起始和结束位置                                   │
 * │ 2. 链表指针：前后空闲块的连接                                        │
 * └──────────────────────────────────────────────────────────────────────┘
 */
struct FreeBlock 
{
    int start;                // 空闲块起始位置
    int end;                  // 空闲块结束位置
    FreeBlock* prev;          // 前一个空闲块
    FreeBlock* next;          // 后一个空闲块
    
    /**
     * @brief 默认构造函数
     */
    FreeBlock() : start(0), end(0), prev(nullptr), next(nullptr) {}
    
    /**
     * @brief 带参数构造函数
     * @param s 起始位置
     * @param e 结束位置
     */
    FreeBlock(int s, int e) : start(s), end(e), prev(nullptr), next(nullptr) {}
};
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 分区类定义 ═══════════════════════════════╗*/
/**
 * @brief     磁盘分区类
 * @details   管理磁盘上的连续空间区域:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 空间管理：分区范围和空闲单元统计                                   │
 * │ 2. 写入控制：记录写入位置和标签信息                                   │
 * │ 3. 空闲管理：维护空闲块链表                                          │
 * │ 4. 对象追踪：记录非本分区对象                                        │
 * └──────────────────────────────────────────────────────────────────────┘
 */
class Part
{
public:
    int start;                 // 分区起始位置
    int end;                   // 分区结束位置
    int free_cells;            // 空闲单元数量
    int last_write_pos;        // 上次写入位置
    int tag;                   // 分区标签
    int size;                  // 分区大小
    FreeBlock* free_list_head; // 空闲块链表头指针
    FreeBlock* free_list_tail; // 空闲块链表尾指针

    // 当前分区内不属于该分区的对象
    std::vector<int> other_objs;

    /**
     * @brief 默认构造函数
     */
    Part() : start(0), end(0), free_cells(0), last_write_pos(0), tag(0), size(0), 
             free_list_head(nullptr), free_list_tail(nullptr) {}
    
    /**
     * @brief 带参数构造函数
     */
    Part(int start, int end, int free_cells, int last_write_pos, int tag, int size) : 
         start(start), end(end), free_cells(free_cells), last_write_pos(last_write_pos), 
         tag(tag), size(size), free_list_head(nullptr), free_list_tail(nullptr) {}

    /**
     * @brief 初始化空闲块链表
     */
    void init_free_list();
    
    /**
     * @brief 分配空闲块
     * @param pos 位置
     * @details 仅维护链表，不影响原有free_cells
     */
    void allocate_block(int pos);
    
    /**
     * @brief 释放块
     * @param pos 位置
     * @details 标记为空闲，并更新链表
     */
    void free_block(int pos);

    /**
     * @brief 清理空闲块链表
     * @details 释放内存
     */
    void _clear_free_list();

    /**
     * @brief 查找最佳匹配空闲块
     * @param target_size 目标大小
     * @param is_reverse 是否反向查找
     * @param first_or_best 是否优先查找
     * @return 找到的空闲块指针
     */
    FreeBlock* _find_best_block(int target_size, bool is_reverse, bool first_or_best);

    /**
     * @brief 验证链表一致性
     * @param disk 磁盘指针
     * @return 一致性检查结果
     */
    int _verify_free_list_consistency(Disk* disk);
    
    /**
     * @brief 验证链表完整性
     * @return 完整性检查结果
     */
    int _verify_free_list_integrity();

private:
    /**
     * @brief 插入空闲块
     * @param start_pos 起始位置
     * @param end_pos 结束位置
     */
    void _insert_free_block(int start_pos, int end_pos);
    
    /**
     * @brief 移除空闲块
     * @param block 空闲块指针
     */
    void _remove_free_block(FreeBlock* block);
    
    /**
     * @brief 合并相邻空闲块
     * @param block 空闲块指针
     */
    void _merge_adjacent_blocks(FreeBlock* block);
};
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 磁盘类定义 ═══════════════════════════════╗*/
/**
 * @brief     磁盘类
 * @details   模拟物理磁盘设备，提供以下功能:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 存储管理：单元格和分区的组织                                       │
 * │ 2. 读写控制：双磁头读写操作                                          │
 * │ 3. 垃圾回收：空间整理和优化                                          │
 * │ 4. 性能统计：请求等待时间分析                                        │
 * └──────────────────────────────────────────────────────────────────────┘
 */
class Disk
{
public:
    Controller* controller;           // 控制器指针

    int id;                           // 磁盘ID
    int size;                         // 磁盘大小

    std::vector<Cell> cells;                      // 磁盘单元格
    std::vector<std::vector<Part>> part_tables;   // 磁盘分区表

    int K;                            // GC操作令牌
    
    int point1;                       // 磁头1位置
    int point2;                       // 磁头2位置

    int tokens1;                      // 磁头1令牌
    int tokens2;                      // 磁头2令牌

    int prev_read_token1;             // 磁头1上次读取令牌
    int prev_read_token2;             // 磁头2上次读取令牌

    int data_size1;                   // 数据区1大小
    int data_size2;                   // 数据区2大小

    // 标签间接反向映射，用于交错写入
    int tag_reverse[MAX_TAG_NUM+1] = {0};

    // 请求等待时间统计
    std::deque<int> recent_wait_times;    // 最近请求等待时间
    long long total_wait_time = 0;        // 总等待时间
    static const int MAX_RECENT_REQS = 66;// 记录范围

    /**
     * @brief 磁盘构造函数
     */
    Disk() : id(0), point1(1), point2(1), size(0), tokens1(0), tokens2(0), 
             prev_read_token1(80), prev_read_token2(80) {}
    
    /**
     * @brief 磁盘析构函数
     */
    ~Disk();

    /**
     * @brief 初始化磁盘
     * @param size 磁盘大小
     * @param tag_order 标签顺序
     * @param tag_size_rate 标签大小比例
     * @param tag_size_db 标签大小数据库
     */
    void init(int size, const std::vector<int> &tag_order, 
              const std::vector<double> &tag_size_rate, 
              const std::vector<std::vector<double>> &tag_size_db);

    /**
     * @brief 释放单元格
     * @param cell_id 单元格ID
     */
    void free_cell(int cell_id);

    /**
     * @brief 写入对象单元
     * @param obj_id 对象ID
     * @param units 单元索引
     * @param tag 标签
     * @param part 目标分区
     * @return 写入的单元列表
     */
    std::vector<int> write(int obj_id, const std::vector<int> &units, 
                          int tag, Part* part);

    /**
     * @brief 读取操作
     * @param op_id 磁头ID
     * @return 操作指令和完成的请求
     */
    std::pair<std::string, std::vector<int>> read(int op_id);

    /**
     * @brief 垃圾回收
     * @return 交换的单元对
     */
    std::vector<std::pair<int, int>> gc();

    /**
     * @brief 获取指定标签的分区
     * @param tag 标签
     * @return 分区列表
     */
    std::vector<Part>& get_parts(int tag)
    {
        return part_tables[tag];
    }

    /**
     * @brief 获取平均等待时间
     * @return 平均等待时间
     */
    float get_avg_wait_time() const 
    { 
        return static_cast<float>(total_wait_time) / recent_wait_times.size(); 
    }

private:
    /**
     * @brief 获取最佳读取起点
     * @param op_id 磁头ID
     * @return 最佳起点位置
     */
    int _get_best_start(int op_id);
    
    /**
     * @brief 根据最佳路径读取
     * @param start 起始位置
     * @param op_id 磁头ID
     * @return 读取路径、完成请求和占用对象
     */
    std::tuple<std::string, std::vector<int>, std::vector<int>> _read_by_best_path(int start, int op_id);
    
    /**
     * @brief 读取单元格
     * @param cell_idx 单元格索引
     * @param completed_reqs 完成的请求
     */
    void _read_cell(int cell_idx, std::vector<int>& completed_reqs);
    
    /**
     * @brief 更新请求等待时间统计
     * @param req_id 请求ID
     * @param current_timestamp 当前时间戳
     */
    void _update_wait_time_stats(int req_id, int current_timestamp);

    /**
     * @brief 更新分区内外部对象
     */
    void _update_other_objs();
    
    /**
     * @brief 磁盘一对多交换
     * @param gc_pairs 交换对
     * @param is_add_free 是否添加空闲块
     */
    void _disk_gc_s2m(std::vector<std::pair<int, int>>& gc_pairs, bool is_add_free);
    
    /**
     * @brief 分区一对多交换
     * @param part 分区
     * @param gc_pairs 交换对
     * @param is_add_free 是否添加空闲块
     */
    void _part_gc_s2m(Part& part, std::vector<std::pair<int, int>>& gc_pairs, bool is_add_free);
    
    /**
     * @brief 磁盘多对多交换
     * @param gc_pairs 交换对
     */
    void _disk_gc_m2m(std::vector<std::pair<int, int>>& gc_pairs);
    
    /**
     * @brief 分区多对多交换
     * @param part 分区
     * @param gc_pairs 交换对
     */
    void _part_gc_m2m(Part& part, std::vector<std::pair<int, int>>& gc_pairs);
    
    /**
     * @brief 分区内聚拢
     * @param part 分区
     * @param gc_pairs 交换对
     * @param is_split_obj 是否分割对象
     */
    void _part_gc_inner(Part& part, std::vector<std::pair<int, int>>& gc_pairs, bool is_split_obj);
    
    /**
     * @brief 交换单元格
     * @param cell_idx1 单元格1
     * @param cell_idx2 单元格2
     */
    void _swap_cell(int cell_idx1, int cell_idx2);
    
    /**
     * @brief 查找一对多匹配
     * @param candidate_objs 候选对象
     * @param target_size 目标大小
     * @param matched_objs 匹配的对象
     * @param padding 填充大小
     * @return 是否找到匹配
     */
    bool _find_s2m_match(const std::vector<int>& candidate_objs, 
                        int target_size, 
                        std::vector<int>& matched_objs, 
                        int& padding);
    
    /**
     * @brief 查找多对多匹配
     * @param candidate_objs1 候选对象组1
     * @param candidate_objs2 候选对象组2
     * @param matched_objs1 匹配对象组1
     * @param matched_objs2 匹配对象组2
     * @param max_target_size 最大目标大小
     * @return 是否找到匹配
     */
    bool _find_m2m_match(const std::vector<int>& candidate_objs1, 
                         const std::vector<int>& candidate_objs2, 
                         std::vector<int>& matched_objs1, 
                         std::vector<int>& matched_objs2, 
                         int max_target_size);
    
    /**
     * @brief 动态规划求解子集和
     * @param size_obj_pairs 大小和对象对
     * @param target_size 目标大小
     * @param matched_objs 匹配的对象
     * @return 是否找到解
     */
    bool _dp_subset_sum(const std::vector<std::pair<int, int>>& size_obj_pairs, 
                       int target_size, 
                       std::vector<int>& matched_objs);
    
    /**
     * @brief 执行一对多交换
     * @param single_obj_idx 单个对象索引
     * @param multi_obj_idxs 多个对象索引
     * @param gc_pairs 交换对
     * @param padding 填充大小
     * @param target_part 目标分区
     */
    void _swap_s2m(int single_obj_idx, 
                  const std::vector<int>& multi_obj_idxs, 
                  std::vector<std::pair<int, int>>& gc_pairs, 
                  int padding, 
                  Part* target_part);
    
    /**
     * @brief 执行多对多交换
     * @param matched_objs1 匹配对象组1
     * @param matched_objs2 匹配对象组2
     * @param gc_pairs 交换对
     */
    void _swap_m2m(const std::vector<int>& matched_objs1, 
                  const std::vector<int>& matched_objs2, 
                  std::vector<std::pair<int, int>>& gc_pairs);
};
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 对象类定义 ═══════════════════════════════╗*/
/**
 * @brief     对象类
 * @details   存储系统中的基本数据实体:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 基本属性：ID、大小和标签                                          │
 * │ 2. 副本管理：多副本位置信息                                          │
 * │ 3. 请求关联：当前访问请求                                           │
 * │ 4. 状态跟踪：占用状态管理                                           │
 * └──────────────────────────────────────────────────────────────────────┘
 */
class Object
{
public:
    int id;                                         // 对象ID
    int size;                                       // 对象大小
    int tag;                                        // 对象标签
    std::vector<std::pair<int, std::vector<int>>> replicas; // 副本：磁盘ID和单元索引
    std::unordered_set<int> req_ids;                // 请求ID集合
    bool occupied;                                  // 是否被占用

    /**
     * @brief 对象构造函数
     */
    Object() : id(0), size(0), tag(0), occupied(false)
    {
        replicas.resize(REP_NUM, {0, std::vector<int>()});
    }
};
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 请求类定义 ═══════════════════════════════╗*/
/**
 * @brief     请求类
 * @details   表示对对象的访问请求:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 对象关联：请求的目标对象                                          │
 * │ 2. 进度跟踪：未读取单元管理                                          │
 * │ 3. 时间控制：创建时间记录                                           │
 * └──────────────────────────────────────────────────────────────────────┘
 */
class Req
{
public:
    int obj_id;                 // 对象ID
    Int3Set remain_units;       // 剩余未读取单元
    int timestamp;              // 创建时间戳
    
    /**
     * @brief 请求构造函数
     */
    Req() : obj_id(0), timestamp(0) {}
    
    /**
     * @brief 初始化请求
     * @param req_id 请求ID
     * @param obj 对象
     * @param timestamp 时间戳
     */
    void init(int req_id, Object& obj, int timestamp)
    {
        obj_id = obj.id;
        remain_units.clear();
        for (int i = 1; i <= obj.size; ++i)
        {
            remain_units.insert(i);
        }
        this->timestamp = timestamp;
    }
    
    /**
     * @brief 清除请求
     */
    void clear()
    {
        obj_id = 0; 
        remain_units.clear(); 
        timestamp = 0;
    }
};
/*╚═════════════════════════════════════════════════════════════════════════╝*/