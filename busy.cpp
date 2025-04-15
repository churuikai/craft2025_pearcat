#include "constants.h"
#include "disk_obj_req.h"
#include "debug.h"
#include "data_analysis.h"
#include "token_table.h"
// 添加请求
void Controller::add_req(int req_id, int obj_id)
{
    // 更新请求
    REQS[req_id % LEN_REQ].init(req_id, OBJECTS[obj_id], timestamp);
    // 更新对象
    OBJECTS[obj_id].req_ids.insert(req_id);

    // 更新活跃请求范围
    assert(req_id > req_new_idx);
    req_new_idx = req_id;


    // 更新磁盘, 只更新热数据区
    auto &[disk_id, cells_idx] = OBJECTS[obj_id].replicas[0];
    for (int cell_idx : cells_idx)
    {
        DISKS[disk_id].cells[cell_idx].req_ids.insert(req_id);
    }
}

void Controller::remove_req(int req_id)
{
    // 更新对象
    int obj_id = REQS[req_id % LEN_REQ].obj_id;
    OBJECTS[obj_id].req_ids.erase(req_id);
    // 更新请求
    REQS[req_id % LEN_REQ].clear();
    // 更新磁盘
    auto &[disk_id, cells_idx] = OBJECTS[obj_id].replicas[0];
    for (int cell_idx : cells_idx)
    {
        DISKS[disk_id].cells[cell_idx].req_ids.erase(req_id);
    }
}


void Controller::pre_filter_req(std::vector<std::pair<int, int>> &reqs)
{

    // 标签对应的读取频率次序
    std::vector<int> tag_order(M+1);

    // 请求是否有邻居
    std::unordered_map<int, bool> req_is_alone;

    // 计算标签对应的读取频率次序
    auto &order_tag = get_sorted_read_tag(this->timestamp);
    for(int i = 0; i < M; ++i)
    {
        tag_order[order_tag[i]] = i;
    }

    // 计算请求附近6格是否有邻居
    for(auto [req_id, obj_id] : reqs)
    {
        int size = OBJECTS[obj_id].size;
        int tag = OBJECTS[obj_id].tag;
        Disk& disk = DISKS[OBJECTS[obj_id].replicas[0].first];
        std::vector<int>& cell_idxs = OBJECTS[obj_id].replicas[0].second;

        req_is_alone[req_id] = true;
        for (auto &cell_idx : cell_idxs)
        {
            int prev_cell_idx = cell_idx;
            int next_cell_idx = cell_idx;
            for (int k = 0; k < 7; ++k)
            {
                if(disk.cells[prev_cell_idx].req_ids.size() > 0 or disk.cells[next_cell_idx].req_ids.size() > 0)
                {
                    req_is_alone[req_id] = false; break;
                }
                prev_cell_idx = prev_cell_idx == 1 ? 1 : prev_cell_idx - 1;
                next_cell_idx = next_cell_idx == size ? size : next_cell_idx + 1;
            }
            if(not req_is_alone[req_id]) break;
        }
    }

    // 给reqs排序, 有邻居在后，其次按请求频率排序，频率低在前
    // std::sort(reqs.begin(), reqs.end(), [&](const std::pair<int, int>& a, const std::pair<int, int>& b) {
    //     bool a_alone = req_is_alone[a.first];
    //     bool b_alone = req_is_alone[b.first];
    //     if (a_alone != b_alone) return a_alone;
    //     return tag_order[OBJECTS[a.second].tag] < tag_order[OBJECTS[b.second].tag];
    // });

    std::vector<std::pair<int, int>> new_reqs;
    // int end_idx = int(reqs.size() * drop_rate);
    for(int i = 0; i < reqs.size(); ++i)
    {
        Disk& disk = DISKS[OBJECTS[reqs[i].second].replicas[0].first];
        float avg_wait_time = disk.get_avg_wait_time();

        int m = 12;
        if(avg_wait_time > 43) m -= 1;
        else if(avg_wait_time > 29) m-=2;
        else if(avg_wait_time > 18) m-=3;
        else if(avg_wait_time > 16) m-=4;
        else if(avg_wait_time > 15) m-=5;
        else if(avg_wait_time > 14) m-=6;
        else if(avg_wait_time > 13) m-=7;
        else if(avg_wait_time > 11) m-=7;
        else if(avg_wait_time > 10) m-=8;
        else m = 0;

        if(req_is_alone[reqs[i].first] and tag_order[OBJECTS[reqs[i].second].tag] < m)
        {
            over_load_reqs.push_back(reqs[i].first);
        }
        else
        {
            new_reqs.push_back(reqs[i]);
        }
    }
    reqs = new_reqs;
}



void Controller::post_filter_req()
{
    // 过滤即将超时的请求
    while(req_105_idx < req_new_idx)
    {
        int birth_time = REQS[req_105_idx % LEN_REQ].timestamp;
        if(birth_time == 0) 
        {
            req_105_idx++; 
            continue;
        }
        else if (birth_time + 104 < timestamp)
        {
            busy_reqs.push_back(req_105_idx);
            remove_req(req_105_idx);
            req_105_idx++;
        }
        else break;
    }
}
