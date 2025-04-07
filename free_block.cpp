#include "disk_obj_req.h"
#include <cassert>
#include <algorithm>
#include "debug.h"
#include <climits>
#include <string>

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
}

// 分配一个位置（将位置标记为已使用）
void Part::allocate_block(int pos) {
    assert(tag != 0);


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
                remove_free_block(current);
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
    insert_free_block(pos, pos);
}

// 在链表中插入一个新的空闲块
void Part::insert_free_block(int start_pos, int end_pos) {
    assert(tag != 0);
    // 创建新空闲块
    FreeBlock* new_block = new FreeBlock(start_pos, end_pos);
    
    // 如果链表为空，直接插入
    if (free_list_head == nullptr) {
        free_list_head = new_block;
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
        }
    }
    
    // 尝试合并相邻块
    merge_adjacent_blocks(new_block);
}

// 从链表中移除一个空闲块
void Part::remove_free_block(FreeBlock* block) {
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
    }
    
    // 释放内存
    delete block;
}

// 合并相邻的空闲块
void Part::merge_adjacent_blocks(FreeBlock* block) {
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
        }
        
        // 递归检查是否还能继续合并
        // merge_adjacent_blocks(prev_block);
        
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
        }
        
        // 递归检查是否还能继续合并
        // merge_adjacent_blocks(block);
        
        // 释放内存
        delete next_block;
    }
}



// 清理空闲块链表（释放内存）
void Part::clear_free_list() {
    FreeBlock* current = free_list_head;
    while (current != nullptr) {
        FreeBlock* next = current->next;
        delete current;
        current = next;
    }
    free_list_head = nullptr;
} 

// 验证链表函数实现

// 验证空闲块链表是否与实际空闲单元一致
int Part::verify_free_list_consistency(Disk* disk) {
    assert(tag != 0);
    int inconsistencies = 0;
    
    // 统计实际空闲的cell数量
    int actual_free_cells = 0;
    int start = std::min(this->start, this->end);
    int end = std::max(this->start, this->end);
    for (int i = start; i <= end; i++) {
        if (disk->cells[i]->obj_id == 0) {
            actual_free_cells++;
        }
    }
    
    // 统计空闲块链表中的空闲cell数量
    int list_free_cells = 0;
    std::vector<bool> in_free_list(disk->size + 1, false);
    FreeBlock *current = free_list_head;
    while (current != nullptr) {
        for (int i = current->start; i <= current->end; ++i) {
            if (!in_free_list[i]) {  // 避免重复计数
                list_free_cells++;
                in_free_list[i] = true;
            } else {
                // 如果已经计数过，说明存在重叠块
                inconsistencies++;
                debug("链表错误：空闲块重叠在位置", i);
            }
        }
        current = current->next;
    }
    
    // 比较数量是否一致
    if (actual_free_cells != list_free_cells || actual_free_cells != free_cells) {
        inconsistencies++;
        debug("分区不一致：tag=", tag, " size=", size, 
              " part范围=", std::min(start, end), "-", std::max(start, end),
              " actual_free=", actual_free_cells, 
              " list_free=", list_free_cells, 
              " part.free_cells=", free_cells);
        
        // 打印前20个空闲单元位置
        std::string free_cells_str = "实际空闲位置: ";
        int count = 0;
        for (int i = std::min(start, end); i <= std::max(start, end) && count < 20; ++i) {
            if (disk->cells[i]->obj_id == 0) {
                free_cells_str += std::to_string(i) + " ";
                count++;
            }
        }
        debug(free_cells_str);
        
        // 打印链表中的前20个空闲块
        std::string free_blocks_str = "链表空闲块: ";
        count = 0;
        FreeBlock* block = free_list_head;
        while (block != nullptr && count < 20) {
            free_blocks_str += "[" + std::to_string(block->start) + "-" + std::to_string(block->end) + "] ";
            block = block->next;
            count++;
        }
        debug(free_blocks_str);
    }
    
    // 详细验证每个cell
    std::vector<bool> is_free(disk->size + 1, false);
    
    // 标记cells中哪些是空闲的
    for (int i = start; i <= end; i++) {
        if (disk->cells[i]->obj_id == 0) {
            is_free[i] = true;
        }
    }
    
    // 检查链表中的每个块
    current = free_list_head;
    while (current != nullptr) {
        for (int i = current->start; i <= current->end; i++) {
            if (!is_free[i]) {
                inconsistencies++;
                debug("链表错误：cell[", i, "]在链表中标记为空闲，但实际已被占用");
            }
            // 标记为已检查
            is_free[i] = false;
        }
        current = current->next;
    }
    
    // 检查是否有空闲的cell没有在链表中
    for (int i = start; i <= end; i++) {
        if (is_free[i]) {
            inconsistencies++;
            debug("链表错误：cell[", i, "]实际空闲，但在链表中未找到");
        }
    }
    
    return inconsistencies;
}

// 验证空闲块链表的连接性和排序
int Part::verify_free_list_integrity() {
    assert(tag != 0);
    int inconsistencies = 0;
    
    // 检查链表的连接性
    FreeBlock* current = free_list_head;
    while (current != nullptr && current->next != nullptr) {
        if (current->next->prev != current) {
            inconsistencies++;
            debug("链表错误：双向链表连接错误，块[", current->start, "-", current->end, "]的后继块的前驱不是自己");
        }
        
        // 检查是否按起始位置排序
        if (current->start >= current->next->start) {
            inconsistencies++;
            debug("链表错误：块未按起始位置排序，块[", current->start, "-", current->end, 
                  "]排在块[", current->next->start, "-", current->next->end, "]之前");
        }
        
        current = current->next;
    }
    
    return inconsistencies;
}

// 打印空闲块链表信息
void Part::print_free_list_info() {
    assert(tag != 0);
    
    if (free_list_head == nullptr) {
        info("Tag", tag, "Size", size, "：无空闲块");
        return;
    }
    
    // 只输出前5个空闲块
    int count = 0;
    std::string blocks_info = "Tag" + std::to_string(tag) + " Size" + std::to_string(size) + "：";
    FreeBlock *current = free_list_head;
    while (current != nullptr && count < 5) {
        blocks_info += "[" + std::to_string(current->start) + "-" + std::to_string(current->end) + "] ";
        current = current->next;
        count++;
    }
    
    if (current != nullptr) {
        blocks_info += "... (还有更多空闲块)";
    }
    
    info(blocks_info);
}

// 获取空闲块链表的统计信息
void Part::get_free_list_stats(int& total_blocks, int& max_block_size, int& min_block_size, double& avg_block_size, int& fragmentation_count) {
    assert(tag != 0);
    
    FreeBlock *current = free_list_head;
    while (current != nullptr) {
        int block_size = current->end - current->start + 1;
        total_blocks++;
        max_block_size = std::max(max_block_size, block_size);
        min_block_size = std::min(min_block_size, block_size);
        avg_block_size += block_size;
        
        if (current->next != nullptr) {
            // 如果当前块与下一个块不连续，增加碎片计数
            if (current->end + 1 < current->next->start) {
                fragmentation_count++;
            }
        }
        
        current = current->next;
    }
} 