#include "disk_obj_req.h"
#include "constants.h"
#include <cstring>
#include "debug.h"

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

    // 如果有上次缓存
    // if (!prev_occupied_obj.empty())
    // {
    //     for (int obj_id : prev_occupied_obj)
    //     {
    //         OBJECTS[obj_id].occupied = false;
    //     }
    //     prev_occupied_obj.clear();
    // }

    int start = _get_best_start(timestamp);
    //if(TIME==2)
    //debug(start);
    if (start == -1)
    {
        return {"#", std::vector<int>()};
    }

    // 如果最佳起点读取代价大于剩余令牌数，则J
    if ((start - point + size) % size > tokens - 64)
    {
        // if((start < point or (point==1 &&  start > point))&& id==1){
        //     debug(TIME,point);
        // }
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

    int free_count = 0;
    int read_count = 0;
    int end_token = prev_read_token;
    bool r_or_pr = false;
    int token = 0;

    bool arr_start = false;
    while (true)
    {
        if(point == start) arr_start = true;
        // 如果是读取
        if (!cells[point]->req_ids.empty() && arr_start )
        {
            RPRResult result = GET_TOKEN_TABLE(prev_read_token, free_count, read_count + 1);

            if (tokens - result.cost < 0)
            {
                // 结算
                prev_read_token = end_token;
                if (r_or_pr)
                {
                    path.append(free_count + read_count, 'r');
                }
                else
                {
                    path.append(free_count, 'p');
                    path.append(read_count, 'r');
                }
                tokens -= token;
                return {path, completed_reqs, occupied_obj};
            }

            // 读取该cell
            req_cells_num -= cells[point]->req_ids.size();
            auto reqs = cells[point]->read();
            completed_reqs.insert(completed_reqs.end(), reqs.begin(), reqs.end());

            // 累积
            end_token = result.prev_token;
            r_or_pr = result.r_or_p;
            token = result.cost;
            read_count++;
        }
        // 如果是跳过
        else
        {
            if (read_count > 0)
            {
                // 结算
                prev_read_token = end_token;
                end_token = 80;
                if (r_or_pr)
                {
                    path.append(free_count + read_count, 'r');
                }
                else
                {
                    path.append(free_count, 'p');
                    path.append(read_count, 'r');
                }
                tokens -= token;
                read_count = 0;
                free_count = 0;
                r_or_pr = false;
                token = 0;
            }

            if (tokens - free_count - 1 < 0)
            {
                // 结算
                if (free_count > 0)
                {
                    prev_read_token = 80;
                }
                path.append(free_count, 'p');
                tokens -= free_count;
                return {path, completed_reqs, occupied_obj};
            }
            else
            {
                end_token = 80;
                free_count++;
            }
        }
        // if(point == 1 && id==1){
        //     debug(TIME, point);
        // }
        point = point % size + 1;
    }
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
    int count = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (!cells[start]->req_ids.empty())
        {
            // // 如果是孤立的，并且请求数量大于250，则不读取
            // // return start;
            // if(i>6 and req_pos.size() > 100*cells[start]->req_ids.size()){
            //     int alone_count = 2;
            //     int tmp = start % size + 1;
            //     tmp = tmp == part_tables[0][0] ? 1: tmp;
            //     while(alone_count--){
            //         if(!cells[tmp]->req_ids.empty()){
            //             return start;
            //         }
            //         tmp = tmp % size + 1;
            //         tmp = tmp == part_tables[0][0] ? 1: tmp;
            //     }
            //     if(id==1){
            //         debug(TIME, start);
            //     }

            // }
            // else{
            //     return start;
            // }

            return start;
            // if (start_start == -1)
            // {
            //     start_start = start;
            // }
            // count++;
        }
        start = start % size + 1;
    }

    _get_consume_token(point, prev_read_token, point==1?size:point-1);
//    _get_consume_token(1, prev_read_token, size);

    if(TIME%5000 == 0)
    {
        int tmp = point;
        for(int i = 0; i<size; i++ ){
            tmp = tmp%size+1;
        debug(i, consume_token_tmp[tmp]);
        
        }
        debug(TIME, "=====================================================");
    }

    double max_earnings = 0;
    double current_score;
    start = point;
    int best_start = point;
    int abandon_reqs = 0;
    for (int n = 0; n < size-1; n++)
    {
        start = start % size + 1;
        abandon_reqs += cells[start]->req_ids.size();
        current_score = (consume_token_tmp[start] / G_float - (n > G_float ? 1 : n / G_float)) * req_cells_num - abandon_reqs*consume_token_tmp[start == 1 ? size : start - 1] / G_float;
        if (max_earnings <= current_score)
        {
            max_earnings = current_score;
            best_start = start;
        }
    }

    if (max_earnings > 0){
        //debug(TIME, id, point);
        //debug(best_start);

        return best_start;
        
    }
    start = point;
    for (int i = 0; i < size - part_tables[0][2]; ++i)
    {
        if (!cells[start]->req_ids.empty())
        {
            return start;
        }
        start = start % size + 1;
    }

    // 附近没有就选一个
    int max_count = 0;
    auto iters = req_pos.begin();
    for (auto iter = req_pos.begin(); iter != req_pos.end(); ++iter)
    {
        if (cells[iter->second[0]]->req_ids.size() >= max_count)
        {
            max_count = cells[iter->second[0]]->req_ids.size();
            start = iter->second[0];
            iters = iter;
        }
    }

    int start_prev = (start + size - 2) % size + 1;
    count = 0;
    int conti_count = 0;
    while (conti_count < 10)
    {
        if (cells[start_prev]->req_ids.empty())
        {
            conti_count++;
            count++;
        }
        else
        {
            conti_count = 0;
            start = start_prev;
        }
        start_prev = (start_prev + size - 2) % size + 1;
    }
    assert(start_prev > 0);

    return start;
}

void Disk::_get_consume_token(int start_point, int last_token, int target_point)
{
    // 数据区边界，理论上point到达尾边界需要jump
    int data_zone_end = part_tables[0][0] - 1;
    int data_length = data_zone_end;
    // int data_length = data_zone_end - data_zone_start + 1;

    // 扫盘，计算从point到每一个磁盘点的token消耗
    int check_point = start_point;
    int o_count = 0;
    int f_count = 0;
    enum State
    {
        FO,
        OF,
        OO,
        FF
    };
    bool r_or_pr;
    int prev_f_pos = start_point;
    int prev_last_token = last_token;
    int prev_cost = 0;
    consume_token_tmp[start_point==1?size:start_point-1] = 0;
    State state;

    if (!cells[start_point]->req_ids.empty())
    {
        state = FO;
    }
    else
    {
        state = FF;
    }
    while (check_point != target_point)
    {
        switch (state)
        {
        case FO:
        case OO:
        {
            o_count++;
            RPRResult result = GET_TOKEN_TABLE(last_token, f_count, o_count);
            // 记录消耗
            consume_token_tmp[check_point] = result.cost+prev_cost;
            last_token = result.prev_token;
            r_or_pr = result.r_or_p;
            check_point = check_point % size + 1;
            state = cells[check_point]->req_ids.empty() ? OF : OO;
            break;
        }
        case OF:
        {
            f_count = 1;
            o_count = 0;
            // 示例：更新[start_pos, current_pos]的token消耗
            if (r_or_pr)
            {
                int continuous_count = 0;
                while (cells[prev_f_pos]->req_ids.empty())
                {
                    if (++continuous_count > G)
                    {
                        consume_token_tmp[prev_f_pos] = consume_token_tmp[(prev_f_pos + size - 1) % size == 0 ? size : (prev_f_pos + size - 1) % size];
                    }
                    else
                    {
                        prev_last_token = get_next_token(prev_last_token);
                        consume_token_tmp[prev_f_pos] = prev_last_token - 1;
                    }
                    prev_f_pos = prev_f_pos % size + 1;
                }
            }

            // 记录消耗
            consume_token_tmp[check_point] = consume_token_tmp[check_point==1?size:check_point-1] + 1;
            last_token = 80;
            prev_f_pos = check_point;
            prev_last_token = last_token;
            prev_cost +=  consume_token_tmp[check_point==1?size:check_point-1];
            check_point = check_point % size + 1;
            state = cells[check_point]->req_ids.empty() ? FF : FO;
            break;
        }
        case FF:
        {
            f_count++;
            consume_token_tmp[check_point] = consume_token_tmp[check_point==1?size:check_point-1] + 1;
            check_point = check_point % size + 1;
            state = cells[check_point]->req_ids.empty() ? FF : FO;
            break;
        }
        default:
            break;
        }
    }

    if (!cells[target_point]->req_ids.empty() && r_or_pr)
    {
        while (cells[prev_f_pos]->req_ids.empty())
        {
            prev_last_token = get_next_token(prev_last_token);
            consume_token_tmp[prev_f_pos] = prev_last_token - 1;
            prev_f_pos = prev_f_pos % size + 1;
        }
    }
}
