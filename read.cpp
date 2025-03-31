#include "disk_obj_req.h"
#include "constants.h"
#include "debug.h"
#include "token_table.h"
void process_read(Controller &controller)
{
    int n_read;
    scanf("%d", &n_read);

    // 添加请求
    for (int i = 0; i < n_read; ++i)
    {
        int req_id, obj_id;
        scanf("%d %d", &req_id, &obj_id);
        controller.add_req(req_id, obj_id);
    }

    // 处理所有请求
    auto [disk_operations, completed_requests] = controller.read();

    // 输出磁头操作
    for (const auto &op : disk_operations)
    {
        printf("%s\n", op.c_str());
    }

    // 输出完成的请求
    printf("%d\n", completed_requests.size());
    for (int req_id : completed_requests)
    {
        printf("%d\n", req_id);
    }
    fflush(stdout);
}

// 添加请求
void Controller::add_req(int req_id, int obj_id)
{
    // 更新请求
    REQS[req_id % LEN_REQ].update(req_id, obj_id, timestamp);
    // 更新对象
    OBJECTS[obj_id].req_ids.insert(req_id);
    // 更新activate_reqs
    activate_reqs.insert(req_id);
    // 更新磁盘
    for (int i = 0; i < 3 - BACK_NUM; ++i)
    {
        auto [disk_id, cells_idx] = OBJECTS[obj_id].replicas[i];
        DISKS[disk_id].add_req(req_id, cells_idx);
    }
}
// 处理请求
std::pair<std::vector<std::string>, std::vector<int>> Controller::read()
{
    std::vector<std::string> ops;
    std::vector<int> completed_reqs;

    for (int disk_id = 1; disk_id <= N; ++disk_id)
    {
        auto [op, completed_req] = DISKS[disk_id].read(timestamp);
        ops.push_back(op);
        completed_reqs.insert(completed_reqs.end(), completed_req.begin(), completed_req.end());
    }
    return {ops, completed_reqs};
}

void Disk::add_req(int req_id, const std::vector<int> &cells_idx)
{
    for (int cell_idx : cells_idx)
    {
        cells[cell_idx]->req_ids.insert(req_id);
        req_cells_num++;
        req_pos[req_id].add(cell_idx);
    }
}

std::pair<std::string, std::vector<int>> Disk::read(int timestamp)
{

    int start = _get_best_start(timestamp);
    // if(TIME==2)
    // debug(start);
    if (start == -1)
    {
        return {"#", std::vector<int>()};
    }
    // 如果最佳起点读取代价大于剩余令牌数，则J
    if ((start - point + size) % size > tokens)
    {
        if ((start < point or (point == 1 && start > point)) && id == 1)
        {
            debug(TIME, point);
        }

        point = start;
        prev_read_token = 80;
        return {"j " + std::to_string(start), std::vector<int>()};
    }
    // 如果最佳起点读取代价小于剩余令牌数，则读取
    else
    {
        auto [path, completed_reqs, _] = _read_by_best_path(start);
        return {path + "#", completed_reqs};
    }
}

std::tuple<std::string, std::vector<int>, std::vector<int>> Disk::_read_by_best_path(int start)
{
    // 通过free-read状态获取读取路径, 维护point、prev_read_token、tokens
    std::string path = "";
    std::vector<int> completed_reqs;
    std::vector<int> occupied_obj;

    int win_end = point;
    auto fo_seq = EMPTY_SEQUENCE;
    for (int i = 0; i < 13; ++i)
    {
        // 更新滑动窗口序列
        update_sequence(fo_seq, !cells[win_end]->req_ids.empty());
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
            auto completed_reqs_cell = cells[point]->read();
            completed_reqs.insert(completed_reqs.end(), completed_reqs_cell.begin(), completed_reqs_cell.end());
        }
        else
        {
            path.append(1, 'p');
        }
        tokens -= decision.cost;
        prev_read_token = decision.next_token;
        if (point == 1 && id == 1)
        {
            debug(TIME, point);
        }
        point = point % size + 1;
        update_sequence(fo_seq, !cells[win_end]->req_ids.empty());
        win_end = win_end % size + 1;
    }
    return {path, completed_reqs, occupied_obj};
}

void Req::update(int req_id, int obj_id, int timestamp)
{
    // this->id = req_id;
    this->obj_id = obj_id;
    remain_units.clear();
    for (int i = 1; i <= OBJECTS[obj_id].size; ++i)
    {
        remain_units.insert(i);
    }
    this->timestamp = timestamp;
}

// Cell的read方法实现（依赖其他类）
std::vector<int> Cell::read()
{

    std::vector<int> completed_reqs;
    // 检查req是否完成 更新完成的 req
    for (int req_id : req_ids)
    {
        REQS[req_id % LEN_REQ].remain_units.erase(unit_id);
        if (REQS[req_id % LEN_REQ].remain_units.empty())
        {
            completed_reqs.push_back(req_id);
            // 更新对象
            OBJECTS[REQS[req_id % LEN_REQ].obj_id].req_ids.erase(req_id);
        }
    }

    // 更新磁盘
    if (!completed_reqs.empty() && obj_id > 0)
    {
        // 修复req_id使用问题
        int first_req_id = *req_ids.begin();
        for (auto &[disk_id, cell_idxs] : OBJECTS[REQS[first_req_id % LEN_REQ].obj_id].replicas)
        {
            for (int req_id : completed_reqs)
            {
                DISKS[disk_id].remove_req(req_id);
            }
            for (int cell_idx : cell_idxs)
            {
                for (int req_id : completed_reqs)
                {
                    DISKS[disk_id].cells[cell_idx]->req_ids.erase(req_id);
                }
            }
        }
    }

    return completed_reqs;
}

void Disk::remove_req(int req_id)
{
    // cell在读取中已经被清除req_ids，所以不需要再清除
    if (req_pos.find(req_id) != req_pos.end())
    {
        req_pos.erase(req_id);
    }
    return;
}

int Disk::_get_best_start(int timestamp)
{
    if (req_pos.empty())
    {
        return -1;
    }
    if (req_pos.size() == 1)
    {
        return req_pos.begin()->second[0];
    }
    // 找到离point最近的
    int start = point;

    // 连续4个空，认定为零散点，触发扫盘。
    for (int i = 0; i < 4; i++)
    {
        start = start < get_parts(0, 0)[0].start ? start : 1;
        if (!cells[start]->req_ids.empty())
        {
            return start;
        }
        start = start % size + 1;        
    }

    _get_consume_token(point, prev_read_token, point);
    
    if (TIME%5000==0)
    {
        int tmp = point;
        for (int i = 0; i<size; i++)
        {
            debug(tmp, consume_token_tmp[tmp]);
            tmp = tmp % size + 1;
        }
        debug(TIME, "=====================================================");
    }

    for (int i = 0; i < size-get_parts(0, 0)[0].free_cells; ++i)
    {
        start = start < get_parts(0, 0)[0].start ? start : 1;
        if (!cells[start]->req_ids.empty())
        {
            return start;
        }
        start = start % size + 1;
    }
}

void Disk::_get_consume_token(int start_point, int last_token, int target_point)
{
    // 初始化
    int check_point = start_point;
    auto fo_seq = EMPTY_SEQUENCE;
    update_sequence(fo_seq, !cells[check_point]->req_ids.empty());
    auto result_tmp = get_decision(prev_read_token, fo_seq);
    consume_token_tmp[check_point] = result_tmp.cost;

    int prev_point = check_point;
    check_point = check_point % size + 1;

    // 在target_point之前停止循环，所以扫盘需传入point作为target_point
    while (check_point != target_point)
    {
        update_sequence(fo_seq, !cells[check_point]->req_ids.empty());
        result_tmp = get_decision(prev_read_token, fo_seq);
        consume_token_tmp[check_point] = consume_token_tmp[prev_point] + result_tmp.cost;
        check_point = check_point % size + 1;
        prev_point = prev_point % size + 1;
    }
}