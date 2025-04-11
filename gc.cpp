#include "disk_obj_req.h"
#include "controller.h"
#include "debug.h"
#include "data_analysis.h"
#include "io.h"
void process_gc(Controller &controller)
{

    (void)scanf("%*s%*s");
    printf("GARBAGE COLLECTION\n");
    // debug("time", controller.timestamp, "gc");

    for (int i = 1; i <= N; i++)
    {
        auto gc_pairs = controller.DISKS[i].gc();
        printf("%d\n", (int)gc_pairs.size());
        for (auto &pair : gc_pairs)
        {
            printf("%d %d\n", pair.first, pair.second);
        }
    }
    fflush(stdout);
}

std::vector<std::pair<int, int>> Disk::gc() {
    std::vector<std::pair<int, int>> gc_pairs;
    assert(not IS_PART_BY_SIZE);

    // 遍历每个分区，找到不属于该分区的对象

    for(int i = 1; i <= 17; i++)
    {
        auto &parts = get_parts(i, 1);
        for(auto &part : parts) 
        {
            for(int i = part.start; true; i+= part.start < part.end?1:-1) 
            {
                assert(cells[i]->part->tag == part.tag);
                if(cells[i]->obj_id != 0 and cells[i]->tag != part.tag) 
                {
                    // 如果不在part.other_objs中，则加入
                    if(std::find(part.other_objs.begin(), part.other_objs.end(), cells[i]->obj_id) == part.other_objs.end())
                    {
                        part.other_objs.push_back(cells[i]->obj_id);
                    }
                }
                if(i == part.end) break;
            }
        }
    }
    // debug("disk", this->id, "--------------------------");
    // 如果K还有剩余，尝试一对多组合排列交换
    if(this->K > 0) {
        _disk_gc_s2m(gc_pairs, false);
    }
    // 如果K还有剩余，尝试引入空闲块组合排列交换
    if(this->K > 0) {
        _disk_gc_s2m(gc_pairs, true);
    }
    // 如果K还有剩余，尝试多对多交换
    if(this->K > 0) {
        _disk_gc_m2m(gc_pairs);
    }
 
    // 获取读取频率排序的tag
    std::vector<int> sorted_tags = get_sorted_read_tag(controller->timestamp+180);
    std::reverse(sorted_tags.begin(), sorted_tags.end());

    // 遍历每个tag
    for(auto &tag : sorted_tags)
    {
        // 获取该tag的分区
        auto &parts = get_parts(tag, 1);
        // 遍历每个分区
        for(auto &part : parts)
        {
            if(this->K > 0)
            {
                // 分区内部聚拢
                _part_gc_inner(part, gc_pairs, false);
            }
        }
    }

    
    // 遍历每个tag
    for(auto &tag : sorted_tags)
    {
        // 获取该tag的分区
        auto &parts = get_parts(tag, 1);
        // 遍历每个分区
        for(auto &part : parts)
        {
            if(this->K > 0)
            {
                // 分区内部聚拢
                _part_gc_inner(part, gc_pairs, true);
            }
        }
    }

    // 打印K
    // debug("剩余 K", this->K);


    return gc_pairs;
}


// 分区内部匹配
void Disk::_part_gc_inner(Part& part, std::vector<std::pair<int, int>>& gc_pairs, bool is_split_obj)
{
    // 从end向start聚拢
    int end = part.end;
    int start = part.start;
    bool is_reverse = end < start;
    if (not is_split_obj or true) {
        while(end != start) {
            // 找到末端第一个完整的obj
            if(cells[end]->obj_id == 0 or controller->OBJECTS[cells[end]->obj_id].tag != part.tag)
            {
                end += is_reverse ? 1 : -1;
                continue;
            }
            else
            {
                assert(cells[end]->obj_id != 0 and controller->OBJECTS[cells[end]->obj_id].tag == part.tag);
                int obj_size = controller->OBJECTS[cells[end]->obj_id].size;
                int obj_id = cells[end]->obj_id;

                std::vector<int> candidate_cells;
                while(cells[end]->obj_id == obj_id)
                {
                    assert(is_reverse ? end <= start : end >= start);
                    candidate_cells.push_back(end);
                    if(end != start) end += is_reverse ? 1 : -1;
                }
                if(end == start) break;
                
                assert(candidate_cells.size() > 0);

                // 找到start-end中最佳匹配的空闲区或其他对象
                
                // 收集start-end中所有空闲块和其它对象块
                std::vector<std::vector<int>> candidate_blocks;
                std::vector<std::vector<int>> candidate_blocks_tmp;
                int i = start;
                while(true)
                {
                    if(controller->OBJECTS[cells[i]->obj_id].tag != part.tag)
                    {
                        candidate_blocks.push_back({});
                        while(controller->OBJECTS[cells[i]->obj_id].tag != part.tag or cells[i]->obj_id == 0)
                        {
                            candidate_blocks.back().push_back(i);
                            if(i == end) break;
                            i += is_reverse ? -1 : 1;
                        }
                        if(i == end) break;
                    }
                    else if(cells[i]->obj_id == 0)
                    {
                        candidate_blocks_tmp.push_back({});
                        while(cells[i]->obj_id == 0 or controller->OBJECTS[cells[i]->obj_id].tag != part.tag)
                        {
                            candidate_blocks_tmp.back().push_back(i);
                            if(i == end) break;
                            i += is_reverse ? -1 : 1;
                        }
                        if(i == end) break;
                    }
                    else
                    {
                        if(i == end) break;
                        i += is_reverse ? -1 : 1;
                    }
                }
                // 合并candidate_blocks_tmp
                candidate_blocks.insert(candidate_blocks.end(), candidate_blocks_tmp.begin(), candidate_blocks_tmp.end());

                // 找到start-end中最佳匹配的空闲区或其他对象
                int best_diff = 9999;
                int best_block_idx = -1;
                for(int i = 0; i < candidate_blocks.size(); i++)
                {
                    assert(candidate_blocks[i].size() > 0);
                    int diff = candidate_blocks[i].size() - candidate_cells.size();
                    if(diff < best_diff and diff >= 0)
                    {
                        best_diff = diff;
                        best_block_idx = i;
                    }
                }
                if(best_block_idx != - 1)
                {
                    for(int i = 0; i < candidate_cells.size(); i++)
                    {
                        int cell_idx1 = candidate_cells[i];
                        int cell_idx2 = candidate_blocks[best_block_idx][i];
                        int obj_id1 = cells[cell_idx1]->obj_id;
                        int obj_id2 = cells[cell_idx2]->obj_id;
                        int obj_size1 = controller->OBJECTS[obj_id1].size;
                        int obj_size2 = controller->OBJECTS[obj_id2].size;
                        int obj_tag1 = controller->OBJECTS[obj_id1].tag;
                        int obj_tag2 = controller->OBJECTS[obj_id2].tag;
                        int part_tag1 = cells[cell_idx1]->part->tag;
                        int part_tag2 = cells[cell_idx2]->part->tag;
                        // debug("inr", "id", obj_id1,"tag",obj_tag1, "size", obj_size1, "cell", cell_idx1, "part", part_tag1, "---", "id", obj_id2, "tag", obj_tag2, "size", obj_size2, "cell", cell_idx2, "part", part_tag2);
            
                        _swap_cell(candidate_cells[i], candidate_blocks[best_block_idx][i]);
                        gc_pairs.push_back({candidate_cells[i], candidate_blocks[best_block_idx][i]});
                        this->K -= 1;
                        if(this->K == 0) return;
                    }
                }
                // 如果找不到最佳匹配，则按传统方法
                else
                {
                    if(not is_split_obj) return;
                    // 从start端开始找空闲块
                    std::reverse(candidate_cells.begin(), candidate_cells.end());
                    for(int i = start; ; i += is_reverse ? -1 : 1)
                    {
                        if(controller->OBJECTS[cells[i]->obj_id].tag != part.tag)
                        {
                            int cell_idx1 = i;
                            int cell_idx2 = candidate_cells.back();
                            int obj_id1 = cells[cell_idx1]->obj_id;
                            int obj_id2 = cells[cell_idx2]->obj_id;
                            int obj_size1 = controller->OBJECTS[obj_id1].size;
                            int obj_size2 = controller->OBJECTS[obj_id2].size;
                            int obj_tag1 = controller->OBJECTS[obj_id1].tag;
                            int obj_tag2 = controller->OBJECTS[obj_id2].tag;
                            int part_tag1 = cells[cell_idx1]->part->tag;
                            int part_tag2 = cells[cell_idx2]->part->tag;
                            // debug("inr-split", "id", obj_id1,"tag",obj_tag1, "size", obj_size1, "cell", cell_idx1, "part", part_tag1, "---", "id", obj_id2, "tag", obj_tag2, "size", obj_size2, "cell", cell_idx2, "part", part_tag2);
                
                            _swap_cell(i, candidate_cells.back());
                            gc_pairs.push_back({i, candidate_cells.back()});
                            candidate_cells.pop_back();
                            
                            this->K -= 1;
                            if(candidate_cells.size() == 0) break;
                            if(this->K == 0) return;
                        }
                        if(i == end) break;
                    }
                    for(int i = start; ; i += is_reverse ? -1 : 1)
                    {
                        if(candidate_cells.size() == 0) break;
                        if(cells[i]->obj_id == 0)
                        {
                            int cell_idx1 = i;
                            int cell_idx2 = candidate_cells.back();
                            int obj_id1 = cells[cell_idx1]->obj_id;
                            int obj_id2 = cells[cell_idx2]->obj_id;
                            int obj_size1 = controller->OBJECTS[obj_id1].size;
                            int obj_size2 = controller->OBJECTS[obj_id2].size;
                            int obj_tag1 = controller->OBJECTS[obj_id1].tag;
                            int obj_tag2 = controller->OBJECTS[obj_id2].tag;
                            int part_tag1 = cells[cell_idx1]->part->tag;
                            int part_tag2 = cells[cell_idx2]->part->tag;
                            // debug("inr-split", "id", obj_id1,"tag",obj_tag1, "size", obj_size1, "cell", cell_idx1, "part", part_tag1, "---", "id", obj_id2, "tag", obj_tag2, "size", obj_size2, "cell", cell_idx2, "part", part_tag2);
                
                            _swap_cell(i, candidate_cells.back());
                            gc_pairs.push_back({i, candidate_cells.back()});
                            candidate_cells.pop_back();
                            
                            this->K -= 1;
                            if(candidate_cells.size() == 0) break;
                            if(this->K == 0) return;
                        }
                        if(i == end) break;
                    }
                    // 如果还有剩余的cell，则已经没有空闲空间
                    if(candidate_cells.size() > 0) return;
                }
            }
            // 如果K已经用完，则退出
            if(this->K == 0) return;

        }
    }
    return;

}


// 交换大小拼接后匹配、tag能匹配的
void Disk::_disk_gc_s2m(std::vector<std::pair<int, int>>& gc_pairs, bool is_add_free)
{
    // 对所有分区进行处理
    for(int i = 1; i <= 17; i++)
    {
        auto &parts = get_parts(i, 1);
        // 遍历每个分区
        for(auto &part : parts) 
        {
            _part_gc_s2m(part, gc_pairs, is_add_free);
        }
    }
}

void Disk::_part_gc_s2m(Part &part, std::vector<std::pair<int, int>> &gc_pairs, bool is_add_free)
{
    // 存储将要从part.other_objs中删除的对象ID
    std::vector<int> to_remove;
    // 遍历分区中的非本分区对象
    for (auto &other_obj : part.other_objs)
    {
        assert(part.tag != controller->OBJECTS[other_obj].tag);
        // 获取对象的标签、目标标签和大小
        int this_tag = part.tag;
        int target_tag = controller->OBJECTS[other_obj].tag;
        int obj_size = controller->OBJECTS[other_obj].size;
        bool matched = false;

        // 如果对象大小超过剩余K值，则跳过
        if (obj_size > this->K)
            continue;

        // 寻找目标标签的分区
        for (auto &tmp_part : get_parts(target_tag, 1))
        {
            assert(tmp_part.tag == target_tag);
            // 如果已经匹配成功，则退出循环
            if (matched)
                break;

            // 收集该分区中所有不属于该分区的对象（标签为this_tag的对象）
            std::vector<int> candidate_objs;
            for (auto &tmp_obj : tmp_part.other_objs)
            {
                if (controller->OBJECTS[tmp_obj].tag == this_tag &&
                    controller->OBJECTS[tmp_obj].size <= obj_size)
                {
                    candidate_objs.push_back(tmp_obj);
                }
            }

            // 尝试匹配一组对象，使其大小之和等于obj_size
            std::vector<int> matched_objs;
            // 允许填充的空闲cell数
            int padding = is_add_free ? std::min(tmp_part.free_cells, obj_size) : 0;

            if (_find_s2m_match(candidate_objs, obj_size, matched_objs, padding))
            {
                // 执行一对多交换
                _swap_s2m(other_obj, matched_objs, gc_pairs, padding, &tmp_part);

                // 标记要删除的对象
                to_remove.push_back(other_obj);

                // 从tmp_part.other_objs中删除已匹配的对象
                for (auto &matched_obj : matched_objs)
                {
                    tmp_part.other_objs.erase(
                        std::remove(tmp_part.other_objs.begin(), tmp_part.other_objs.end(), matched_obj),
                        tmp_part.other_objs.end());
                }

                // 标记已匹配
                matched = true;

                // 更新K值
                this->K -= obj_size;
                assert(this->K >= 0);
                if (this->K == 0)
                    return;

                // 跳出tmp_part循环
                break;
            }
        }

        // 如果已经匹配，继续处理下一个对象
        if (matched)
            continue;
    }

    // 从part.other_objs中移除已交换的对象
    for (auto obj_id : to_remove)
    {
        part.other_objs.erase(
            std::remove(part.other_objs.begin(), part.other_objs.end(), obj_id),
            part.other_objs.end());
    }
}

// 多对多交换
void Disk::_disk_gc_m2m(std::vector<std::pair<int, int>>& gc_pairs)
{
    // 对所有分区进行处理
    for(int i = 1; i <= 17; i++)
    {
        auto &parts = get_parts(i, 1);
        // 遍历每个分区
        for(auto &part : parts) 
        {
            _part_gc_m2m(part, gc_pairs);
        }
    }
}

void Disk::_part_gc_m2m(Part &part, std::vector<std::pair<int, int>> &gc_pairs)
{
    for(int tag_idx = 1; tag_idx <= 17; tag_idx++)
    {
        auto &parts = get_parts(tag_idx, 1);
        for(auto &tmp_part : parts)
        {
            // 收集本分区中对方分区的对象
            std::vector<int> candidate_objs1;
            for(auto &obj_id : part.other_objs)
            {
                if(controller->OBJECTS[obj_id].tag == tmp_part.tag)
                {
                    candidate_objs1.push_back(obj_id);
                }
            }
            // 收集对方分区中本分区的对象
            std::vector<int> candidate_objs2;
            for(auto &obj_id : tmp_part.other_objs)
            {
                if(controller->OBJECTS[obj_id].tag == part.tag)
                {
                    candidate_objs2.push_back(obj_id);
                }
            }
            if(candidate_objs1.size() == 0 || candidate_objs2.size() == 0) continue;
            // 尝试匹配一组对象，使其大小之和等于obj_size
            std::vector<int> matched_objs1;
            std::vector<int> matched_objs2;
            if(_find_m2m_match(candidate_objs1, candidate_objs2, matched_objs1, matched_objs2, this->K))
            {
                // 获取大小
                int size1 = 0;
                int size2 = 0;
                for(auto &obj_id : matched_objs1) size1 += controller->OBJECTS[obj_id].size;
                for(auto &obj_id : matched_objs2) size2 += controller->OBJECTS[obj_id].size;
 
                assert(size1 == size2 and size1 <= this->K);
                // 执行多对多交换
                _swap_m2m(matched_objs1, matched_objs2, gc_pairs);
                // 更新K
                this->K -= size1;
                // 更新part.other_objs
                for(auto &obj_id : matched_objs1)
                {
                    part.other_objs.erase(
                        std::remove(part.other_objs.begin(), part.other_objs.end(), obj_id),
                        part.other_objs.end());
                }
                // 更新tmp_part.other_objs
                for(auto &obj_id : matched_objs2)
                {
                    tmp_part.other_objs.erase(
                        std::remove(tmp_part.other_objs.begin(), tmp_part.other_objs.end(), obj_id),
                        tmp_part.other_objs.end());
                }
                
                if(this->K == 0) return;


            }

        }
    }
}