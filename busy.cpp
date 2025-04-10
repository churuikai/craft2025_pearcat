#include "constants.h"
#include "disk_obj_req.h"
#include "debug.h"
#include "data_analysis.h"

// 添加请求
void Controller::add_req(int req_id, int obj_id)
{
    // 前置过滤请求
    if (not _pre_filter_req(req_id, obj_id))
    {
        // 更新请求
        REQS[req_id % LEN_REQ].update(req_id, OBJECTS[obj_id], timestamp);
        // 更新对象
        OBJECTS[obj_id].req_ids.insert(req_id);
        // 更新activate_reqs
        activate_reqs.insert(req_id);
        // 更新磁盘, 只更新热数据区
        for (int i = 0; i < 3 - BACK_NUM; ++i)
        {
            auto [disk_id, cells_idx] = OBJECTS[obj_id].replicas[i];
            DISKS[disk_id].req_pos[req_id] = Int16Array();
            for (int cell_idx : cells_idx)
            {
                DISKS[disk_id].cells[cell_idx]->req_ids.insert(req_id);
                DISKS[disk_id].req_cells_num++;
                DISKS[disk_id].req_pos[req_id].add(cell_idx);
            }
        }
    }
    else
    {
        // 上报丢弃
        over_load_reqs.push_back(req_id);
    }

}

void Controller::remove_req(int req_id)
{
    // 更新对象
    int obj_id = REQS[req_id % LEN_REQ].obj_id;
    OBJECTS[obj_id].req_ids.erase(req_id);
    // 更新请求
    REQS[req_id % LEN_REQ].remove();
    // 更新activate_reqs
    activate_reqs.erase(req_id);
    // 更新磁盘
    for (int i = 0; i < 3 - BACK_NUM; ++i)
    {
        auto [disk_id, cells_idx] = OBJECTS[obj_id].replicas[i];
        for (int cell_idx : cells_idx)
        {
            assert(DISKS[disk_id].cells[cell_idx]->req_ids.find(req_id) != DISKS[disk_id].cells[cell_idx]->req_ids.end());
            DISKS[disk_id].cells[cell_idx]->req_ids.erase(req_id);
            DISKS[disk_id].req_cells_num--;
        }
        DISKS[disk_id].req_pos.erase(req_id);
    }

}


bool Controller::_pre_filter_req(int req_id, int obj_id)
{
    // 判断是否超载
    bool is_over_load = false;
    int tag = OBJECTS[obj_id].tag;
    int size = OBJECTS[obj_id].size;
    // 获取标签读频率顺序（由小到大排序）
    auto &order_tag = get_sorted_read_tag(this->timestamp);
    // 更新磁盘
    for (int i = 0; i < 3 - BACK_NUM; ++i)
    {
        auto [disk_id, cells_idx] = OBJECTS[obj_id].replicas[i];
        // 计算当前负载能力 比 最低负载能力 的倍数
        float n = (1.0 * G / DISKS[disk_id].req_pos.size() / V) / LOAD_COEFFICIENT;
        // 超过8倍 不考虑舍弃
        if (n > 12) break;
        // 计算需要舍弃的请求数量
        // int m = M - 8 - (int)n;
        int m = M - 6.5 - (int)n;
        if (m < 0) m = 0;
        // 从低频到高频依次判断是否舍弃
        for (int j = 0; j < m; ++j)
        {

            bool is_alone = true;
            // 获取obj邻居
            for(auto& [disk_id, cell_idxs] : this->OBJECTS[obj_id].replicas)
            {

                for(auto& cell_idx : cell_idxs)
                {
                    if (not is_alone) break;
                    int prev_cell_idx = cell_idx;
                    int next_cell_idx = cell_idx;
                    for(int k = 0; k < 6; ++k)
                    {
                        prev_cell_idx = prev_cell_idx == 1 ? prev_cell_idx : prev_cell_idx - 1;
                        next_cell_idx = next_cell_idx == size ? next_cell_idx : next_cell_idx + 1;
                        if(this->DISKS[disk_id].cells[prev_cell_idx]->req_ids.size() > 0 or this->DISKS[disk_id].cells[next_cell_idx]->req_ids.size() > 0)
                        {
                            is_alone = false;
                            break;
                        }
                    }

                }
                break;
            }
            
            if (tag == order_tag[j] and this->OBJECTS[obj_id].req_ids.size() == 0 and is_alone)
            {
                is_over_load = true;
                break;
            }
        }
    }
    return is_over_load;
}


void Controller::post_filter_req()
{
    std::vector<int> drop_req_ids = _post_filter_req();
    for (int req_id : drop_req_ids)
    {
        // 删除请求
        remove_req(req_id);
        // 上报丢弃
        over_load_reqs.push_back(req_id);
    }
}

std::vector<int> Controller::_post_filter_req()
{
    std::vector<int> drop_req_ids;
    for(int disk_id = 0; disk_id <= N; ++disk_id)
    {
        DISKS[disk_id].filter_req(drop_req_ids);
    }
    return drop_req_ids;
}


void Disk::filter_req(std::vector<int>& drop_req_ids)
{

}