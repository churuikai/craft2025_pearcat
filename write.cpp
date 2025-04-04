#include "disk_obj_req.h"
#include "constants.h"
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

    int strategy = 3;
    if(strategy == 1) {
        std::mt19937 gen(TIME);
        // 策略1 随机打乱 tag_list.begin() + 2, tag_list.end()
        std::shuffle(tag_list.begin() + 2, tag_list.end(), gen);
    }
    else if(strategy == 2) {
        // 策略2 将索引2到tag_list[idx]=tag之间的内容移动到最后面
        auto it = std::find(tag_list.begin() + 2, tag_list.end(), tag);
        std::rotate(tag_list.begin() + 2, it+1, tag_list.end());
    }
    else if(strategy == 3) {
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

    for (int tag_ : tag_list)
    {
        if (space.size() == 3 - BACK_NUM)
            break;
        if ((!IS_INTERVAL_REVERSE and tag_ == -1) or tag_ == 0)
            continue;
        for (int size_ : size_list)
        {
            if (space.size() == 3 - BACK_NUM)
                break;
            if ((!IS_PART_BY_SIZE and size_ != 1) or size_ == 0)
                continue;
            for (int i = 1 + disk_start; i <= N + disk_start; ++i)
            {
                int disk_id = (i - 1) % N + 1;
                // 检查磁盘是否已经在space中
                if (space.size() == 3 - BACK_NUM)
                    break;
                if (std::any_of(space.begin(), space.end(), [disk_id](const auto &p)
                                { return p.first == disk_id; }))
                    continue;
                tag_ = tag_ == -1 ? DISKS[disk_id].tag_reverse[tag] : tag_;
                // for(auto& part : DISKS[disk_id].get_parts(tag_, size_))
                for (int part_idx : op_list)
                {
                    auto &parts = DISKS[disk_id].get_parts(tag_, size_);
                    if (parts.size() == 0)
                        continue;
                    Part &part = parts[part_idx];
                    if (part.free_cells >= obj_size)
                    {
                        space.push_back({disk_id, &part});
                        if (space.size() == 3 - BACK_NUM)
                            break;
                    }
                }
            }
        }
    }

    assert(space.size() == 3 - BACK_NUM && "数据区域磁盘空间不足");

    // 寻找备份区
    for (int i = 1 + disk_start; i <= N + disk_start; ++i)
    {
        if (space.size() == 3)
            break;
        int disk_id = (i - 1) % N + 1;
        if (std::any_of(space.begin(), space.end(), [disk_id](const auto &p)
                        { return p.first == disk_id; }))
            continue;
        if (DISKS[disk_id].get_parts(0, 0)[0].free_cells >= obj_size)
        {
            space.push_back({disk_id, &DISKS[disk_id].get_parts(0, 0)[0]});
        }
    }

    assert(space.size() == 3 && "备份区磁盘空间不足");
    return space;
}

// 写入
Object *Controller::write(int obj_id, int obj_size, int tag)
{
    // 获取对象
    Object &obj = OBJECTS[obj_id];
    obj.size = obj_size;
    obj.tag = tag;

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
        // 同tag同向，不同tag反向
        int pointer = (tag == part->tag) ? part->start : part->end;
        std::vector<int> result;
        for (int unit_id : units)
        {
            while (cells[pointer]->obj_id != 0)
            {
                if (tag == part->tag)
                {
                    pointer = part->start < part->end ? pointer + 1 : pointer - 1;
                }
                else
                {
                    pointer = part->start > part->end ? pointer + 1 : pointer - 1;
                }
            }
            cells[pointer]->obj_id = obj_id;
            cells[pointer]->unit_id = unit_id;
            cells[pointer]->tag = tag;
            part->free_cells--;
            result.push_back(pointer);
            // pointer = pointer == part[1] ? part[0] : pointer % size + 1;
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
        cell->obj_id = obj_id;
        cell->unit_id = unit_id;
        cell->tag = tag;
        part->free_cells--;
        result.push_back(pointer);
        pointer = pointer == part->end ? part->start : pointer % size + 1;
    }
    part->last_write_pos = pointer;
    return result;
}