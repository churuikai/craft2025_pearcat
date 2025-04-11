#include "constants.h"
#include "disk_obj_req.h"
#include "debug.h"
#include "data_analysis.h"
#include "token_table.h"
// 添加请求
void Controller::add_req(int req_id, int obj_id)
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
        auto& [disk_id, cells_idx] = OBJECTS[obj_id].replicas[i];
        DISKS[disk_id].req_pos[req_id] = Int16Array();
        for (int cell_idx : cells_idx)
        {
            DISKS[disk_id].cells[cell_idx]->req_ids.insert(req_id);
            // assert(DISKS[disk_id].cells[cell_idx]->req_ids.find(req_id) != DISKS[disk_id].cells[cell_idx]->req_ids.end());
            DISKS[disk_id].req_cells_num++;
            DISKS[disk_id].req_pos[req_id].add(cell_idx);
        }
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

    auto &[disk_id, cells_idx] = OBJECTS[obj_id].replicas[0];
    for (int cell_idx : cells_idx)
    {
        assert(DISKS[disk_id].cells[cell_idx]->req_ids.find(req_id) != DISKS[disk_id].cells[cell_idx]->req_ids.end());
        DISKS[disk_id].cells[cell_idx]->req_ids.erase(req_id);
        DISKS[disk_id].req_cells_num--;
    }
    DISKS[disk_id].req_pos.erase(req_id);


}


void Controller::pre_filter_req(std::vector<std::pair<int, int>> &reqs)
{
    // 获取标签读频率顺序（由小到大排序）
    auto &order_tag = get_sorted_read_tag(this->timestamp);

    // 标签对应的次序
    std::vector<int> tag_order(order_tag.size()+1);


    // 每个请求是否有邻居
    std::unordered_map<int, bool> req_is_alone;

    for(int i = 0; i < order_tag.size(); ++i)
    {
        tag_order[order_tag[i]] = i;
    }

    for(auto [req_id, obj_id] : reqs)
    {
        int size = OBJECTS[obj_id].size;
        int tag = OBJECTS[obj_id].tag;
        Disk& disk = DISKS[OBJECTS[obj_id].replicas[0].first];
        std::vector<int>& cell_idxs = OBJECTS[obj_id].replicas[0].second;
        // 获取请求的邻居

        bool is_alone = true;
        // 获取obj邻居

        for (auto &cell_idx : cell_idxs)
        {
            if (not is_alone)
                break;
            int prev_cell_idx = cell_idx;
            int next_cell_idx = cell_idx;
            for (int k = 0; k < 6; ++k)
            {
                prev_cell_idx = prev_cell_idx == 1 ? prev_cell_idx : prev_cell_idx - 1;
                next_cell_idx = next_cell_idx == size ? next_cell_idx : next_cell_idx + 1;
                if (disk.cells[prev_cell_idx]->req_ids.size() > 0 or disk.cells[next_cell_idx]->req_ids.size() > 0)
                {
                    is_alone = false;
                    break;
                }
            }
        }
        req_is_alone[req_id] = is_alone and this->OBJECTS[obj_id].req_ids.size() == 0;
    }

    // 给reqs排序, 有邻居在后，其次按请求频率排序，频率低在前
    std::sort(reqs.begin(), reqs.end(), [&](const std::pair<int, int>& a, const std::pair<int, int>& b) {
        bool a_alone = req_is_alone[a.first];
        bool b_alone = req_is_alone[b.first];
        if (a_alone != b_alone) return a_alone;
        return tag_order[OBJECTS[a.second].tag] < tag_order[OBJECTS[b.second].tag];
    });


    std::vector<std::pair<int, int>> new_reqs;


    float avg_wait_time = 0;
    for(int disk_id = 1; disk_id <= N; ++disk_id)
    {
        avg_wait_time += DISKS[disk_id].get_avg_wait_time();
    }
    avg_wait_time /= N;

    // float drop_rate;
    // if(avg_wait_time > 39) drop_rate = 0.35;
    // else if(avg_wait_time > 35) drop_rate = 0.27;
    // else if(avg_wait_time > 30) drop_rate = 0.26;
    // else if(avg_wait_time > 25) drop_rate = 0.25;
    // else if(avg_wait_time > 23) drop_rate = 0.24;
    // else if(avg_wait_time > 18) drop_rate = 0.22;
    // else if(avg_wait_time > 16) drop_rate = 0.2;
    // else if(avg_wait_time > 15) drop_rate = 0.18;
    // else if(avg_wait_time > 14) drop_rate = 0.16;
    // else if(avg_wait_time > 13) drop_rate = 0.14;
    // else if(avg_wait_time > 11) drop_rate =0.12;
    // else if(avg_wait_time > 10) drop_rate =0.1;
    // else drop_rate = 0;

    float drop_rate;
    if(avg_wait_time > 35) drop_rate = 0.3;
    else if(avg_wait_time > 33) drop_rate = 0.26;
    else if(avg_wait_time > 30) drop_rate = 0.25;
    else if(avg_wait_time > 25) drop_rate = 0.24;
    else if(avg_wait_time > 23) drop_rate = 0.23;
    else if(avg_wait_time > 18) drop_rate = 0.22;
    else if(avg_wait_time > 16) drop_rate = 0.2;
    else if(avg_wait_time > 15) drop_rate = 0.18;
    else if(avg_wait_time > 14) drop_rate = 0.16;
    else if(avg_wait_time > 13) drop_rate = 0.14;
    else if(avg_wait_time > 11) drop_rate =0.12;
    else if(avg_wait_time > 10) drop_rate =0.1;
    else drop_rate = 0;


    int m = 10;
    if(avg_wait_time > 39) m -= 1;
    else if(avg_wait_time > 23) m-=2;
    else if(avg_wait_time > 18) m-=3;
    else if(avg_wait_time > 16) m-=4;
    else if(avg_wait_time > 15) m-=5;
    else if(avg_wait_time > 14) m-=6;
    else if(avg_wait_time > 13) m-=7;
    else if(avg_wait_time > 11) m-=7;
    else if(avg_wait_time > 10) m-=8;
    else m = 0;

    int end_idx = int(reqs.size() * drop_rate);
    for(int i = 0; i < reqs.size(); ++i)
    {
        // 如果alone，则over
        if((req_is_alone[reqs[i].first]) and i<end_idx and tag_order[OBJECTS[reqs[i].second].tag] < m)
        {
            over_load_reqs.push_back(reqs[i].first);
        }
        else
        {
            new_reqs.push_back(reqs[i]);
        }
    }
    reqs = new_reqs;

    return;

    // obj_id 重复的请求
    std::unordered_map<int, int> obj_req_num;
    for(auto [req_id, obj_id] : reqs)
    {
        obj_req_num[obj_id]++;
    }

    for(auto [req_id, obj_id] : reqs)
    {
        // 判断是否超载
        bool is_over_load = false;
        int tag = OBJECTS[obj_id].tag;
        int size = OBJECTS[obj_id].size;
        Disk& disk = DISKS[OBJECTS[obj_id].replicas[0].first];
        std::vector<int>& cell_idxs = OBJECTS[obj_id].replicas[0].second;

        float avg_wait_time = disk.get_avg_wait_time();

        int m = 10;
        if(avg_wait_time > 39) m -= 1;
        else if(avg_wait_time > 23) m-=2;
        else if(avg_wait_time > 18) m-=3;
        else if(avg_wait_time > 16) m-=4;
        else if(avg_wait_time > 15) m-=5;
        else if(avg_wait_time > 14) m-=6;
        else if(avg_wait_time > 13) m-=7;
        else if(avg_wait_time > 11) m-=7;
        else if(avg_wait_time > 10) m-=8;
        else m = 0;
        // 从低频到高频依次判断是否舍弃
        for (int j = 0; j < m; ++j)
        {
            if(tag != order_tag[j])continue;
            bool is_alone = true;
            // 获取obj邻居

            for (auto &cell_idx : cell_idxs)
            {
                if (not is_alone)
                    break;
                int prev_cell_idx = cell_idx;
                int next_cell_idx = cell_idx;
                for (int k = 0; k < 6; ++k)
                {
                    prev_cell_idx = prev_cell_idx == 1 ? prev_cell_idx : prev_cell_idx - 1;
                    next_cell_idx = next_cell_idx == size ? next_cell_idx : next_cell_idx + 1;
                    if (disk.cells[prev_cell_idx]->req_ids.size() > 0 or disk.cells[next_cell_idx]->req_ids.size() > 0)
                    {
                        is_alone = false;
                        break;
                    }
                }
            }

            if (this->OBJECTS[obj_id].req_ids.size() == 0 and is_alone)
            {
                is_over_load = true;
                break;
            }
        }
    
        if(is_over_load)
        {
            over_load_reqs.push_back(req_id);
        }
        else
        {
            new_reqs.push_back({req_id, obj_id});
        }
    }

    reqs = new_reqs;
    return;
    // 计算影响
    new_reqs.clear();

    // obj_id 重复的请求
    // std::unordered_map<int, int> obj_req_num;
    // for(auto [req_id, obj_id] : reqs)
    // {
    //     obj_req_num[obj_id]++;
    // }


    // 计算影响
    for(auto [req_id, obj_id] : reqs)
    {
        if(obj_req_num[obj_id] > 2)
        {
            new_reqs.push_back({req_id, obj_id});
            continue;
        }
        Disk& disk = DISKS[OBJECTS[obj_id].replicas[0].first];
        float influence = disk.calcu_win_influence(req_id, obj_id, true);
        // debug("req_id: ", req_id, "obj_id: ", obj_id, "influence: ", influence);
        if(influence < -200)
        {

            over_load_reqs.push_back(req_id);
        }
        else
        {
            new_reqs.push_back({req_id, obj_id});
        }
    }

    reqs = new_reqs;
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
    for(int disk_id = 1; disk_id <= N; ++disk_id)
    {
        DISKS[disk_id].filter_req(drop_req_ids);
    }
    return drop_req_ids;
}


void Disk::filter_req(std::vector<int>& drop_req_ids)
{
    return;
    if(controller->timestamp % 5 != 0) return;

    for(auto& [req_id, cell_idxs] : req_pos)
    {
        // 判断是否在cell_idxs, 不在则说明读了一半
        bool is_half_read = false;
        for(int cell_idx : cell_idxs)
        {
            if(cells[cell_idx]->req_ids.find(req_id) == cells[cell_idx]->req_ids.end())
            {
                is_half_read = true;
                break;
            }
        }
        if(is_half_read) continue;

        int alive_time = controller->timestamp - controller->REQS[req_id % LEN_REQ].timestamp;
        if(alive_time <= 5)
        {
            int obj_id = controller->REQS[req_id % LEN_REQ].obj_id;
            // 计算影响力
            float influence = calcu_win_influence(req_id, obj_id, false);
            if(influence < -20)
            {
                drop_req_ids.push_back(req_id);
            }
        }
    }

}

float Disk::calcu_win_influence(int req_id, int obj_id, bool is_pre)
{
    int win_before = 26;
    int win_after = 13;
    // int obj_id = controller->REQS[req_id % LEN_REQ].obj_id;
    std::vector<int>& cell_idxs = controller->OBJECTS[obj_id].replicas[0].second;
    // debug("req_id: ", req_id, "obj_id: ", obj_id, "cell_idxs: ", cell_idxs);
    assert(!cell_idxs.empty());
    int part_start;
    int part_end;
    if (cell_idxs[0]>=1 and cell_idxs[0]<=data_size1)
    {
        part_start = 1;
        part_end = data_size1;
    }
    else
    {
        part_start = data_size1 + 1;
        part_end = data_size1 + data_size2; 
    }
    int min_id = *min_element(cell_idxs.begin(), cell_idxs.end());
    int max_id = *max_element(cell_idxs.begin(), cell_idxs.end());

    if (cell_idxs[0]>=1 and cell_idxs[0]<=data_size1)
    {
        int point = point1;
        int dist = (min_id - point + size) % size;
        if(dist > 500) return 1;

    }
    else
    {
        int point = point2;
        int dist = (min_id - point + size) % size;
        if(dist > 500) return 1;
    }

    // 窗口起始点
    int start = std::max(part_start, min_id - win_before);
    int end = std::min(part_end, max_id + win_after);

    int norm_read_token = 80;
    int assume_read_token = 80;
    // 没有该请求
    auto norm_seq = EMPTY_SEQUENCE;
    int norm_token = 0;
    // 有该请求
    auto assume_seq = EMPTY_SEQUENCE;
    int assume_token = 0;

    int head_point = start;
    int check_point = start;

    for (int i = 0; i < 13; ++i)
    {
        if (find(cell_idxs.begin(), cell_idxs.end(), check_point) != cell_idxs.end()) // 在cell_idxs中
        {
            if(is_pre)
            {
                update_sequence(norm_seq, cells[check_point]->req_ids.size()>0); // 没有该请求
                update_sequence(assume_seq, true);
            }
            else
            {
                update_sequence(norm_seq, cells[check_point]->req_ids.size()>1); // 没有该请求
                update_sequence(assume_seq, true);
            }
        }
        else 
        {
            update_sequence(norm_seq, !cells[check_point]->req_ids.empty());
            update_sequence(assume_seq, !cells[check_point]->req_ids.empty());
        }
        check_point++;
    }
    auto norm_result_tmp = get_decision(norm_read_token,norm_seq);
    auto assume_result_tmp = get_decision(assume_read_token, assume_seq);

    norm_token += norm_result_tmp.cost;
    assume_token += assume_result_tmp.cost;

    norm_read_token = norm_result_tmp.next_token;
    assume_read_token = assume_result_tmp.next_token;

    head_point++;

    while (head_point <= end)
    {
        if (find(cell_idxs.begin(), cell_idxs.end(), check_point) != cell_idxs.end()) // 在cell_idxs中
        {
            if(is_pre)
            {
                update_sequence(norm_seq, cells[check_point]->req_ids.size()>1);
                update_sequence(assume_seq, true);
            }
            else
            {
                update_sequence(norm_seq, cells[check_point]->req_ids.size()>0);
                update_sequence(assume_seq, true);
            }
        }
        else if (check_point > part_end)    // 超出边界一定会跳，相当于后续均为p
        {
            update_sequence(norm_seq, false);
            update_sequence(assume_seq, false);
        }
        else 
        {
            update_sequence(norm_seq, !cells[check_point]->req_ids.empty());
            update_sequence(assume_seq, !cells[check_point]->req_ids.empty());
        }
        check_point++;
        norm_result_tmp = get_decision(norm_read_token,norm_seq);
        assume_result_tmp = get_decision(assume_read_token, assume_seq);

        norm_token += norm_result_tmp.cost;
        assume_token += assume_result_tmp.cost;
        norm_read_token = norm_result_tmp.next_token;
        assume_read_token = assume_result_tmp.next_token;
        head_point++;
    }

    int gain = cell_idxs.size();

    float delat_t = (assume_token - norm_token) / G_float;
    float influence = gain - delat_t*req_cells_num/2;
    return influence;
}
