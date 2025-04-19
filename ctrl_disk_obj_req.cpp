/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *   ██████╗████████╗██████╗ ██╗         ██████╗ ██████╗      ██╗
 *  ██╔════╝╚══██╔══╝██╔══██╗██║         ██╔══██╗██╔══██╗     ██║
 *  ██║        ██║   ██████╔╝██║         ██║  ██║██████╔╝     ██║
 *  ██║        ██║   ██╔══██╗██║         ██║  ██║██╔══██╗██   ██║
 *  ╚██████╗   ██║   ██║  ██║███████╗    ██████╔╝██████╔╝╚█████╔╝
 *   ╚═════╝   ╚═╝   ╚═╝  ╚═╝╚══════╝    ╚═════╝ ╚═════╝  ╚════╝ 
 * 
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * 【模块功能】
 * ┌─────────────────┬───────────────────────────────────────────────────────────┐
 * │ 请求管理         │ 实现请求的添加、删除和状态更新                              │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 磁盘管理         │ 提供空闲块链表的维护和管理                                  │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 资源分配         │ 处理磁盘空间的分配和释放                                    │
 * └─────────────────┴───────────────────────────────────────────────────────────┘
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#include "ctrl_disk_obj_req.h"  // ⟪控制器、磁盘、对象、请求相关⟫
#include <climits>              // ⟪系统限制常量⟫

/*╔══════════════════════════════ 请求处理实现 ══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：添加新的请求到系统                                               │
 * │ 步骤：                                                               │
 * │ 1. 更新请求状态和对象关联                                              │
 * │ 2. 更新活跃请求范围                                                   │
 * │ 3. 更新磁盘热数据区                                                   │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void Controller::add_req(int req_id, int obj_id)
{
    // ◆ 更新请求和对象状态
    REQS[req_id % LEN_REQ].init(req_id, OBJECTS[obj_id], timestamp);  // ● 初始化请求
    OBJECTS[obj_id].req_ids.insert(req_id);                           // ● 关联对象

    // ◆ 更新活跃请求范围
    assert(req_id > req_new_idx);
    req_new_idx = req_id;

    // ◆ 更新磁盘热数据区
    auto &[disk_id, cells_idx] = OBJECTS[obj_id].replicas[0];
    for (int cell_idx : cells_idx)
    {
        DISKS[disk_id].cells[cell_idx].req_ids.insert(req_id);
    }
}

/*╔══════════════════════════════ 请求移除实现 ══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：从系统中移除指定请求                                             │
 * │ 步骤：                                                               │
 * │ 1. 更新对象的请求关联                                                 │
 * │ 2. 清理请求状态                                                       │
 * │ 3. 更新磁盘数据区                                                     │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void Controller::remove_req(int req_id)
{
    // ◆ 更新对象关联
    int obj_id = REQS[req_id % LEN_REQ].obj_id;
    OBJECTS[obj_id].req_ids.erase(req_id);

    // ◆ 清理请求状态
    REQS[req_id % LEN_REQ].clear();

    // ◆ 更新磁盘数据区
    auto &[disk_id, cells_idx] = OBJECTS[obj_id].replicas[0];
    for (int cell_idx : cells_idx)
    {
        DISKS[disk_id].cells[cell_idx].req_ids.erase(req_id);
    }
}

/*╔════════════════════════════ 空闲块查找实现 ═══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：按指定方向查找最合适的空闲块                                      │
 * │ 参数：                                                                │
 * │ - target_size: 目标大小                                               │
 * │ - is_reverse: 是否反向查找                                            │
 * │ - first_or_best: 是否返回首个匹配块                                    │
 * └──────────────────────────────────────────────────────────────────────┘
 */
FreeBlock* Part::_find_best_block(int target_size, bool is_reverse, bool first_or_best) 
{
    // ◆ 验证链表状态
    assert(free_list_head != nullptr or free_cells == 0);
    if(free_list_head != nullptr ) assert(free_list_head->prev == nullptr);
    assert(free_list_tail != nullptr or free_cells == 0);
    if(free_list_tail != nullptr) assert(free_list_tail->next == nullptr);
    assert(tag != 0);

    // ◆ 查找最佳匹配块
    FreeBlock* current = is_reverse ? free_list_tail : free_list_head;
    int best_diff = INT_MAX;
    FreeBlock* best_block = nullptr;

    while (current != nullptr) 
    {
        // ● 计算大小差异
        int diff = current->end - current->start + 1 - target_size;
        if (diff >= 0 && diff < best_diff) 
        {
            if(first_or_best) return current;
            best_diff = diff;
            best_block = current;
        }
        current = is_reverse ? current->prev : current->next;
    }
    return best_block;
}

/*╔════════════════════════════ 空闲链表初始化 ═══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：初始化分区的空闲块链表                                           │
 * │ 步骤：                                                               │
 * │ 1. 检查空闲单元数量                                                   │
 * │ 2. 标准化分区范围                                                     │
 * │ 3. 创建初始空闲块                                                     │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void Part::init_free_list() 
{
    assert(tag != 0);
    
    // ◆ 检查空闲单元
    if (free_cells <= 0) return;
    
    // ◆ 标准化分区范围
    int min_pos = std::min(start, end);
    int max_pos = std::max(start, end);
    
    // ◆ 创建初始空闲块
    free_list_head = new FreeBlock(min_pos, max_pos);
    free_list_tail = free_list_head;
}

/*╔════════════════════════════ 空闲块分配实现 ═══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：分配指定位置的空闲块                                             │
 * │ 步骤：                                                                │
 * │ 1. 验证位置有效性                                                     │
 * │ 2. 查找目标空闲块                                                     │
 * │ 3. 根据位置情况更新或分割块                                            │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void Part::allocate_block(int pos) 
{
    // ◆ 状态验证
    assert(tag != 0);
    assert(free_list_head != nullptr or free_cells == 0);
    if(free_list_head != nullptr ) assert(free_list_head->prev == nullptr);
    assert(free_list_tail != nullptr or free_cells == 0);
    if(free_list_tail != nullptr) assert(free_list_tail->next == nullptr);
    
    // ◆ 验证位置有效性
    int min_pos = std::min(start, end);
    int max_pos = std::max(start, end);
    assert(pos >= min_pos && pos <= max_pos);
    
    // ◆ 查找目标块
    FreeBlock* current = free_list_head;
    assert(current != nullptr);
    
    while (current != nullptr) 
    {
        assert(current->start <= current->end);
        if (current->start <= pos && current->end >= pos) 
        {
            // ● 处理单个单元块
            if (current->start == pos && current->end == pos) 
            {
                _remove_free_block(current);
                return;
            }
            
            // ● 处理起始位置
            if (current->start == pos) 
            {
                current->start = pos + 1;
                return;
            }
            
            // ● 处理结束位置
            if (current->end == pos) 
            {
                current->end = pos - 1;
                return;
            }
            
            // ● 处理中间位置
            FreeBlock* new_block = new FreeBlock(pos + 1, current->end);
            current->end = pos - 1;
            
            // ● 链接新块
            new_block->next = current->next;
            new_block->prev = current;
            
            if (current->next != nullptr) 
            {
                current->next->prev = new_block;
            } 
            else 
            {
                free_list_tail = new_block;
            }
            
            current->next = new_block;
            return;
        }
        current = current->next;
    }
    assert(false and "找不到包含该位置的空闲块");
}

/*╔════════════════════════════ 空闲块释放实现 ═══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：释放指定位置为空闲状态                                           │
 * │ 步骤：                                                               │
 * │ 1. 验证位置有效性                                                     │
 * │ 2. 检查重复释放                                                       │
 * │ 3. 创建新空闲块                                                       │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void Part::free_block(int pos) 
{
    // ◆ 状态验证
    assert(tag != 0);
    assert(free_list_head != nullptr or free_cells == 0);
    if(free_list_head != nullptr ) assert(free_list_head->prev == nullptr);
    assert(free_list_tail != nullptr or free_cells == 0);
    if(free_list_tail != nullptr) assert(free_list_tail->next == nullptr);

    // ◆ 验证位置有效性
    int min_pos = std::min(start, end);
    int max_pos = std::max(start, end);
    assert(pos >= min_pos && pos <= max_pos);
    
    // ◆ 检查重复释放
    FreeBlock* current = free_list_head;
    while (current != nullptr) 
    {
        if (current->start <= pos && current->end >= pos) 
        {
            assert(false and "位置已经在空闲块中");
        }
        current = current->next;
    }
    
    // ◆ 创建新空闲块
    _insert_free_block(pos, pos);
}

/*╔════════════════════════════ 空闲块插入实现 ═══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：在链表中插入新的空闲块                                           │
 * │ 步骤：                                                               │
 * │ 1. 创建新空闲块                                                      │
 * │ 2. 查找插入位置                                                      │
 * │ 3. 更新链表指针                                                      │
 * │ 4. 合并相邻块                                                        │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void Part::_insert_free_block(int start_pos, int end_pos) 
{
    assert(tag != 0);
    
    // ◆ 创建新块
    FreeBlock* new_block = new FreeBlock(start_pos, end_pos);
    
    // ◆ 处理空链表
    if (free_list_head == nullptr) 
    {
        free_list_head = new_block;
        free_list_tail = new_block;
        return;
    }
    
    // ◆ 查找插入位置
    FreeBlock* current = free_list_head;
    FreeBlock* prev = nullptr;
    
    while (current != nullptr && current->start < start_pos) 
    {
        assert(current->start != start_pos);
        prev = current;
        current = current->next;
    }
    
    // ◆ 插入新块
    if (prev == nullptr) 
    {
        // ● 插入头部
        new_block->next = free_list_head;
        if (free_list_head != nullptr) 
        {
            free_list_head->prev = new_block;
        }
        free_list_head = new_block;
    } 
    else 
    {
        // ● 插入中间或尾部
        new_block->prev = prev;
        new_block->next = current;
        prev->next = new_block;
        if (current != nullptr) 
        {
            current->prev = new_block;
        } 
        else 
        {
            free_list_tail = new_block;
        }
    }
    
    // ◆ 合并相邻块
    _merge_adjacent_blocks(new_block);
}

/*╔════════════════════════════ 空闲块移除实现 ═══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：从链表中移除指定的空闲块                                         │
 * │ 步骤：                                                               │
 * │ 1. 更新前驱节点指针                                                   │
 * │ 2. 更新后继节点指针                                                   │
 * │ 3. 更新头尾指针                                                       │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void Part::_remove_free_block(FreeBlock* block) 
{
    assert(tag != 0);
    if (block == nullptr) return;
    
    // ◆ 更新链表指针
    if (block->prev != nullptr) 
    {
        block->prev->next = block->next;
    } 
    else 
    {
        free_list_head = block->next;
    }
    
    if (block->next != nullptr) 
    {
        block->next->prev = block->prev;
    } 
    else 
    {
        free_list_tail = block->prev;
    }
    
    // ◆ 释放内存
    delete block;
}

/*╔════════════════════════════ 空闲块合并实现 ═══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：合并相邻的空闲块                                                 │
 * │ 步骤：                                                                │
 * │ 1. 检查并合并前向相邻块                                                │
 * │ 2. 检查并合并后向相邻块                                                │
 * │ 3. 更新链表结构                                                       │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void Part::_merge_adjacent_blocks(FreeBlock* block) 
{
    assert(tag != 0);
    if (block == nullptr) return;
    
    // ◆ 尝试前向合并
    if (block->prev != nullptr && block->prev->end + 1 == block->start) 
    {
        FreeBlock* prev_block = block->prev;
        
        // ● 更新块边界
        assert(prev_block->end < block->end);
        prev_block->end = block->end;
        
        // ● 更新链表结构
        prev_block->next = block->next;
        if (block->next != nullptr) 
        {
            block->next->prev = prev_block;
        } 
        else 
        {
            free_list_tail = prev_block;
        }
        
        // ● 释放内存
        delete block;
        return;
    }
    
    // ◆ 尝试后向合并
    if (block->next != nullptr && block->end + 1 == block->next->start) 
    {
        FreeBlock* next_block = block->next;
        
        // ● 更新块边界
        assert(block->end < next_block->end);
        block->end = next_block->end;
        
        // ● 更新链表结构
        block->next = next_block->next;
        if (next_block->next != nullptr) 
        {
            next_block->next->prev = block;
        } 
        else 
        {
            free_list_tail = block;
        }
        
        // ● 释放内存
        delete next_block;
    }
}

/*╔════════════════════════════ 空闲链表清理实现 ══════════════════════════════╗
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 功能：清理空闲块链表并释放内存                                         │
 * │ 步骤：                                                               │
 * │ 1. 遍历并释放所有节点                                                 │
 * │ 2. 重置链表头尾指针                                                   │
 * └──────────────────────────────────────────────────────────────────────┘
 */
void Part::_clear_free_list() 
{
    // ◆ 遍历释放节点
    FreeBlock* current = free_list_head;
    while (current != nullptr) 
    {
        FreeBlock* next = current->next;
        delete current;
        current = next;
    }

    // ◆ 重置指针
    free_list_head = nullptr;
    free_list_tail = nullptr;
}

