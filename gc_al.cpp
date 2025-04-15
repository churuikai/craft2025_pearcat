#include "disk_obj_req.h"
#include "controller.h"
#include "debug.h"
#include "data_analysis.h"
#include "io.h"
#include <algorithm>

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

// 多对多匹配

// 多对多匹配函数，尽可能找到最大的匹配
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


// 查找一组对象，使其大小之和等于目标大小, padding 表示允许插入的最大空闲块数量
bool Disk::_find_s2m_match(const std::vector<int>& candidate_objs, int target_size, std::vector<int>& matched_objs, int& padding)
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

// 动态规划求解子集和问题
bool Disk::_dp_subset_sum(const std::vector<std::pair<int, int>>& size_obj_pairs, int target_size, std::vector<int>& matched_objs)
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

// 执行一对多交换
void Disk::_swap_s2m(int single_obj_idx, const std::vector<int>& multi_obj_idxs, std::vector<std::pair<int, int>>& gc_pairs, int padding, Part* target_part)
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

// 执行多对多交换
void Disk::_swap_m2m(const std::vector<int>& matched_objs1, const std::vector<int>& matched_objs2, std::vector<std::pair<int, int>>& gc_pairs)
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



