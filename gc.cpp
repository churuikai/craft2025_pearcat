#include "disk_obj_req.h"
#include "controller.h"
#include "debug.h"
#include "data_analysis.h"
#include "io.h"


void process_gc(Controller &controller)
{

    (void)scanf("%*s%*s");
    printf("GARBAGE COLLECTION\n");

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

    // 首先尝试进行对象互相交换
    for(int i = 1; i <= 17; i++)
    {
        auto &parts = get_parts(i, 1);
        // 遍历每个分区
        for(auto &part : parts) 
        {
            // 使用vector来存储将要删除的对象ID
            std::vector<int> to_remove;
            // 遍历每个分区的其他对象
            for(auto &other_obj : part.other_objs)
            {
                assert(controller->OBJECTS[other_obj].tag != part.tag);

                // 寻找该对象应在的分区是否有大小对应的该分区的对象
                int this_tag = part.tag;
                int target_tag = controller->OBJECTS[other_obj].tag;
                int obj_size = controller->OBJECTS[other_obj].size;
                bool swapped = false;
                for(auto &tmp_part : get_parts(target_tag, 1))
                {
                    // 记录要从tmp_part.other_objs中删除的对象
                    std::vector<int> tmp_to_remove;
                    // 遍历该分区的其他对象
                    for(auto &tmp_obj : tmp_part.other_objs)
                    {
                        int tmp_size = controller->OBJECTS[tmp_obj].size;
                        int tmp_tag = controller->OBJECTS[tmp_obj].tag;

                        // 如果该对象大小相同，且属于本分区，则交换
                        if(tmp_size == obj_size and tmp_tag == this_tag)
                        {
                            _swap_obj(other_obj, tmp_obj, gc_pairs);
                            to_remove.push_back(other_obj);
                            tmp_to_remove.push_back(tmp_obj);
                            swapped = true;
                            this->K--;
                            if(this->K == 0) return gc_pairs;
                            break;
                        }
                    }
                    // 从tmp_part.other_objs中移除已交换的对象
                    if(!tmp_to_remove.empty()) {
                        for(auto obj_id : tmp_to_remove) {
                            tmp_part.other_objs.erase(
                                std::remove(tmp_part.other_objs.begin(), tmp_part.other_objs.end(), obj_id), 
                                tmp_part.other_objs.end()
                            );
                        }
                    }
                    if(swapped) break;
                }
                if(swapped) continue;
            }
            // 从part.other_objs中移除已交换的对象
            for(auto obj_id : to_remove) {
                part.other_objs.erase(
                    std::remove(part.other_objs.begin(), part.other_objs.end(), obj_id), 
                    part.other_objs.end()
                );
            }
        }
    }

    // 然后尝试与放进对方的空闲块

    return gc_pairs;
}



void Disk::_swap_obj(int obj_idx1, int obj_idx2, std::vector<std::pair<int, int>>& gc_pairs)
{
    assert(controller->OBJECTS[obj_idx1].size == controller->OBJECTS[obj_idx2].size);
    assert(controller->OBJECTS[obj_idx1].replicas[0].first == this->id);
    assert(controller->OBJECTS[obj_idx2].replicas[0].first == this->id);
    // 交换两个对象的单元格
    for(int i = 0; i < controller->OBJECTS[obj_idx1].replicas[0].second.size(); ++i)
    {
        assert(cells[controller->OBJECTS[obj_idx1].replicas[0].second[i]]->obj_id == obj_idx1);
        assert(cells[controller->OBJECTS[obj_idx2].replicas[0].second[i]]->obj_id == obj_idx2);
     
        int cell_idx1 = controller->OBJECTS[obj_idx1].replicas[0].second[i];
        int cell_idx2 = controller->OBJECTS[obj_idx2].replicas[0].second[i];
     
        _swap_cell(cell_idx1, cell_idx2);
        gc_pairs.push_back({cell_idx1, cell_idx2});
    }
}

void Disk::_swap_cell(int cell_idx1, int cell_idx2) {
    // 维护分区
    Cell *cell1 = cells[cell_idx1];
    Cell *cell2 = cells[cell_idx2];
    if(cell1->obj_id == 0 and cell2->obj_id == 0) return;
    if(cell1->obj_id == 0 and cell2->obj_id != 0) 
    {
        cell1->part->allocate_block(cell_idx1);
        cell2->part->free_block(cell_idx2);
        cell1->part->free_cells--;
        cell2->part->free_cells++;
    }
    else if(cell1->obj_id != 0 and cell2->obj_id == 0)
    {
        cell1->part->free_block(cell_idx1);
        cell2->part->allocate_block(cell_idx2);
        cell1->part->free_cells++;
        cell2->part->free_cells--;
    }
    // 维护对象
    Object &obj1 = controller->OBJECTS[cell1->obj_id];
    Object &obj2 = controller->OBJECTS[cell2->obj_id];

    assert(this->id == obj1.replicas[0].first);
    assert(this->id == obj2.replicas[0].first);


    if(cell1->obj_id != 0) {
        obj1.replicas[0].second[cell1->unit_id-1] = cell_idx2;
    }
    if(cell2->obj_id != 0) {
        obj2.replicas[0].second[cell2->unit_id-1] = cell_idx1;
    }

    // 交换单元格
    std::swap(cells[cell_idx1]->obj_id, cells[cell_idx2]->obj_id);
    std::swap(cells[cell_idx1]->unit_id, cells[cell_idx2]->unit_id);
    std::swap(cells[cell_idx1]->req_ids, cells[cell_idx2]->req_ids);
    std::swap(cells[cell_idx1]->tag, cells[cell_idx2]->tag);
}