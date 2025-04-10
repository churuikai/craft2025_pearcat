#include "constants.h"
#include "io.h"
#include "controller.h"
#include "disk_obj_req.h"

#include <iostream>
#include <algorithm>
#include "debug.h"
#include <random>

void process_write(Controller &controller)
{
    int n_write;
    scanf("%d", &n_write);
    if (n_write == 0)
        return;
    // 处理每个写入请求
    for (int i = 0; i < n_write; ++i)
    {
        int obj_id, obj_size, tag;
        scanf("%d%d%d", &obj_id, &obj_size, &tag);
        Object *obj = controller.write(obj_id, obj_size, tag);
        printf("%d\n", obj_id);
        for (const auto &[disk_id, cell_idxs] : obj->replicas)
        {
            printf("%d ", disk_id);
            for (size_t j = 0; j < cell_idxs.size(); ++j)
                printf(" %d", cell_idxs[j]);
            printf("\n");
        }
    }
    fflush(stdout);
}

// 获取磁盘和对应分区
std::vector<std::pair<int, Part *>> Controller::_get_disk(int obj_size, int tag)
{

    std::vector<std::pair<int, Part *>> space; // <disk_id, part_idx>
    // 每过cycle个周期换一次磁盘
    // int cycle = 1;
    // int disk_start = TIME / cycle + cycle;
    // 每写入cycle_disk次，换一个磁盘，每写入cycle_op次，换一个数据区；cycle_disk必须为cycle_op的整数倍
    int cycle_disk = 4;
    int cycle_op = 1;
    assert(cycle_disk % cycle_op == 0 && "cycle_disk必须为cycle_op的整数倍");
    int disk_start = (++write_count / cycle_disk) + 1;
    int op_start = (write_count / cycle_op) + 1;

    // 优先选择对应tag对应size分区有空闲空间的磁盘
    std::vector<int> tag_list = {tag, -1, 1, 4, 6, 15, 2, 10, 13, 8, 14, 3, 12, 9, 16, 7, 11, 5};

    int strategy = 2;
    if(strategy == 1) {
        // 策略2 将索引2到tag_list[idx]=tag之间的内容移动到最后面
        auto it = std::find(tag_list.begin() + 2, tag_list.end(), tag);
        std::rotate(tag_list.begin() + 2, it+1, tag_list.end());
    }
    else if(strategy == 2) {
        // 策略3 将索引2到tag_list[idx]=tag之间的内容反向后与后面交替合并
        auto it = std::find(tag_list.begin() + 2, tag_list.end(), tag);
        std::reverse(tag_list.begin() + 2, it + 1);
        std::vector<int> first_part(tag_list.begin() + 2, it + 1);
        std::vector<int> second_part(it + 1, tag_list.end());
        tag_list.erase(tag_list.begin() + 2, tag_list.end());
        size_t i = 0, j = 0;
        while (i < first_part.size() || j < second_part.size()) {
            if (j < second_part.size())
                tag_list.push_back(second_part[j++]);
            if (i < first_part.size())
                tag_list.push_back(first_part[i++]);
        }
    }
    // 把START_TAG和END_TAG移动到最后
    auto it = std::find(++tag_list.begin(), tag_list.end(), TAG_ORDERS[0][0]);
    if(it != tag_list.end()) {
        tag_list.erase(it);
        tag_list.push_back(TAG_ORDERS[0][0]);
    }
    it = std::find(++tag_list.begin(), tag_list.end(), TAG_ORDERS[0][TAG_ORDERS[0].size()-1]); 
    if(it != tag_list.end()) {
        tag_list.erase(it);
        tag_list.push_back(TAG_ORDERS[0][TAG_ORDERS[0].size()-1]);
    }
    

    tag_list.push_back(17);
    for (int i = 2; i < tag_list.size(); ++i)
    {
        if (tag_list[i] == tag)
        {
            tag_list[i] = 0;
        }
    }
    std::vector<int> size_list = {obj_size, 1, 2, 3, 4, 5};
    size_list[obj_size] = 0;
    
    // 数据区交替写入进行
    std::vector<int> op_list = op_start % 2 == 0 ? std::vector<int>{0, 1} : std::vector<int>{1, 0};


    // 先寻找能够匹配空闲块大小的的同tag区域

        // 先尝试在相同tag和size的分区中寻找能够精确匹配空闲块大小的区域
        for (int size_ : size_list) 
        {
            if (space.size() == 3 - BACK_NUM) break;  // 如果已经找到足够的空间，则退出
            if ((!IS_PART_BY_SIZE and size_ != 1) or size_ == 0) continue;  // 跳过不符合条件的size
            
            // 遍历所有磁盘
            for (int i = 1 + disk_start; i <= N + disk_start; ++i) 
            {
                int disk_id = (i - 1) % N + 1;  // 计算实际的磁盘ID
                if (space.size() == 3 - BACK_NUM) break;  // 如果已经找到足够的空间，则退出
                // 检查该磁盘是否已经被选中
                if (std::any_of(space.begin(), space.end(), [disk_id](const auto &p) { return p.first == disk_id; })) continue;

                // 按照操作列表顺序遍历数据区
                for (int part_idx : op_list) 
                {
                    if (space.size() == 3 - BACK_NUM) break;  // 如果已经找到足够的空间，则退出
                    auto& parts = DISKS[disk_id].get_parts(tag, size_);  // 获取指定tag和size的分区列表
                    
                    // 遍历分区
                    for (auto& part : parts) 
                    {
                        if (part.free_cells < obj_size) continue;  // 如果空闲单元不足，则跳过
                        
                        // 在空闲块链表中查找大小恰好等于obj_size的块
                        FreeBlock* current = part.free_list_head;
                        while (current != nullptr) 
                        {
                            // 找到大小恰好匹配的空闲块
                            if (current->end - current->start + 1 == obj_size) 
                            {
                                space.push_back({disk_id, &part});
                                break;
                            }
                            current = current->next;
                        }
                        if (space.size() == 3 - BACK_NUM) break;  // 如果已经找到足够的空间，则退出
                    }
                }
            }
        }
    
        // 如果上面没有找到足够的空间，则尝试在不同tag或size的分区中寻找
        for (int tag_ : tag_list)
        {
            if (space.size() == 3 - BACK_NUM) break;  // 如果已经找到足够的空间，则退出
            if ((!IS_INTERVAL_REVERSE and tag_ == -1) or tag_ == 0) continue;  // 跳过不符合条件的tag
            
            // 遍历所有可能的size
            for (int size_ : size_list)
            {
                if (space.size() == 3 - BACK_NUM) break;  // 如果已经找到足够的空间，则退出
                if ((!IS_PART_BY_SIZE and size_ != 1) or size_ == 0) continue;  // 跳过不符合条件的size
                
                // 遍历所有磁盘
                for (int i = 1 + disk_start; i <= N + disk_start; ++i)
                {
                    int disk_id = (i - 1) % N + 1;  // 计算实际的磁盘ID
                    if (space.size() == 3 - BACK_NUM) break;  // 如果已经找到足够的空间，则退出
                    
                    // 检查该磁盘是否已经被选中
                    if (std::any_of(space.begin(), space.end(), [disk_id](const auto &p) { return p.first == disk_id; })) continue;
                    
                    // 处理反向tag的情况
                    tag_ = tag_ == -1 ? DISKS[disk_id].tag_reverse[tag] : tag_;
                    
                    // 按照操作列表顺序遍历数据区
                    for (int part_idx : op_list)
                    {
                        auto &parts = DISKS[disk_id].get_parts(tag_, size_);
                        if (parts.size() == 0) continue;  // 如果没有对应的分区，则跳过
                        
                        Part &part = parts[part_idx];
                        // 如果分区有足够的空闲空间
                        if (part.free_cells >= obj_size)
                        {
                            space.push_back({disk_id, &part});
                            if (space.size() == 3 - BACK_NUM) break;  // 如果已经找到足够的空间，则退出
                        }
                    }
                }
            }
        }

    assert(space.size() == 3 - BACK_NUM);

    // 寻找备份区域：在剩余磁盘中寻找有足够空间的备份区
    for (int i = 1 + disk_start; i <= N + disk_start; ++i)
    {
        // 如果已经找到所有需要的空间，则退出循环
        if (space.size() == 3) break;
        
        // 计算实际的磁盘ID（循环使用磁盘）
        int disk_id = (i - 1) % N + 1;
        
        // 跳过已经被选中的磁盘
        if (std::any_of(space.begin(), space.end(), 
                        [disk_id](const auto &p) { return p.first == disk_id; }))
            continue;
        
        // 检查备份区(tag=0, size=0)是否有足够的空闲空间
        if (DISKS[disk_id].get_parts(0, 0)[0].free_cells >= obj_size)
        {
            // 将该备份区添加到选中空间列表中
            space.push_back({disk_id, &DISKS[disk_id].get_parts(0, 0)[0]});
        }
    }

    assert(space.size() == 3);
    return space;
}

// 写入
Object *Controller::write(int obj_id, int obj_size, int tag)
{
    // 获取对象
    Object &obj = OBJECTS[obj_id];
    obj.size = obj_size;
    obj.tag = tag;
    obj.id = obj_id;

    // 获取磁盘
    auto space = _get_disk(obj_size, tag);
    assert(space.size() == 3);

    if (space.empty())
    {
        return nullptr;
    }

    // 写入磁盘
    std::vector<int> units;
    for (int i = 1; i <= obj_size; ++i)
    {
        units.push_back(i);
    }

    for (size_t i = 0; i < space.size(); ++i)
    {
        auto &[disk_id, part] = space[i];
        auto pos = DISKS[disk_id].write(obj_id, units, tag, part);
        obj.replicas[i].first = disk_id;
        obj.replicas[i].second.insert(obj.replicas[i].second.end(), pos.begin(), pos.end());
    }

    return &obj;
}

std::vector<int> Disk::write(int obj_id, const std::vector<int> &units, int tag, Part *part)
{
    if (IS_INTERVAL_REVERSE)
    {
        // 判断是否反向写入
        bool is_reverse = tag == part->tag ? part->start > part->end : part->start < part->end;
        int start = std::min(part->start, part->end);
        int end = std::max(part->start, part->end);
        
        // 优先寻找最合适的空闲单元
        int pointer = 0;
        int min_diff = 99999;
        
        // 如果是非备份区且标签匹配，尝试找到最合适的空闲块
        if (part->tag == tag && part->tag != 0)
        {
            // 通过链表找最合适的匹配（尽量减少碎片）
            FreeBlock *current = is_reverse ? part->free_list_tail : part->free_list_head;
            while (current != nullptr) 
            {
                // 检查空闲块是否足够大
                if (current->end - current->start + 1 >= units.size()) 
                {
                    int diff = current->end - current->start + 1 - units.size();
                    // 选择浪费空间最少的空闲块
                    if (diff < min_diff) 
                    {
                        min_diff = diff;
                        pointer = is_reverse ? current->end : current->start;
                    }
                }
                current = is_reverse ? current->prev : current->next;
            }
        }
        
        // 如果没找到合适的空闲块，从分区的起始或结束位置开始
        if (pointer == 0) 
        {
            pointer = is_reverse ? end : start;
        }

        std::vector<int> result;
        // 为每个单元寻找存储位置
        for (int unit_id : units)
        {
            // 寻找空闲单元
            while (cells[pointer]->obj_id != 0)
            {
                // 循环查找空闲单元
                // pointer = is_reverse ?  pointer == start ? end : pointer - 1 :
                //                         pointer == end ? start : pointer + 1; // 向前或向后移动
                pointer = is_reverse ?  pointer - 1 : pointer + 1; // 向前或向后移动
            }
            
            // 写入数据
            cells[pointer]->obj_id = obj_id;
            cells[pointer]->unit_id = unit_id;
            cells[pointer]->tag = tag;
            part->free_cells--;
            
            // 更新空闲块链表（只更新非备份区的分区）
            if (part->tag != 0) 
            {
                part->allocate_block(pointer);
            }
            
            result.push_back(pointer);
        }
        return result;
    }

    // int pointer = part[0];
    int pointer = part->last_write_pos; // 从上次写入的位置开始遍历
    std::vector<int> result;
    for (int unit_id : units)
    {
        while (cells[pointer]->obj_id != 0)
        {
            pointer = pointer == part->end ? part->start : pointer % size + 1;
        }
        // assert(pointer <= part[1] && "分区空间不足");
        Cell *cell = cells[pointer];
        // 确保目标单元确实是空闲的
        assert(cell->obj_id == 0 && "尝试写入已被占用的单元");
        
        cell->obj_id = obj_id;
        cell->unit_id = unit_id;
        cell->tag = tag;
        part->free_cells--;
        
        // 更新空闲块链表（只更新非备份区的分区）
        if (part->tag != 0) {
            part->allocate_block(pointer);
        }
        
        result.push_back(pointer);
        pointer = pointer == part->end ? part->start : pointer % size + 1;
    }
    part->last_write_pos = pointer;
    return result;
}