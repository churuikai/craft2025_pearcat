#include "constants.h"
#include "io.h"
#include "controller.h"
#include "disk_obj_req.h"
#include "debug.h"

// 删除
std::vector<int> Controller::delete_obj(int obj_id)
{
    // 释放磁盘
    for (auto &[disk_id, units] : OBJECTS[obj_id].replicas)
    {
        for (int cell_id : units)
        {
            DISKS[disk_id].free_cell(cell_id);
        }
    }
    // 释放请求
    std::vector<int> aborted_requests(OBJECTS[obj_id].req_ids.begin(), OBJECTS[obj_id].req_ids.end());
    for (int req_id : aborted_requests)
    {
        REQS[req_id % LEN_REQ].clear();
    }
    OBJECTS[obj_id].req_ids.clear();
    OBJECTS[obj_id].id = 0;
    OBJECTS[obj_id].replicas.clear();
    OBJECTS[obj_id].size = 0;
    OBJECTS[obj_id].tag = 0;
    return aborted_requests;
}

void Disk::free_cell(int cell_id)
{ 
    Part* part = cells[cell_id].part;

    // 更新空闲块链表（只更新非备份区的分区）
    if (part->tag != 0) {
        part->free_block(cell_id);
    }
    part->free_cells++;
    
    cells[cell_id].free();
}