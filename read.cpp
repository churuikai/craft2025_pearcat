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
    bool is_over_load = false;
    int tag = OBJECTS[obj_id].tag;
    int min_read_tag = get_min_read_tag();
    // debug(TIME, tag, min_read_tag);
    // 更新磁盘
    for (int i = 0; i < 3 - BACK_NUM; ++i)
    {
        auto [disk_id, cells_idx] = OBJECTS[obj_id].replicas[i];
        if (1.0 * G / DISKS[disk_id].req_pos.size() / V < LOAD_COEFFICIENT and tag == min_read_tag)
        {
            is_over_load = true;
            over_load_reqs.push_back(req_id);
        }
    }
    if (not is_over_load)
    {
        for (int i = 0; i < 3 - BACK_NUM; ++i)
        {
            auto [disk_id, cells_idx] = OBJECTS[obj_id].replicas[i];
            DISKS[disk_id].add_req(req_id, cells_idx);
        }
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

// 处理请求
std::pair<std::vector<std::string>, std::vector<int>> Controller::read()
{
    std::vector<std::string> ops;
    std::vector<int> completed_reqs;
    if(TIME>18000) debug(111);
    for (int disk_id = 1; disk_id <= N; ++disk_id)
    {
        // 磁头1
        auto [op1, completed_req1] = DISKS[disk_id].read(1);
        ops.push_back(op1);
        completed_reqs.insert(completed_reqs.end(), completed_req1.begin(), completed_req1.end());
        if(TIME>18000) debug(222);
        // 磁头2
        auto [op2, completed_req2] = DISKS[disk_id].read(2);
        ops.push_back(op2);
        completed_reqs.insert(completed_reqs.end(), completed_req2.begin(), completed_req2.end());
        if(TIME>18000) debug(333);
    }
    if(TIME>18000) debug(444);
    // 更新activate_reqs
    for (int req_id : completed_reqs) {
        activate_reqs.erase(req_id);
    }
    return {ops, completed_reqs};
}


std::pair<std::string, std::vector<int>> Disk::read(int op_id)
{
    int &point =            op_id == 1 ? point1 : point2;
    int &tokens =           op_id == 1 ? tokens1 : tokens2;
    int &prev_read_token =  op_id == 1 ? prev_read_token1 : prev_read_token2;


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
    int &point =            op_id == 1 ? point1 : point2;
    int &tokens =           op_id == 1 ? tokens1 : tokens2;
    int &prev_read_token =  op_id == 1 ? prev_read_token1 : prev_read_token2;
    int data_size =         op_id == 1 ? data_size1 : data_size2;
    int part_start =        op_id == 1 ? 1 : data_size1+1;
    int part_end =          op_id == 1 ? data_size1 : data_size1+data_size2;

    if (req_pos.empty())
    {
        return -1;
    }
    if (req_pos.size() == 1)
    {
        if(req_pos.begin()->second[0] >= part_start and req_pos.begin()->second[0] <= part_end) return req_pos.begin()->second[0];
        else return -1;
    }
    // 找到离point最近的
    int start = point;

    for (int i = 0; i < data_size; ++i)
    {
        start = start <= part_end ? start : part_start;
        if (!cells[start]->req_ids.empty())
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

    // if (point!=start)
    // {
    //     int data_length = size - get_parts(0, 0)[0].free_cells;
    //     int p_length = (start + data_length - point) % data_length;
    //     path.append(p_length, 'p');
    //     tokens -= p_length;
    //     prev_read_token = 80;
    //     point = start;
    // }

    int &point =            op_id == 1 ? point1 : point2;
    int &tokens =           op_id == 1 ? tokens1 : tokens2;
    int &prev_read_token =  op_id == 1 ? prev_read_token1 : prev_read_token2;
    int data_size =         op_id == 1 ? data_size1 : data_size2;
    int part_start =        op_id == 1 ? 1 : data_size1+1;
    int part_end =          op_id == 1 ? data_size1 : data_size1+data_size2;

    int win_end = point;
    auto fo_seq = EMPTY_SEQUENCE;
    int point_start = point;

    // 定义临时函数判断是否读取
    auto is_read = [&](int cell_idx) -> bool
    {
        return not cells[cell_idx]->req_ids.empty() and cell_idx >= part_start and cell_idx <= part_end;
    };


    bool arrive = false;
    for (int i = 0; i < 13; ++i)
    {
        // 更新滑动窗口序列
        if(win_end == start) arrive = true;
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
            auto completed_reqs_cell = cells[point]->read();
            completed_reqs.insert(completed_reqs.end(), completed_reqs_cell.begin(), completed_reqs_cell.end());
        }
        else
        {
            path.append(1, 'p');
        }
        tokens -= decision.cost;
        prev_read_token = decision.next_token;
        point = point % size + 1;
        if(win_end == start) arrive = true;
        update_sequence(fo_seq, is_read(win_end));
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
                    if (DISKS[disk_id].cells[cell_idx]->req_ids.find(req_id) != DISKS[disk_id].cells[cell_idx]->req_ids.end())
                    {
                        DISKS[disk_id].cells[cell_idx]->req_ids.erase(req_id);
                        DISKS[disk_id].req_cells_num--;
                    }
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

// int Disk::_get_best_start(int timestamp)
// {
//     if (req_pos.empty())
//     {
//         return -1;
//     }
//     if (req_pos.size() == 1)
//     {
//         return req_pos.begin()->second[0];
//     }
//     // 找到离point最近的
//     int start = point;

//     // 连续4个空，认定为零散点，触发扫盘。
//     int count = 0;
//     // if (TIME >= 0)

//     for (int i = 0; i < 4; i++)
//     {
//         start = start < get_parts(0, 0)[0].start ? start : 1;
//         if (!cells[start]->req_ids.empty())
//         {
//             count++;
//             // return start;
//         }
//         start = start % size + 1;
//     }
//     start = point;
//     if (count < 1 and TIME>=50000 and TIME <= 80000)
//     {
//         // if(id==1)
//         // {
//         //     debug(TIME, req_cells_num);
//         // }
//         _get_consume_token(start, prev_read_token, start == 1 ? size : start - 1);

//         // if (TIME%5000==0 and id==1)
//         // {
//         //     int tmp = point;
//         //     for (int i = 0; i<size; i++)
//         //     {
//         //         debug(tmp, consume_token_tmp[tmp]);
//         //         tmp = tmp % size + 1;
//         //     }
//         //     debug(TIME, "=====================================================");
//         // }
//         start = point;
//         float max_earnings = 0;
//         float current_scores = 0;
//         float attenuation_rate = 10000;
//         float gain_scores = 0;
//         float attenuation_scores = 0;
//         int abandon_reqs = 0;
//         int best_start = start;
//         for (int n = 0; n < (size - get_parts(0, 0)[0].free_cells) * 4 / 5; ++n)
//         {
//             start = start < get_parts(0, 0)[0].start ? start : 1;
//             abandon_reqs += cells[start]->req_ids.size();
//             gain_scores = (consume_token_tmp[start] / G_float - (n > G ? 1 : n / G_float)) * (req_cells_num - abandon_reqs);
//             attenuation_scores = ((consume_token_tmp[point == 1 ? size : point - 1] -consume_token_tmp[start])/ G_float + (n > G ? 1 : n / G_float)) * abandon_reqs;
//             current_scores = gain_scores - attenuation_rate * attenuation_scores;
//             // if (TIME == 61 && id == 8)
//             // {
//             //     debug(start, current_scores);
//             //     debug(req_cells_num, abandon_reqs);
//             // }

//             if (max_earnings <= current_scores and abandon_reqs > 0)
//             {
//                 max_earnings = current_scores;
//                 best_start = start;
//             }
//             start = start % size + 1;
//         }

//         if (max_earnings > 0)
//         {
//             debug(TIME, id, point);
//             debug(best_start%size+1);
//             return best_start%size+1;
//         }
//     }

//     start = point;
//     for (int i = 0; i < size - get_parts(0, 0)[0].free_cells; ++i)
//     {
//         start = start < get_parts(0, 0)[0].start ? start : 1;
//         if (!cells[start]->req_ids.empty())
//         {
//             return start;
//         }
//         start = start % size + 1;
//     }

//     assert(false);
//     return -1;
// }

// void Disk::_get_consume_token(int start_point, int last_token, int target_point)
// {
//     // 初始化
//     int prev_point = start_point;
//     int check_point = start_point;
//     auto fo_seq = EMPTY_SEQUENCE;
//     int read_token_tmp = last_token;
//     int continue_f_count = 0;
//     for (int i = 0; i < 13; ++i)
//     {
//         // 更新滑动窗口序列
//         update_sequence(fo_seq, !cells[check_point]->req_ids.empty());
//         check_point = check_point % size + 1;
//     }
//     if (cells[prev_point]->req_ids.empty())
//     {
//         continue_f_count++;
//     }
//     auto result_tmp = get_decision(read_token_tmp, fo_seq);
//     consume_token_tmp[prev_point] = result_tmp.cost;
//     read_token_tmp = result_tmp.next_token;

//     // 在target_point之前停止循环，所以扫盘需传入point作为target_point
//     while (prev_point != target_point)
//     {
//         update_sequence(fo_seq, !cells[check_point]->req_ids.empty());
//         result_tmp = get_decision(read_token_tmp, fo_seq);
//         prev_point = prev_point % size + 1;
//         if (cells[prev_point]->req_ids.empty())
//         {
//             continue_f_count++;
//         }
//         else
//         {
//             continue_f_count = 0;
//         }

//         if (continue_f_count >= G)
//         {
//             consume_token_tmp[prev_point] = consume_token_tmp[prev_point == 1 ? size : prev_point - 1];
//         }
//         else
//         {
//             consume_token_tmp[prev_point] = consume_token_tmp[prev_point == 1 ? size : prev_point - 1] + result_tmp.cost;
//             read_token_tmp = result_tmp.next_token;
//         }
//         check_point = check_point % size + 1;
//     }
// }