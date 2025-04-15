#include "verify.h"
#include "debug.h"
#include <algorithm>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <cassert>
#include <climits>

void process_verify(Controller &controller) {
    // 验证所有磁盘
    int total_inconsistencies = 0;
    for (int disk_id = 1; disk_id <= N; disk_id++) {
        Disk &disk = controller.DISKS[disk_id];
   
        int disk_inconsistencies = 0;
        
        // 遍历所有分区（除备份区外）
        for (int tag = 1; tag <= M; tag++)
        {
            for (auto &part : disk.get_parts(tag))
            {
                // 使用新的验证函数
                disk_inconsistencies += part._verify_free_list_consistency(&disk);
                disk_inconsistencies += part._verify_free_list_integrity();
            }
        }

        // 检查冗余区
        for (auto &part : disk.get_parts(17)) {
            disk_inconsistencies += part._verify_free_list_consistency(&disk);
            disk_inconsistencies += part._verify_free_list_integrity();
        }
        
        if (disk_inconsistencies > 0) {
            info("磁盘", disk_id, "存在", disk_inconsistencies, "处链表不一致");
            assert(false);
        }
        total_inconsistencies += disk_inconsistencies;
    }

    // 输出详细统计信息
    info("=============================================================");
    info("TIME: ", controller.timestamp, " 磁盘碎片信息...(只输出前1个)");
    info("=============================================================");
    
    for (int disk_id = 1; disk_id <= 1; disk_id++) {
        // 使用新的磁盘统计函数
        DiskStatsInfo stats = controller.get_disk_stats(disk_id);
        
        info("======================== 磁盘", disk_id, "统计信息 ========================");
        
        // 按标签输出每个分区的信息
        for (int tag = 1; tag <= 17; tag++) {
            if(tag > M && tag != 17) continue;
            // 如果该标签下有分区统计信息
            if (stats.part_stats.find(tag) != stats.part_stats.end() && !stats.part_stats[tag].empty()) {
      
                for (const auto& part_stats : stats.part_stats[tag]) {
                    // 输出基本信息
                    std::stringstream ss;
                    ss << "tag:" << tag << " size:" << part_stats.size 
                       << " 平均(去最大):" << std::fixed << std::setprecision(2) << part_stats.avg_block_size
                       << " 最大:" << part_stats.max_block_size
                       << " 最小:" << part_stats.min_block_size
                       << " 碎片数:" << part_stats.fragmentation_count;
          
                    // 统计碎片大小1-10的具体数量
                    std::stringstream ss_small;
                    ss_small << "小碎片分布(1-5): ";
                    for (int size = 1; size <= 5; size++) {
                        int count = part_stats.fragment_size_distribution.count(size) > 0 ? 
                                    part_stats.fragment_size_distribution.at(size) : 0;
                        ss_small << count << "--";
                    }
                    info(tag, ss.str(), ss_small.str());
                }
            }
        }
    }
}

// 验证空闲块链表是否与实际空闲单元一致
int Part::_verify_free_list_consistency(Disk* disk) {
    assert(tag != 0);
    int inconsistencies = 0;
    
    // 统计实际空闲的cell数量
    int actual_free_cells = 0;
    int start = std::min(this->start, this->end);
    int end = std::max(this->start, this->end);
    for (int i = start; i <= end; i++) {
        if (disk->cells[i].obj_id == 0) {
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
        for (int i = std::min(start, end); i <= std::max(start, end) && count < 200; ++i) {
            if (disk->cells[i].obj_id == 0) {
                free_cells_str += std::to_string(i) + " ";
                count++;
            }
        }
        debug(free_cells_str);
        
        // 打印链表中的前20个空闲块
        std::string free_blocks_str = "链表空闲块: ";
        count = 0;
        FreeBlock* block = free_list_head;
        while (block != nullptr && count < 200) {
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
        if (disk->cells[i].obj_id == 0) {
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
int Part::_verify_free_list_integrity() {
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

// 获取磁盘的统计信息
DiskStatsInfo Controller::get_disk_stats(int disk_id) {
    DiskStatsInfo stats;
    stats.total_blocks = 0;
    stats.max_block_size = 0;
    stats.min_block_size = INT_MAX;
    stats.avg_block_size = 0;
    stats.fragmentation_count = 0;
    
    Disk &disk = DISKS[disk_id];
    
    std::vector<int> block_sizes; // 用于存储所有块的大小

    // 遍历所有正常分区
    for (int tag = 1; tag <= M; tag++)
    {

        for (const auto &part : disk.get_parts(tag))
        {
            // 创建新的分区统计信息
            PartStatsInfo part_stats;
            part_stats.tag = tag;
            part_stats.size = 1;

            std::vector<int> part_block_sizes; // 该分区的块大小

            FreeBlock *current = part.free_list_head;
            while (current != nullptr)
            {
                int block_size = current->end - current->start + 1;
                part_stats.total_blocks++;
                part_block_sizes.push_back(block_size);

                // 更新分区统计的最大最小值
                part_stats.max_block_size = std::max(part_stats.max_block_size, block_size);
                part_stats.min_block_size = std::min(part_stats.min_block_size, block_size);

                // 更新分区碎片大小分布
                part_stats.fragment_size_distribution[block_size]++;

                // 更新整个磁盘的统计信息
                stats.total_blocks++;
                block_sizes.push_back(block_size);
                stats.max_block_size = std::max(stats.max_block_size, block_size);
                stats.min_block_size = std::min(stats.min_block_size, block_size);
                stats.fragment_size_distribution[block_size]++;

                if (current->next != nullptr)
                {
                    // 如果当前块与下一个块不连续，增加碎片计数
                    if (current->end + 1 < current->next->start)
                    {
                        part_stats.fragmentation_count++;
                        stats.fragmentation_count++;
                    }
                }

                current = current->next;
            }

            // 计算分区的平均值，忽略最大值
            if (part_stats.total_blocks > 1)
            {
                // 移除最大值
                int max_size = part_stats.max_block_size;
                auto it = std::find(part_block_sizes.begin(), part_block_sizes.end(), max_size);
                if (it != part_block_sizes.end())
                {
                    part_block_sizes.erase(it);

                    // 计算剩余块的平均大小
                    double sum = std::accumulate(part_block_sizes.begin(), part_block_sizes.end(), 0.0);
                    part_stats.avg_block_size = sum / part_block_sizes.size();
                }
            }
            else if (part_stats.total_blocks == 1)
            {
                // 只有一个块时，平均值就是该块的大小
                part_stats.avg_block_size = part_block_sizes[0];
            }

            // 只有当分区有空闲块时才添加到统计中
            if (part_stats.total_blocks > 0)
            {
                stats.part_stats[tag].push_back(part_stats);
            }
        }
    }

    // 统计冗余区
    for (const auto &part : disk.get_parts(17)) {
        // 创建新的分区统计信息
        PartStatsInfo part_stats;
        part_stats.tag = 17; // 冗余区标签
        part_stats.size = 1;
        
        std::vector<int> part_block_sizes; // 该分区的块大小
        
        FreeBlock *current = part.free_list_head;
        while (current != nullptr) {
            int block_size = current->end - current->start + 1;
            part_stats.total_blocks++;
            part_block_sizes.push_back(block_size);
            
            // 更新分区统计的最大最小值
            part_stats.max_block_size = std::max(part_stats.max_block_size, block_size);
            part_stats.min_block_size = std::min(part_stats.min_block_size, block_size);
            
            // 更新分区碎片大小分布
            part_stats.fragment_size_distribution[block_size]++;
            
            // 更新整个磁盘的统计信息
            stats.total_blocks++;
            block_sizes.push_back(block_size);
            stats.max_block_size = std::max(stats.max_block_size, block_size);
            stats.min_block_size = std::min(stats.min_block_size, block_size);
            stats.fragment_size_distribution[block_size]++;
            
            if (current->next != nullptr) {
                // 如果当前块与下一个块不连续，增加碎片计数
                if (current->end + 1 < current->next->start) {
                    part_stats.fragmentation_count++;
                    stats.fragmentation_count++;
                }
            }
            
            current = current->next;
        }
        
        // 计算分区的平均值，忽略最大值
        if (part_stats.total_blocks > 1) {
            // 移除最大值
            int max_size = part_stats.max_block_size;
            auto it = std::find(part_block_sizes.begin(), part_block_sizes.end(), max_size);
            if (it != part_block_sizes.end()) {
                part_block_sizes.erase(it);
                
                // 计算剩余块的平均大小
                double sum = std::accumulate(part_block_sizes.begin(), part_block_sizes.end(), 0.0);
                part_stats.avg_block_size = sum / part_block_sizes.size();
            }
        } else if (part_stats.total_blocks == 1) {
            // 只有一个块时，平均值就是该块的大小
            part_stats.avg_block_size = part_block_sizes[0];
        }
        
        // 只有当分区有空闲块时才添加到统计中
        if (part_stats.total_blocks > 0) {
            stats.part_stats[17].push_back(part_stats);
        }
    }
    
    // 计算整个磁盘的平均值，忽略最大值
    if (stats.total_blocks > 1) {
        // 移除最大值
        int max_size = stats.max_block_size;
        auto it = std::find(block_sizes.begin(), block_sizes.end(), max_size);
        if (it != block_sizes.end()) {
            block_sizes.erase(it);
            
            // 计算剩余块的平均大小
            double sum = std::accumulate(block_sizes.begin(), block_sizes.end(), 0.0);
            stats.avg_block_size = sum / block_sizes.size();
        }
    } else if (stats.total_blocks == 1) {
        // 只有一个块时，平均值就是该块的大小
        stats.avg_block_size = block_sizes[0];
    }
    
    return stats;
}



