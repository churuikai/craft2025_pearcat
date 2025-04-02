#include "constants.h"
#include "disk_obj_req.h"
#include "debug.h"

void process_busy(Controller &controller)
{
    // 清理长周期的请求
    std::vector<int> busy_req_ids;

    if (TIME % 5 == 0)
    {
        for (int req_id : controller.activate_reqs)
        {
            if (REQS[req_id % LEN_REQ].timestamp + 100 < TIME)
            {
                busy_req_ids.push_back(req_id);
                // 释放磁盘
                for (auto &[disk_id, cell_idxs] : OBJECTS[REQS[req_id % LEN_REQ].obj_id].replicas)
                {
                    for (int cell_idx : cell_idxs)
                    {
                        if (DISKS[disk_id].cells[cell_idx]->req_ids.find(req_id) != DISKS[disk_id].cells[cell_idx]->req_ids.end())
                        {
                            DISKS[disk_id].cells[cell_idx]->req_ids.erase(req_id);
                            DISKS[disk_id].req_cells_num--;
                        }
                    }
                    // 释放磁盘
                    DISKS[disk_id].req_pos.erase(req_id);
                }
                // 释放对象
                OBJECTS[REQS[req_id % LEN_REQ].obj_id].req_ids.erase(req_id);
            }
        }
        // 清理activate_reqs
        for (int req_id : busy_req_ids)
        {
            controller.activate_reqs.erase(req_id);
        }
    }
    int n_busy = busy_req_ids.size();
    printf("%d\n", n_busy);
    for (int i = 0; i < n_busy; i++) {
        printf("%d\n", busy_req_ids[i]);
    }
    fflush(stdout);
    if(n_busy > 0)
    {
        debug("some busy==================================================================================");
        debug(TIME, busy_req_ids.size());
        debug(controller.activate_reqs.size());
        debug(1.0*G/controller.activate_reqs.size()/N/V);
    }
    // 更新负载系数
    for (int i = 0; i < N; i++) {
        DISKS[i].load_coefficient = 1.0*G/DISKS[i].req_pos.size()/V;
    }
}
