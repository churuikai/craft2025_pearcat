#include "constants.h"
#include "io.h"
#include "controller.h"
#include "disk_obj_req.h"

#include "debug.h"
#include "token_table.h"
#include "data_analysis.h"
#include <cmath>


// 处理请求
std::pair<std::vector<std::string>, std::vector<int>> Controller::read()
{
    std::vector<std::string> ops;
    std::vector<int> completed_reqs;
    for (int disk_id = 1; disk_id <= N; ++disk_id)
    {
        // 磁头1
        auto [op1, completed_req1] = DISKS[disk_id].read(1);
        ops.push_back(op1);
        completed_reqs.insert(completed_reqs.end(), completed_req1.begin(), completed_req1.end());
        // 磁头2
        auto [op2, completed_req2] = DISKS[disk_id].read(2);
        ops.push_back(op2);
        completed_reqs.insert(completed_reqs.end(), completed_req2.begin(), completed_req2.end());
    }

    return {ops, completed_reqs};
}

std::pair<std::string, std::vector<int>> Disk::read(int op_id)
{
    int &point = op_id == 1 ? point1 : point2;
    int &tokens = op_id == 1 ? tokens1 : tokens2;
    int &prev_read_token = op_id == 1 ? prev_read_token1 : prev_read_token2;

    int start = _get_best_start(op_id);

    if (start == -1)
    {
        return {"#", std::vector<int>()};
    }
    // 如果最佳起点读取代价大于剩余令牌数，则J
    if ((start - point + size) % size > tokens)
    {
        point = start;
        prev_read_token = 80;
        return {"j " + std::to_string(start), std::vector<int>()};
    }
    // 如果最佳起点读取代价小于剩余令牌数，则读取
    else
    {
        auto [path, completed_reqs, _] = _read_by_best_path(start, op_id);
        return {path + "#", completed_reqs};
    }
}

int Disk::_get_best_start(int op_id)
{
    int &point = op_id == 1 ? point1 : point2;
    int &tokens = op_id == 1 ? tokens1 : tokens2;
    int &prev_read_token = op_id == 1 ? prev_read_token1 : prev_read_token2;
    int data_size = op_id == 1 ? data_size1 : data_size2;
    int part_start = op_id == 1 ? 1 : data_size1 + 1;
    int part_end = op_id == 1 ? data_size1 : data_size1 + data_size2;

    // 找到离point最近的
    int start = point;

    for (int i = 0; i < data_size; ++i)
    {
        start = start <= part_end ? start : part_start;
        if (!cells[start].req_ids.empty())
        {
            return start;
        }
        // start = start % size + 1;
        start = start + 1;
    }

    return -1;
}

std::tuple<std::string, std::vector<int>, std::vector<int>> Disk::_read_by_best_path(int start, int op_id)
{
    // 通过free-read状态获取读取路径, 维护point、prev_read_token、tokens
    std::string path = "";
    std::vector<int> completed_reqs;
    std::vector<int> occupied_obj;

    int &point = op_id == 1 ? point1 : point2;
    int &tokens = op_id == 1 ? tokens1 : tokens2;
    int &prev_read_token = op_id == 1 ? prev_read_token1 : prev_read_token2;
    int data_size = op_id == 1 ? data_size1 : data_size2;
    int part_start = op_id == 1 ? 1 : data_size1 + 1;
    int part_end = op_id == 1 ? data_size1 : data_size1 + data_size2;

    int win_end = point;
    auto fo_seq = EMPTY_SEQUENCE;
    int point_start = point;

    // 定义临时函数判断是否读取
    auto is_read = [&](int cell_idx) -> bool
    {
        return not cells[cell_idx].req_ids.empty() and cell_idx >= part_start and cell_idx <= part_end;
    };

    bool arrive = false;
    for (int i = 0; i < 13; ++i)
    {
        // 更新滑动窗口序列
        if (win_end == start)
            arrive = true;
        update_sequence(fo_seq, is_read(win_end));
        win_end = win_end % size + 1;
    }
    while (true)
    {
        auto decision = get_decision(prev_read_token, fo_seq);
        if (tokens - decision.cost < 0)
        {
            break;
        }
        if (decision.is_r)
        {
            path.append(1, 'r');
            _read_cell(point, completed_reqs);
        }
        else
        {
            path.append(1, 'p');
        }
        tokens -= decision.cost;
        prev_read_token = decision.next_token;
        point = point % size + 1;
        if (win_end == start)
            arrive = true;
        update_sequence(fo_seq, is_read(win_end));
        win_end = win_end % size + 1;
    }
    return {path, completed_reqs, occupied_obj};
}

void Disk::_read_cell(int cell_idx, std::vector<int>& completed_reqs)
{
    // 空读
    if(cells[cell_idx].req_ids.empty() or cells[cell_idx].obj_id == 0)
    {
        return;
    }
    
    // 检查req是否完成 更新完成的 req
    for (int req_id : cells[cell_idx].req_ids)
    {
        // 更新请求
        controller->REQS[req_id % LEN_REQ].remain_units.erase(cells[cell_idx].unit_id);
        // 如果请求完成
        if (controller->REQS[req_id % LEN_REQ].remain_units.empty())
        {
            completed_reqs.push_back(req_id);
            // 更新等待时间统计
            _update_wait_time_stats(req_id, controller->timestamp);
            // 更新对象
            controller->OBJECTS[cells[cell_idx].obj_id].req_ids.erase(req_id);
            // 清除请求
            controller->REQS[req_id % LEN_REQ].clear();
        }
    }
    // 更新cell
 
    assert(cells[cell_idx].obj_id != 0);
    cells[cell_idx].req_ids.clear();
}


// 更新请求等待时间统计
void Disk::_update_wait_time_stats(int req_id, int current_timestamp)
{
    // 获取请求创建时间
    int req_timestamp = controller->REQS[req_id % LEN_REQ].timestamp;
    
    // 计算等待时间
    int wait_time = current_timestamp - req_timestamp;
    
    // 先加入新的等待时间到总和
    total_wait_time += wait_time;
    recent_wait_times.push_back(wait_time);
    
    // 如果超过最大数量限制，移除最旧的记录并从总和中减去
    if (recent_wait_times.size() > MAX_RECENT_REQS) 
    {
        total_wait_time -= recent_wait_times.front();
        recent_wait_times.pop_front();
    }
    
}