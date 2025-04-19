/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *  ██╗    ██╗██████╗ ██╗████████╗███████╗
 *  ██║    ██║██╔══██╗██║╚══██╔══╝██╔════╝
 *  ██║ █╗ ██║██████╔╝██║   ██║   █████╗  
 *  ██║███╗██║██╔══██╗██║   ██║   ██╔══╝  
 *  ╚███╔███╔╝██║  ██║██║   ██║   ███████╗
 *   ╚══╝╚══╝ ╚═╝  ╚═╝╚═╝   ╚═╝   ╚══════╝
 * 
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * 【模块功能】
 * ┌─────────────────┬───────────────────────────────────────────────────────────┐
 * │ 写入策略         │ 实现对象写入的具体策略和优化                                │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 空间分配         │ 管理磁盘空间的分配和使用                                    │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 副本管理         │ 处理对象副本的分布和同步                                    │
 * └─────────────────┴───────────────────────────────────────────────────────────┘
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

/*============================================================================
 * 写入事件处理
 *============================================================================
 * 【代码功能】
 * - 实现对象写入的具体策略
 * - 磁盘和分区选择算法
 * - 优化空间分配与利用
 *============================================================================*/

#include "constants.h"          // 常量定义
#include "ctrl_disk_obj_req.h"  // 控制器、磁盘、对象、请求相关
#include "data_analysis.h"      // 数据分析相关
#include "debug.h"              // 调试工具
#include <iostream>
#include <algorithm>
#include <random>

/*╔══════════════════════════════ 写入控制模块 ═══════════════════════════════╗*/
/**
 * @brief     写入对象到磁盘
 * @param     obj_id 对象ID
 * @param     obj_size 对象大小
 * @param     tag 对象标签
 * @return    写入后的对象指针
 * @details   执行以下步骤:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 初始化对象属性                                                     │
 * │ 2. 选择合适的磁盘和分区                                               │
 * │ 3. 准备单元ID列表                                                     │
 * │ 4. 将对象写入所有选中的磁盘                                           │
 * └──────────────────────────────────────────────────────────────────────┘
 */
Object *Controller::write(int obj_id, int obj_size, int tag)
{
    // ◆ 初始化对象属性
    tag = tag==0 ? rand() % M + 1 : tag;
    Object &obj = OBJECTS[obj_id];
    obj.size = obj_size;
    obj.tag = tag;
    obj.id = obj_id;

    // ◆ 选择写入位置
    auto space = _get_write_disk(obj_size, tag);

    // ◆ 准备单元ID列表
    std::vector<int> units;
    for (int i = 1; i <= obj_size; ++i)
    {
        units.push_back(i);
    }

    // ◆ 执行写入操作
    for (size_t i = 0; i < space.size(); ++i)
    {
        auto &[disk_id, part] = space[i];
        auto pos = DISKS[disk_id].write(obj_id, units, tag, part);
        obj.replicas[i].first = disk_id;
        obj.replicas[i].second.insert(obj.replicas[i].second.end(), pos.begin(), pos.end());
    }

    return &obj;
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 磁盘选择模块 ═══════════════════════════════╗*/
/**
 * @brief     选择适合写入的磁盘和分区
 * @param     obj_size 对象大小
 * @param     tag 对象标签
 * @return    磁盘ID和分区指针对的列表
 * @details   执行以下策略:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 寻找能够精确匹配空闲块大小的同标签区域                              │
 * │ 2. 根据标签优先级在其他分区中查找                                      │
 * │ 3. 在其他磁盘中选择备份区域                                           │
 * └──────────────────────────────────────────────────────────────────────┘
 */
std::vector<std::pair<int, Part *>> Controller::_get_write_disk(int obj_size, int tag)
{
    std::vector<std::pair<int, Part *>> space;

    // ◆ 初始化参数
    int cycle_disk = 4;  // 每写入cycle_disk次，换一个磁盘
    int cycle_op = 1;    // 每写入cycle_op次，换一个数据区
    int disk_start = (++write_count / cycle_disk) + 1;
    int op_start = (write_count / cycle_op) + 1;

    // ◆ 构建标签优先级列表
    std::vector<int> tag_list = {tag, -1, WRITE_START};
    std::vector<int> tag_list_tmp = get_similar_tag_sequence(timestamp, WRITE_START, 2);
    tag_list.insert(tag_list.end(), tag_list_tmp.begin(), tag_list_tmp.end());
    
    // ● 优化标签顺序
    auto it = std::find(tag_list.begin() + 2, tag_list.end(), tag);
    std::rotate(tag_list.begin() + 2, it+1, tag_list.end());
    tag_list.push_back(17);

    // ◆ 设置数据区交替写入顺序
    std::vector<int> op_list = op_start % 2 == 0 ? std::vector<int>{0, 1} : std::vector<int>{1, 0};

    // ◆ 策略1: 寻找精确匹配的空闲块
    for (int i = 1 + disk_start; i <= N + disk_start; ++i)
    {
        int disk_id = (i - 1) % N + 1;
        
        // ● 遍历数据区
        for (int part_idx : op_list)
        {
            // ● 检查同标签分区
            for (auto &part : DISKS[disk_id].get_parts(tag))
            {
                if (part.free_cells < obj_size) continue;
                
                // ● 查找精确匹配的空闲块
                FreeBlock *current = part.free_list_head;
                while (current != nullptr)
                {
                    if (current->end - current->start + 1 == obj_size)
                    {
                        space.push_back({disk_id, &part});
                        goto find_back;
                    }
                    current = current->next;
                }
            }
        }
    }

    // ◆ 策略2: 根据标签优先级查找
    for (int tag_ : tag_list)
    {
        for (int i = 1 + disk_start; i <= N + disk_start; ++i)
        {
            int disk_id = (i - 1) % N + 1;
            tag_ = tag_ == -1 ? DISKS[disk_id].tag_reverse[tag] : tag_;

            // ● 遍历数据区
            for (int part_idx : op_list)
            {
                auto &parts = DISKS[disk_id].get_parts(tag_);
                if (parts.size() == 0) continue;

                // ● 检查空闲空间
                Part &part = parts[part_idx];
                if (part.free_cells >= obj_size)
                {
                    space.push_back({disk_id, &part});
                    goto find_back;
                }
            }
        }
    }

    find_back:
    assert(space.size() == 1);

    // ◆ 策略3: 选择备份区域
    for (int i = 1 + disk_start; i <= N + disk_start; ++i)
    {
        if (space.size() == 3) break;
        
        int disk_id = (i - 1) % N + 1;
        
        // ● 跳过已选中的磁盘
        if (std::any_of(space.begin(), space.end(), 
                        [disk_id](const auto &p) { return p.first == disk_id; }))
            continue;
        
        // ● 检查备份区空间
        if (DISKS[disk_id].get_parts(0)[0].free_cells >= obj_size)
        {
            space.push_back({disk_id, &DISKS[disk_id].get_parts(0)[0]});
        }
    }

    assert(space.size() == 3);
    return space;
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 磁盘写入模块 ═══════════════════════════════╗*/
/**
 * @brief     将对象写入指定磁盘分区
 * @param     obj_id 对象ID
 * @param     units 单元ID列表
 * @param     tag 对象标签
 * @param     part 目标分区
 * @return    写入的磁盘单元位置列表
 * @details   执行以下策略:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 确定写入方向                                                       │
 * │ 2. 寻找最优空闲块                                                     │
 * │ 3. 执行写入操作                                                       │
 * │ 4. 更新分区状态                                                       │
 * └──────────────────────────────────────────────────────────────────────┘
 */
std::vector<int> Disk::write(int obj_id, const std::vector<int> &units, int tag, Part *part)
{
    // ◆ 确定写入方向
    bool is_reverse = tag == part->tag ? part->start > part->end : part->start < part->end;
    
    // ● 边界标签特殊处理
    if(part->tag == TAG_ORDERS[0][0] or part->tag == TAG_ORDERS[0][TAG_ORDERS[0].size()-1]) 
    {
        is_reverse = part->start > part->end;
    }
    
    // ◆ 获取分区范围
    int start = std::min(part->start, part->end);
    int end = std::max(part->start, part->end);
    
    // ◆ 寻找最优写入位置
    int pointer = 0;
    int min_diff = 99999;
    
    // ● 对于标签匹配的非备份区，寻找最优空闲块
    if (part->tag == tag && part->tag != 0)
    {
        FreeBlock *current = is_reverse ? part->free_list_tail : part->free_list_head;
        while (current != nullptr) 
        {
            if (current->end - current->start + 1 >= units.size()) 
            {
                int diff = current->end - current->start + 1 - units.size();
                if (diff < min_diff) 
                {
                    min_diff = diff;
                    pointer = is_reverse ? current->end : current->start;
                }
            }
            current = is_reverse ? current->prev : current->next;
        }
    }
    
    // ● 如果没找到合适的空闲块，从分区边界开始
    if (pointer == 0) 
    {
        pointer = is_reverse ? end : start;
    }
    
    // ◆ 执行写入操作
    std::vector<int> result;
    for (int unit_id : units)
    {
        // ● 寻找空闲单元
        while (cells[pointer].obj_id != 0) 
        {
            pointer = is_reverse ? pointer - 1 : pointer + 1;
        }
        
        // ● 写入数据
        cells[pointer].obj_id = obj_id;
        cells[pointer].unit_id = unit_id;
        cells[pointer].tag = tag;
        cells[pointer].part = part;
        part->free_cells--;
        
        // ● 更新空闲块链表
        if (part->tag != 0) 
        {
            part->allocate_block(pointer);
        }
        
        result.push_back(pointer);
    }
    
    return result;
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/