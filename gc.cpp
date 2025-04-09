#include "disk_obj_req.h"
#include "controller.h"
#include "debug.h"
#include "data_analysis.h"
#include "io.h"


void process_gc(Controller &controller)
{

    (void)scanf("%*s%*s");
    printf("GARBAGE COLLECTION\n");
    debug("time", controller.timestamp, "gc");

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
                    debug("time", controller->timestamp, "gc", "part-tag", part.tag,"obj-tag", cells[i]->tag, "obj", cells[i]->obj_id, "cell", i);
                }
                if(i == part.end) break;
            }
        }
    }

    // 如果K还有剩余，尝试一对多交换
    if(this->K > 0) {
        _gc_sizemerge_tag(gc_pairs, false);
    }
    // 如果K还有剩余，尝试引入空闲块一对多交换
    if(this->K > 0) {
        _gc_sizemerge_tag(gc_pairs, true);
    }
    // 如果K还有剩余，尝试对象和目标区域空闲块一对一交换（空闲块可以比对象大，但尽量最佳匹配）
    if(this->K > 0) {
        // _gc_free_tag(gc_pairs);
    }

    // 打印K
    debug("disk", this->id, "K", this->K);

    return gc_pairs;
}


// 交换大小拼接后匹配、tag能匹配的
void Disk::_gc_sizemerge_tag(std::vector<std::pair<int, int>>& gc_pairs, bool is_add_free)
{
    // 对所有分区进行处理
    for(int i = 1; i <= 17; i++)
    {
        auto &parts = get_parts(i, 1);
        // 遍历每个分区
        for(auto &part : parts) 
        {
            // 存储将要从part.other_objs中删除的对象ID
            std::vector<int> to_remove;
            // 遍历分区中的非本分区对象
            for(auto &other_obj : part.other_objs)
            {
                // 获取对象的标签、目标标签和大小
                int this_tag = part.tag;
                int target_tag = controller->OBJECTS[other_obj].tag;
                int obj_size = controller->OBJECTS[other_obj].size;
                bool matched = false;

                // 如果对象大小超过剩余K值，则跳过
                if(obj_size > this->K) continue;

                // 寻找目标标签的分区
                for(auto &tmp_part : get_parts(target_tag, 1))
                {
                    // 如果已经匹配成功，则退出循环
                    if(matched) break;

                    // 收集该分区中所有不属于该分区的对象（标签为this_tag的对象）
                    std::vector<int> candidate_objs;
                    for(auto &tmp_obj : tmp_part.other_objs)
                    {
                        if(controller->OBJECTS[tmp_obj].tag == this_tag && 
                           controller->OBJECTS[tmp_obj].size <= obj_size)
                        {
                            candidate_objs.push_back(tmp_obj);
                        }
                    }

                    // 尝试匹配一组对象，使其大小之和等于obj_size
                    std::vector<int> matched_objs;
                    // 允许填充的空闲cell数
                    int padding = is_add_free?std::min(tmp_part.free_cells, obj_size) : 0;

                    if(_find_size_match(candidate_objs, obj_size, matched_objs, padding))
                    {
                        // 执行一对多交换
                        _swap_obj_multiple(other_obj, matched_objs, gc_pairs, padding, &tmp_part);
                        
                        // 标记要删除的对象
                        to_remove.push_back(other_obj);
                        
                        // 从tmp_part.other_objs中删除已匹配的对象
                        for(auto &matched_obj : matched_objs)
                        {
                            tmp_part.other_objs.erase(
                                std::remove(tmp_part.other_objs.begin(), tmp_part.other_objs.end(), matched_obj), 
                                tmp_part.other_objs.end()
                            );
                        }
                        
                        // 标记已匹配
                        matched = true;
                        
                        // 更新K值
                        this->K -= obj_size;
                        assert(this->K >= 0);
                        if(this->K == 0) return;
                        
                        // 跳出tmp_part循环
                        break;
                    }
                }
                
                // 如果已经匹配，继续处理下一个对象
                if(matched) continue;
            }
            
            // 从part.other_objs中移除已交换的对象
            for(auto obj_id : to_remove)
            {
                part.other_objs.erase(
                    std::remove(part.other_objs.begin(), part.other_objs.end(), obj_id), 
                    part.other_objs.end()
                );
            }
        }
    }
}



