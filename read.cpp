#include "disk_obj_req.h"
#include "constants.h"
#include <cstring>
// #include "debug.h"

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
        req_pos[req_id] = cells_idx;
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
    if (start == -1)
    {
        return {"#", std::vector<int>()};
    }

    // 如果最佳起点读取代价大于剩余令牌数，则J
    if ((start - point + size) % size > tokens - 16)
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
        auto [path, completed_reqs, _] = _read_by_best_path();
        return {path + "#", completed_reqs};
    }
}

std::tuple<std::string, std::vector<int>, std::vector<int>> Disk::_read_by_best_path()
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

    while (true)
    {
        // 如果是读取
        if (!cells[point]->req_ids.empty())
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
    int start_start = -1;
    int count = 0;
    for (int i = 0; i < size-part_tables[0][2]; ++i)
    {
        if (!cells[start]->req_ids.empty())
        {
            return start;
            if (start_start == -1)
            {
                start_start = start;
            }
            count++;
        }
        start = start % size + 1;
        start = start == part_tables[0][0] ? 1: start;
    }
    if (count >= 1)
    {
        return start_start;
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
    while (count < 50)
    {
        if (cells[start_prev]->req_ids.empty())
        {
            count++;
        }
        else
        {
            start = start_prev;
        }
        start_prev = (start_prev + size - 2) % size + 1;
    }
    assert(start_prev > 0);
    return start;
}