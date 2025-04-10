#pragma once
#include "controller.h"

inline void process_timestamp(Controller &controller, int timestamp)
{
    scanf("%*s%*d");
    // 更新各个磁盘的tokens
    for (int i = 1; i <= N; ++i) {
        controller.DISKS[i].tokens1 = G;
        controller.DISKS[i].tokens2 = G;
        controller.DISKS[i].K = k;
    }
    // 同步时间
    controller.timestamp = timestamp;

    printf("TIMESTAMP %d\n", timestamp);
    fflush(stdout);

};
void process_delete(Controller &controller);
void process_write(Controller &controller);
void process_read(Controller &controller);

inline void process_busy(Controller &controller)
{
    // 清理长周期的请求
    std::vector<int> busy_req_ids;
    if (controller.timestamp % 10 == 0)
    {
        for (int req_id : controller.activate_reqs)
        {
            if (controller.REQS[req_id % LEN_REQ].timestamp + 95 < controller.timestamp)
            {
                busy_req_ids.push_back(req_id);
                // 释放磁盘
                for (auto &[disk_id, cell_idxs] : controller.OBJECTS[controller.REQS[req_id % LEN_REQ].obj_id].replicas)
                {
                    for (int cell_idx : cell_idxs)
                    {
                        if (controller.DISKS[disk_id].cells[cell_idx]->req_ids.find(req_id) != controller.DISKS[disk_id].cells[cell_idx]->req_ids.end())
                        {
                            controller.DISKS[disk_id].cells[cell_idx]->req_ids.erase(req_id);
                            controller.DISKS[disk_id].req_cells_num--;
                        }
                    }
                    // 释放磁盘
                    controller.DISKS[disk_id].req_pos.erase(req_id);
                }
                // 释放对象
                controller.OBJECTS[controller.REQS[req_id % LEN_REQ].obj_id].req_ids.erase(req_id);
            }
        }
        // 清理activate_reqs
        for (int req_id : busy_req_ids)
        {
            controller.activate_reqs.erase(req_id);
        }
    }
    int n_over_load = controller.over_load_reqs.size();
    int n_busy = busy_req_ids.size();
    printf("%d\n", n_busy+n_over_load);
    for (int i = 0; i < n_busy; i++) {
        printf("%d\n", busy_req_ids[i]);
    }
    for (int i = 0; i < n_over_load; i++) {
        printf("%d\n", controller.over_load_reqs[i]);
    }
    fflush(stdout);
    controller.over_load_reqs.clear();
    // if(n_busy > 0)
    // {
    //     debug("some busy==================================================================================");
    //     debug(TIME, busy_req_ids.size());
    //     debug(controller.activate_reqs.size());
    //     debug(1.0*G/controller.activate_reqs.size()*N/V);
    // }
    // if(n_over_load > 0){
    //     debug("some over load==================================================================================");
    //     debug(TIME, n_over_load);
    // }
    controller.busy_count += n_busy;
    controller.over_load_count += n_over_load;
    // 更新负载系数
    for (int i = 0; i < N; i++) {
        controller.DISKS[i].load_coefficient = 1.0*G/controller.DISKS[i].req_pos.size()/V;
    }
}

void process_gc(Controller &controller);