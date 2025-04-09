#include "disk_obj_req.h"
#include <cassert>
#include <algorithm>
#include "debug.h"
#include <climits>
#include <string>

// 按指定方向查找最合适匹配的空闲节点
FreeBlock* Part::_find_best_block(int target_size, bool is_reverse, bool first_or_best) 
{
    assert(free_list_head != nullptr or free_cells == 0);
    if(free_list_head != nullptr ) assert(free_list_head->prev == nullptr);
    assert(free_list_tail != nullptr or free_cells == 0);
    if(free_list_tail != nullptr) assert(free_list_tail->next == nullptr);

    assert(tag != 0);
    FreeBlock* current = is_reverse ? free_list_tail : free_list_head;
    int best_diff = INT_MAX;
    FreeBlock* best_block = nullptr;
    while (current != nullptr) {
        int diff = current->end - current->start + 1 - target_size;
        if (diff >= 0 && diff < best_diff) {
            if(first_or_best) {
                return current;
            }
            best_diff = diff;
            best_block = current;
        }
        current = is_reverse ? current->prev : current->next;
    }
    return best_block;
}



// 初始化空闲块链表
void Part::init_free_list() {
    assert(tag != 0);
    // 如果没有空闲单元，直接返回
    if (free_cells <= 0) {
        return;
    }
    
    // 获取分区范围并标准化方向（小的为起点，大的为终点）
    int min_pos = std::min(start, end);
    int max_pos = std::max(start, end);
    
    free_list_head = new FreeBlock(min_pos, max_pos);
    free_list_tail = free_list_head;
}

// 分配一个位置（将位置标记为已使用）
void Part::allocate_block(int pos) {
    assert(tag != 0);

    assert(this->tag !=0);
    assert(free_list_head != nullptr or free_cells == 0);
    if(free_list_head != nullptr ) assert(free_list_head->prev == nullptr);
    assert(free_list_tail != nullptr or free_cells == 0);
    if(free_list_tail != nullptr) assert(free_list_tail->next == nullptr);
    // 验证位置有效性
    int min_pos = std::min(start, end);
    int max_pos = std::max(start, end);
    
    assert(pos >= min_pos && pos <= max_pos);
    
    // 查找包含该位置的空闲块
    FreeBlock* current = free_list_head;
    assert(current != nullptr);
    while (current != nullptr) {
        assert(current->start <= current->end);
        if (current->start <= pos && current->end >= pos) {
            // 找到包含该位置的空闲块
            
            // 如果是单个单元
            if (current->start == pos && current->end == pos) {
                // 直接移除这个块
                _remove_free_block(current);
                return;
            }
            
            // 如果在开头
            if (current->start == pos) {
                current->start = pos + 1;
                return;
            }
            
            // 如果在结尾
            if (current->end == pos) {
                current->end = pos - 1;
                return;
            }
            
            // 如果在中间，需要分割成两个块
            FreeBlock* new_block = new FreeBlock(pos + 1, current->end);
            current->end = pos - 1;
            
            // 插入新块
            new_block->next = current->next;
            new_block->prev = current;
            
            if (current->next != nullptr) {
                current->next->prev = new_block;
            } else {
                // 如果current是尾节点,新块成为尾节点
                free_list_tail = new_block;
            }
            
            current->next = new_block;
            return;
        }
        
        current = current->next;
    }
    
    assert(false and "找不到包含该位置的空闲块");
}

// 释放一个位置（将位置标记为空闲）
void Part::free_block(int pos) {
    assert(tag != 0);
    assert(this->tag !=0);
    // debug("free block", "free_cells", free_cells);
    assert(free_list_head != nullptr or free_cells == 0);
    if(free_list_head != nullptr ) assert(free_list_head->prev == nullptr);
    assert(free_list_tail != nullptr or free_cells == 0);
    if(free_list_tail != nullptr) assert(free_list_tail->next == nullptr);

    // 验证位置有效性
    int min_pos = std::min(start, end);
    int max_pos = std::max(start, end);
    
    assert(pos >= min_pos && pos <= max_pos);

    
    // 检查该位置是否已经在空闲块中
    FreeBlock* current = free_list_head;
    while (current != nullptr) {
        if (current->start <= pos && current->end >= pos) {
            // 位置已经在空闲块中，不需要额外操作
            assert(false and "位置已经在空闲块中");
        }
        current = current->next;
    }
    
    // 插入一个只包含这个位置的空闲块，并尝试合并
    _insert_free_block(pos, pos);
}

// 在链表中插入一个新的空闲块
void Part::_insert_free_block(int start_pos, int end_pos) {
    assert(tag != 0);
    // 创建新空闲块
    FreeBlock* new_block = new FreeBlock(start_pos, end_pos);
    
    // 如果链表为空，直接插入
    if (free_list_head == nullptr) {
        free_list_head = new_block;
        free_list_tail = new_block;
        return;
    }
    
    // 查找合适的插入位置（按起始位置排序）
    FreeBlock* current = free_list_head;
    FreeBlock* prev = nullptr;
    
    while (current != nullptr && current->start < start_pos) {
        assert(current->start != start_pos);
        prev = current;
        current = current->next;
    }
    
    // 插入新块
    if (prev == nullptr) {
        // 插入到链表头部
        new_block->next = free_list_head;
        if (free_list_head != nullptr) {
            free_list_head->prev = new_block;
        }
        free_list_head = new_block;
    } else {
        // 插入到链表中间或尾部
        new_block->prev = prev;
        new_block->next = current;
        prev->next = new_block;
        if (current != nullptr) {
            current->prev = new_block;
        } else {
            // 如果current为nullptr，说明是在尾部插入
            free_list_tail = new_block;
        }
    }
    
    // 尝试合并相邻块
    _merge_adjacent_blocks(new_block);
}

// 从链表中移除一个空闲块
void Part::_remove_free_block(FreeBlock* block) {
    assert(tag != 0);
    if (block == nullptr) return;
    
    // 调整链表指针
    if (block->prev != nullptr) {
        block->prev->next = block->next;
    } else {
        // 如果是头节点，更新头指针
        free_list_head = block->next;
    }
    
    if (block->next != nullptr) {
        block->next->prev = block->prev;
    } else {
        // 如果是尾节点，更新尾指针
        free_list_tail = block->prev;
    }
    
    // 释放内存
    delete block;
}

// 合并相邻的空闲块
void Part::_merge_adjacent_blocks(FreeBlock* block) {
    assert(tag != 0);
    if (block == nullptr) return;
    
    // 尝试与前一个块合并
    if (block->prev != nullptr && block->prev->end + 1 == block->start) {
        FreeBlock* prev_block = block->prev;
        
        // 更新前一个块的结束位置
        // prev_block->end = (prev_block->end > block->end) ? prev_block->end : block->end;
        assert(prev_block->end < block->end);
        prev_block->end = block->end;
        
        // 从链表中移除当前块
        prev_block->next = block->next;
        if (block->next != nullptr) {
            block->next->prev = prev_block;
        } else {
            // 如果合并的是尾部块，更新尾指针
            free_list_tail = prev_block;
        }
        
        // 递归检查是否还能继续合并
        // _merge_adjacent_blocks(prev_block);
        
        // 释放内存
        delete block;
        return;
    }
    
    // 尝试与后一个块合并
    if (block->next != nullptr && block->end + 1 == block->next->start) {
        FreeBlock* next_block = block->next;
        
        // 更新当前块的结束位置
        // block->end = (block->end > next_block->end) ? block->end : next_block->end;
        assert(block->end < next_block->end);
        block->end = next_block->end;
        
        // 从链表中移除后一个块
        block->next = next_block->next;
        if (next_block->next != nullptr) {
            next_block->next->prev = block;
        } else {
            // 如果合并的是尾部块，更新尾指针
            free_list_tail = block;
        }
        
        // 递归检查是否还能继续合并
        // _merge_adjacent_blocks(block);
        
        // 释放内存
        delete next_block;
    }
}

// 清理空闲块链表（释放内存）
void Part::_clear_free_list() {
    FreeBlock* current = free_list_head;
    while (current != nullptr) {
        FreeBlock* next = current->next;
        delete current;
        current = next;
    }
    free_list_head = nullptr;
    free_list_tail = nullptr;
} 

