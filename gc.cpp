/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 *   ██████╗  ██████╗
 *  ██╔════╝ ██╔════╝
 *  ██║  ███╗██║     
 *  ██║   ██║██║     
 *  ╚██████╔╝╚██████╗
 *   ╚═════╝  ╚═════╝
 * 
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 * 【模块功能】
 * ┌─────────────────┬───────────────────────────────────────────────────────────┐
 * │ 垃圾回收         │ 实现磁盘空间的垃圾回收和碎片整理                           │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 空间优化         │ 合并小型空闲块，提高空间利用率                             │
 * ├─────────────────┼───────────────────────────────────────────────────────────┤
 * │ 数据迁移         │ 优化数据布局，减少碎片化                                  │
 * └─────────────────┴───────────────────────────────────────────────────────────┘
 * ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#include "ctrl_disk_obj_req.h"
#include "debug.h"
#include "data_analysis.h"
#include <algorithm>

/*╔══════════════════════════════ 垃圾回收主函数 ═══════════════════════════════╗*/
/**
 * @brief     执行磁盘垃圾回收操作
 * @return    std::vector<std::pair<int, int>> 交换操作的单元格对列表
 * @details   按优先级执行以下垃圾回收策略:
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ 1. 更新各分区中不属于该分区的对象                                       │
 * │ 2. 尝试一对多组合排列交换                                              │
 * │ 3. 尝试引入空闲块一对多组合排列交换                                     │
 * │ 4. 尝试多对多交换                                                     │
 * │ 5. 分区内部聚拢(不分割对象)                                            │
 * │ 6. 分区内部聚拢(分割对象)                                              │
 * └──────────────────────────────────────────────────────────────────────┘
 */
std::vector<std::pair<int, int>> Disk::gc() 
{
    std::vector<std::pair<int, int>> gc_pairs;

    // ◆ 遍历每个分区，找到不属于该分区的对象
    _update_other_objs();

    // ◆ 如果K有剩余，尝试一对多组合排列交换
    if(this->K > 0) _disk_gc_s2m(gc_pairs, false);

    // ◆ 如果K有剩余，尝试引入空闲块一对多组合排列交换
    if(this->K > 0) _disk_gc_s2m(gc_pairs, true);

    // ◆ 如果K有剩余，尝试多对多交换
    if(this->K > 0) _disk_gc_m2m(gc_pairs);

    // ◆ 获取读取频率排序的tag
    std::vector<int> sorted_tags = get_sorted_read_tag(controller->timestamp+1);
    std::reverse(sorted_tags.begin(), sorted_tags.end());

    // ◆ 遍历每个分区,分区内部聚拢 (不分割对象)
    for(auto &tag : sorted_tags)
    {
        for(auto &part : get_parts(tag))
        {
            if(this->K > 0) _part_gc_inner(part, gc_pairs, false);
        }
    }

    // ◆ 遍历每个分区,分区内部聚拢 (分割对象)
    for(auto &tag : sorted_tags)
    {
        for(auto &part : get_parts(tag))
        {
            if(this->K > 0) _part_gc_inner(part, gc_pairs, true);
        }
    }

    return gc_pairs;
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/

/*╔══════════════════════════════ 辅助函数实现 ═══════════════════════════════╗*/
/**
 * @brief     更新各个分区内不属于该分区的对象
 * @details   遍历所有分区，找出并记录每个分区中不属于该分区的对象
 */
void Disk::_update_other_objs()
{
    for(int i = 1; i <= 17; i++)
    {
        auto &parts = get_parts(i);
        for(auto &part : parts) 
        {
            for(int i = part.start; true; i+= part.start < part.end?1:-1) 
            {
                assert(cells[i].part->tag == part.tag);
                if(cells[i].obj_id != 0 and cells[i].tag != part.tag) 
                {
                    // 如果不在part.other_objs中，则加入
                    if(std::find(part.other_objs.begin(), part.other_objs.end(), cells[i].obj_id) == part.other_objs.end())
                    {
                        part.other_objs.push_back(cells[i].obj_id);
                    }
                }
                if(i == part.end) break;
            }
        }
    }
}

/**
 * @brief     分区内部匹配
 * @param     part 待处理的分区
 * @param     gc_pairs 记录交换操作的列表
 * @param     is_split_obj 是否允许分割对象
 * @details   从分区末端向起始端聚拢，优化数据布局
 */
void Disk::_part_gc_inner(Part& part, std::vector<std::pair<int, int>>& gc_pairs, bool is_split_obj)
{
    // 从end向start聚拢
    int end = part.end;
    int start = part.start;
    bool is_reverse = end < start;
    if (not is_split_obj or true) {
        while(end != start) {
            // 找到末端第一个完整的obj
            if(cells[end].obj_id == 0 or controller->OBJECTS[cells[end].obj_id].tag != part.tag)
            {
                end += is_reverse ? 1 : -1;
                continue;
            }
            else
            {
                assert(cells[end].obj_id != 0 and controller->OBJECTS[cells[end].obj_id].tag == part.tag);
                int obj_size = controller->OBJECTS[cells[end].obj_id].size;
                int obj_id = cells[end].obj_id;

                std::vector<int> candidate_cells;
                while(cells[end].obj_id == obj_id)
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
                    if(controller->OBJECTS[cells[i].obj_id].tag != part.tag)
                    {
                        candidate_blocks.push_back({});
                        while(controller->OBJECTS[cells[i].obj_id].tag != part.tag or cells[i].obj_id == 0)
                        {
                            candidate_blocks.back().push_back(i);
                            if(i == end) break;
                            i += is_reverse ? -1 : 1;
                        }
                        if(i == end) break;
                    }
                    else if(cells[i].obj_id == 0)
                    {
                        candidate_blocks_tmp.push_back({});
                        while(cells[i].obj_id == 0 or controller->OBJECTS[cells[i].obj_id].tag != part.tag)
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
                        if(controller->OBJECTS[cells[i].obj_id].tag != part.tag)
                        {
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
                        if(cells[i].obj_id == 0)
                        {
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

/**
 * @brief     交换大小拼接后匹配、tag能匹配的对象
 * @param     gc_pairs 记录交换操作的列表
 * @param     is_add_free 是否允许使用空闲块
 * @details   对所有分区执行一对多交换操作
 */
void Disk::_disk_gc_s2m(std::vector<std::pair<int, int>>& gc_pairs, bool is_add_free)
{
    // 对所有分区进行处理
    for(int i = 1; i <= 17; i++)
    {
        auto &parts = get_parts(i);
        // 遍历每个分区
        for(auto &part : parts) 
        {
            _part_gc_s2m(part, gc_pairs, is_add_free);
        }
    }
}

/**
 * @brief     执行分区级别的一对多交换
 * @param     part 待处理的分区
 * @param     gc_pairs 记录交换操作的列表
 * @param     is_add_free 是否允许使用空闲块
 */
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
        for (auto &tmp_part : get_parts(target_tag))
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

/**
 * @brief     执行多对多交换操作
 * @param     gc_pairs 记录交换操作的列表
 * @details   遍历所有分区，尝试进行多对多对象交换
 */
void Disk::_disk_gc_m2m(std::vector<std::pair<int, int>>& gc_pairs)
{
    // 对所有分区进行处理
    for(int i = 1; i <= 17; i++)
    {
        auto &parts = get_parts(i);
        // 遍历每个分区
        for(auto &part : parts) 
        {
            _part_gc_m2m(part, gc_pairs);
        }
    }
}

/**
 * @brief     执行分区级别的多对多交换
 * @param     part 待处理的分区
 * @param     gc_pairs 记录交换操作的列表
 */
void Disk::_part_gc_m2m(Part &part, std::vector<std::pair<int, int>> &gc_pairs)
{
    for(int tag_idx = 1; tag_idx <= 17; tag_idx++)
    {
        auto &parts = get_parts(tag_idx);
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

/**
 * @brief     交换单元格内容
 * @param     cell_idx1 第一个单元格索引
 * @param     cell_idx2 第二个单元格索引
 * @details   维护分区信息和对象信息，执行单元格交换
 */
void Disk::_swap_cell(int cell_idx1, int cell_idx2) {
    assert(cell_idx1 != cell_idx2);
    // 维护分区
    Cell *cell1 = &cells[cell_idx1];
    Cell *cell2 = &cells[cell_idx2];
    if(cell1->obj_id == 0 and cell2->obj_id == 0) return;
    if(cell1->obj_id == 0 and cell2->obj_id != 0) 
    {

        cell1->part->allocate_block(cell_idx1);
        cell1->part->free_cells--;
        cell2->part->free_block(cell_idx2);
        cell2->part->free_cells++;
    }
    else if(cell1->obj_id != 0 and cell2->obj_id == 0)
    {
        cell2->part->allocate_block(cell_idx2);
        cell2->part->free_cells--;
        cell1->part->free_block(cell_idx1);
        cell1->part->free_cells++;
    }
    // 维护对象
    Object &obj1 = controller->OBJECTS[cell1->obj_id];
    Object &obj2 = controller->OBJECTS[cell2->obj_id];

    if(cell1->obj_id != 0) {
        assert(this->id == obj1.replicas[0].first);
        obj1.replicas[0].second[cell1->unit_id-1] = cell_idx2;
    }
    if(cell2->obj_id != 0) {
        assert(this->id == obj2.replicas[0].first);
        obj2.replicas[0].second[cell2->unit_id-1] = cell_idx1;
    }

    // 交换单元格
    std::swap(cells[cell_idx1].obj_id, cells[cell_idx2].obj_id);
    std::swap(cells[cell_idx1].unit_id, cells[cell_idx2].unit_id);
    std::swap(cells[cell_idx1].req_ids, cells[cell_idx2].req_ids);
    std::swap(cells[cell_idx1].tag, cells[cell_idx2].tag);
}

/**
 * @brief     多对多匹配函数
 * @param     candidate_objs1 第一组候选对象
 * @param     candidate_objs2 第二组候选对象
 * @param     matched_objs1 第一组匹配结果
 * @param     matched_objs2 第二组匹配结果
 * @param     max_target_size 最大目标大小
 * @return    bool 是否找到匹配
 * @details   尽可能找到最大的匹配组合
 */
bool Disk::_find_m2m_match(const std::vector<int>& candidate_objs1, 
                          const std::vector<int>& candidate_objs2, 
                          std::vector<int>& matched_objs1,
                          std::vector<int>& matched_objs2,
                          int max_target_size)
{
    // 准备两组对象的大小列表
    std::vector<std::pair<int, int>> size_obj_pairs1; // <size, obj_id>
    std::vector<std::pair<int, int>> size_obj_pairs2;
    
    for(auto obj_id : candidate_objs1) {
        size_obj_pairs1.push_back({controller->OBJECTS[obj_id].size, obj_id});
    }
    for(auto obj_id : candidate_objs2) {
        size_obj_pairs2.push_back({controller->OBJECTS[obj_id].size, obj_id});
    }
 
    // 计算两组对象的总大小
    int total_size1 = 0, total_size2 = 0;
    for(const auto& pair : size_obj_pairs1) total_size1 += pair.first;
    for(const auto& pair : size_obj_pairs2) total_size2 += pair.first;
    
    // 最大可能的目标大小是两组对象总大小的较小值
    max_target_size = std::min(std::min(total_size1, total_size2), max_target_size);
    
    // 从最大可能的目标大小开始向下搜索
    for(int target_size = max_target_size; target_size > 0; target_size--) {
        std::vector<int> tmp_matched1, tmp_matched2;
        int padding1 = 0, padding2 = 0;
        
        // 尝试在第一组对象中找到匹配
        bool found1 = _find_s2m_match(candidate_objs1, target_size, tmp_matched1, padding1);
        if(!found1) continue;
        
        // 尝试在第二组对象中找到匹配
        bool found2 = _find_s2m_match(candidate_objs2, target_size, tmp_matched2, padding2);
        if(!found2) continue;
        
        // 如果两组对象都找到了匹配，则返回结果
        matched_objs1 = tmp_matched1;
        matched_objs2 = tmp_matched2;
        return true;
    }
    
    // 如果没有找到任何匹配，返回false
    return false;
}

/**
 * @brief     查找一组对象的最佳匹配
 * @param     candidate_objs 候选对象列表
 * @param     target_size 目标大小
 * @param     matched_objs 匹配结果
 * @param     padding 允许的填充大小
 * @return    bool 是否找到匹配
 */
bool Disk::_find_s2m_match(const std::vector<int>& candidate_objs, 
                          int target_size, 
                          std::vector<int>& matched_objs, 
                          int& padding)
{
    // 准备候选对象的大小列表
    std::vector<std::pair<int, int>> size_obj_pairs; // <size, obj_id>
    for(auto obj_id : candidate_objs)
    {
        size_obj_pairs.push_back({controller->OBJECTS[obj_id].size, obj_id});
    }

    // 使用动态规划找到匹配的组合
    bool found = _dp_subset_sum(size_obj_pairs, target_size, matched_objs);

    if(found)
    {
        padding = 0;
        return true;
    }
    
    // 如果没有找到匹配的组合，且允许填充空闲块
    if(!found && padding > 0) {
        // 将空闲块抽象为大小为1的特殊对象
        std::vector<std::pair<int, int>> extended_pairs = size_obj_pairs;
        for(int i = 0; i < padding; i++) {
            extended_pairs.push_back({1, -1}); // 使用-1表示空闲块
        }
        
        // 再次尝试匹配
        found = _dp_subset_sum(extended_pairs, target_size, matched_objs);
        
        // 计算实际使用的空闲块数量
        if(found) {
            int used_padding = 0;
            for(auto obj_id : matched_objs) {
                if(obj_id == -1) used_padding++;
            }
            padding = used_padding;
        }
        // 删除空闲块
        matched_objs.erase(std::remove(matched_objs.begin(), matched_objs.end(), -1), matched_objs.end());
    }
    
    return found;
}

/**
 * @brief     动态规划求解子集和问题
 * @param     size_obj_pairs 对象大小和ID对列表
 * @param     target_size 目标大小
 * @param     matched_objs 匹配结果
 * @return    bool 是否找到解
 */
bool Disk::_dp_subset_sum(const std::vector<std::pair<int, int>>& size_obj_pairs, 
                         int target_size, 
                         std::vector<int>& matched_objs)
{
    int n = size_obj_pairs.size();
    
    // 如果没有候选对象，返回false
    if(n == 0) return false;
    
    // 创建dp表，dp[i][j]表示前i个对象是否可以组成大小j
    std::vector<std::vector<bool>> dp(n + 1, std::vector<bool>(target_size + 1, false));
    
    // 初始化：空集可以组成大小0
    for(int i = 0; i <= n; i++)
        dp[i][0] = true;
    
    // 动态规划填表
    for(int i = 1; i <= n; i++)
    {
        for(int j = 1; j <= target_size; j++)
        {
            // 不选第i个对象
            dp[i][j] = dp[i-1][j];
            
            // 如果当前对象大小不超过j，可以选择该对象
            if(size_obj_pairs[i-1].first <= j)
            {
                dp[i][j] = dp[i][j] || dp[i-1][j - size_obj_pairs[i-1].first];
            }
        }
    }
    
    // 如果无法组成目标大小，返回false
    if(!dp[n][target_size]) return false;
    
    // 回溯找出组成目标大小的对象
    int j = target_size;
    for(int i = n; i > 0 && j > 0; i--)
    {
        // 如果选择了第i个对象
        if(dp[i][j] && !dp[i-1][j])
        {
            matched_objs.push_back(size_obj_pairs[i-1].second);
            j -= size_obj_pairs[i-1].first;
        }
    }
    
    return true;
}

/**
 * @brief     执行一对多交换
 * @param     single_obj_idx 单个对象ID
 * @param     multi_obj_idxs 多个对象ID列表
 * @param     gc_pairs 记录交换操作的列表
 * @param     padding 填充大小
 * @param     target_part 目标分区
 */
void Disk::_swap_s2m(int single_obj_idx, 
                     const std::vector<int>& multi_obj_idxs, 
                     std::vector<std::pair<int, int>>& gc_pairs, 
                     int padding, 
                     Part* target_part)
{
    // 验证单个对象的大小等于多个对象大小之和
    int single_obj_size = controller->OBJECTS[single_obj_idx].size;
    int multi_obj_total_size = 0;
    for(auto obj_idx : multi_obj_idxs)
    {
        multi_obj_total_size += controller->OBJECTS[obj_idx].size;
    }
    assert(single_obj_size == multi_obj_total_size + padding);
    
    // 验证所有对象都在当前磁盘上
    assert(controller->OBJECTS[single_obj_idx].replicas[0].first == this->id);
    for(auto obj_idx : multi_obj_idxs)
    {
        assert(controller->OBJECTS[obj_idx].replicas[0].first == this->id);
    }
    
    // 获取单个对象的单元格索引
    std::vector<int> single_obj_cells = controller->OBJECTS[single_obj_idx].replicas[0].second;
    
    // 获取多个对象的所有单元格索引
    std::vector<int> multi_obj_cells;
    for(auto obj_idx : multi_obj_idxs)
    {
        const auto& cells = controller->OBJECTS[obj_idx].replicas[0].second;
        multi_obj_cells.insert(multi_obj_cells.end(), cells.begin(), cells.end());
    }

    // 查找最匹配的空闲块
    bool is_reverse = target_part->start > target_part->end;
    FreeBlock* best_block = target_part->_find_best_block(padding, is_reverse, false);
    if(best_block != nullptr) 
    {
        int cell_idx = is_reverse ? best_block->end : best_block->start;
        while(padding > 0) 
        {
            multi_obj_cells.push_back(cell_idx);
            cell_idx = is_reverse ? cell_idx - 1 : cell_idx + 1;
            padding--;
        }
    }
    // 如果找不到合适的空闲块，则遍历分配
    else
    {
        int cell_idx = target_part->start;
        while(padding > 0) 
        {
            if(cells[cell_idx].obj_id == 0) {
                multi_obj_cells.push_back(cell_idx);
                padding--;
            }
            cell_idx = is_reverse ? cell_idx - 1 : cell_idx + 1;
        }
    }

    // 确保单元格数量匹配
    assert(single_obj_cells.size() == multi_obj_cells.size());
    
    // 执行单元格交换
    for(size_t i = 0; i < single_obj_cells.size(); i++)
    {
        _swap_cell(single_obj_cells[i], multi_obj_cells[i]);
        gc_pairs.push_back({single_obj_cells[i], multi_obj_cells[i]});
    }
}

/**
 * @brief     执行多对多交换
 * @param     matched_objs1 第一组匹配对象
 * @param     matched_objs2 第二组匹配对象
 * @param     gc_pairs 记录交换操作的列表
 */
void Disk::_swap_m2m(const std::vector<int>& matched_objs1, 
                     const std::vector<int>& matched_objs2, 
                     std::vector<std::pair<int, int>>& gc_pairs)
{
    // 验证所有对象都在当前磁盘上
    assert(controller->OBJECTS[matched_objs1[0]].replicas[0].first == this->id);
    assert(controller->OBJECTS[matched_objs2[0]].replicas[0].first == this->id);
    // 获取所有对象的单元格索引
    std::vector<int> matched_objs1_cells;
    std::vector<int> matched_objs2_cells;
    for(auto obj_idx : matched_objs1)
    {
        const auto& cells = controller->OBJECTS[obj_idx].replicas[0].second;
        matched_objs1_cells.insert(matched_objs1_cells.end(), cells.begin(), cells.end());
    }
    for(auto obj_idx : matched_objs2)
    {
        const auto& cells = controller->OBJECTS[obj_idx].replicas[0].second;
        matched_objs2_cells.insert(matched_objs2_cells.end(), cells.begin(), cells.end());
    }
    // 执行单元格交换
    for(size_t i = 0; i < matched_objs1_cells.size(); i++)
    {
        int cell_idx1 = matched_objs1_cells[i];
        int cell_idx2 = matched_objs2_cells[i];
        _swap_cell(cell_idx1, cell_idx2);
        gc_pairs.push_back({cell_idx1, cell_idx2});
        
    }
}
/*╚═════════════════════════════════════════════════════════════════════════╝*/